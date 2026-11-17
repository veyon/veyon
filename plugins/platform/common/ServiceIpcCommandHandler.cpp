// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ServiceIpcCommandHandler.h"


ServiceIpcCommandHandler::ServiceIpcCommandHandler(const Clock& clock) :
	m_clock(clock)
{
}



IpcMessage::Response ServiceIpcCommandHandler::handleRequest(Client& client, const IpcMessage::Request& request)
{
	vDebug() << "session" << client.sessionId << "command" << request.command;

	if (request.command == IpcProtocol::Command::GetSessionData)
	{
		return getSessionData(client, request.id);
	}

	if (request.command == IpcProtocol::Command::SetSessionData)
	{
		return setSessionData(client, request);
	}

	if (request.command == IpcProtocol::Command::GetTlsHostKey)
	{
		return getTlsHostKey(client, request.id);
	}

	if (request.command == IpcProtocol::Command::GetExternalVncServerPassword)
	{
		return getExternalVncServerPassword(request.id);
	}

	return IpcMessage::failure(request.id, IpcProtocol::Error::UnknownCommandError);
}



void ServiceIpcCommandHandler::removeSessionData(const Client& client)
{
	m_sessionData.erase(sessionKey(client));
}



IpcMessage::Response ServiceIpcCommandHandler::getSessionData(const Client& client, int requestId)
{
	// logon credentials are only for a server at the login screen
	if (client.hasLoggedOnUser == nullptr || client.hasLoggedOnUser())
	{
		vWarning() << "refusing to hand out session data to session" << client.sessionId << "with logged on user";
		return IpcMessage::failure(requestId, IpcProtocol::Error::DeniedError);
	}

	removeExpiredSessionData();

	// only the entry of the caller's own session, at most once
	const auto it = m_sessionData.find(sessionKey(client));
	if (it == m_sessionData.end())
	{
		return IpcMessage::success(requestId);
	}

	const auto data = it->second.data.toUnprotectedByteArray().toBase64();
	m_sessionData.erase(it);

	return IpcMessage::success(requestId, {{IpcProtocol::Key::Data, QString::fromLatin1(data)}});
}



IpcMessage::Response ServiceIpcCommandHandler::setSessionData(const Client& client, const IpcMessage::Request& request)
{
	const auto encodedData = request.arguments.value(IpcProtocol::Key::Data);
	if (encodedData.isString() == false)
	{
		return IpcMessage::failure(request.id, IpcProtocol::Error::BadRequestError);
	}

	const auto decodedData = QByteArray::fromBase64Encoding(encodedData.toString().toLatin1(),
															QByteArray::AbortOnBase64DecodingErrors);
	if (decodedData.decodingStatus != QByteArray::Base64DecodingStatus::Ok)
	{
		return IpcMessage::failure(request.id, IpcProtocol::Error::BadRequestError);
	}

	// logon credentials may only be planted from a session nobody is logged on to
	if (client.hasLoggedOnUser == nullptr || client.hasLoggedOnUser())
	{
		vWarning() << "refusing to store session data for session" << client.sessionId << "with logged on user";
		return IpcMessage::failure(request.id, IpcProtocol::Error::DeniedError);
	}

	removeExpiredSessionData();

	if (decodedData.decoded.isEmpty())
	{
		m_sessionData.erase(sessionKey(client));
	}
	else
	{
		m_sessionData[sessionKey(client)] = {Veyon::Crypto::SecureBuffer::fromByteArray(decodedData.decoded),
											 m_clock() + IpcProtocol::SessionDataLifetime};
	}

	return IpcMessage::success(request.id);
}



IpcMessage::Response ServiceIpcCommandHandler::getTlsHostKey(Client& client, int requestId)
{
	if (++client.tlsHostKeyRequests > IpcProtocol::MaximumTlsHostKeyRequests)
	{
		vWarning() << "too many TLS host key requests from session" << client.sessionId;
		return IpcMessage::failure(requestId, IpcProtocol::Error::DeniedError);
	}

	const auto hostKey = m_tlsHostKeyProvider ? m_tlsHostKeyProvider() : std::nullopt;
	if (hostKey.has_value() == false)
	{
		vDebug() << "no TLS host key available";
		return IpcMessage::failure(requestId, IpcProtocol::Error::DeniedError);
	}

	if (client.weakBootstrap)
	{
		vWarning() << "weak server bootstrap (D-Bus activation): host key exposure possible - session" << client.sessionId;
	}

	return IpcMessage::success(requestId, {
											  {IpcProtocol::Key::TlsKey, QString::fromLatin1(hostKey->privateKeyDer.toUnprotectedByteArray().toBase64())},
											  {IpcProtocol::Key::CertificateChain, QString::fromLatin1(hostKey->certificateChainPem)},
										  });
}



IpcMessage::Response ServiceIpcCommandHandler::getExternalVncServerPassword(int requestId)
{
	const auto password = m_externalVncServerPasswordProvider ? m_externalVncServerPasswordProvider() : std::nullopt;
	if (password.has_value() == false)
	{
		return IpcMessage::failure(requestId, IpcProtocol::Error::DeniedError);
	}

	return IpcMessage::success(requestId, {{IpcProtocol::Key::Password, QString::fromLatin1(password->toUnprotectedByteArray().toBase64())}});
}



void ServiceIpcCommandHandler::removeExpiredSessionData()
{
	const auto now = m_clock();
	for (auto it = m_sessionData.begin(); it != m_sessionData.end();)
	{
		if (now >= it->second.expiryTime)
		{
			it = m_sessionData.erase(it);
		}
		else
		{
			++it;
		}
	}
}
