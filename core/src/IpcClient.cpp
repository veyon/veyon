// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QLocalSocket>

#include "IpcClient.h"
#include "IpcConnection.h"
#include "IpcServer.h"


IpcClient::IpcClient(IpcProtocol::Role role, quint32 sessionId, QObject* parent) :
	QObject(parent),
	m_role(role),
	m_sessionId(sessionId)
{
}



IpcClient::~IpcClient()
{
	delete m_connection;
}



bool IpcClient::connectAndHandshake(const QString& directory, IpcEndpointCredentials&& credentials, int timeout)
{
	if (m_connection)
	{
		vCritical() << "already connected";
		return false;
	}

	QDeadlineTimer deadline(timeout);

	const auto endpointName = credentials.endpointName();
	auto socket = IpcConnection::connectToEndpoint(directory, endpointName, timeout);
	if (socket == nullptr)
	{
		return false;
	}

	m_connection = new IpcConnection(socket, IpcChannel::Side::Client, m_role, m_sessionId,
									 std::move(credentials), nullptr, this);
	connect(m_connection, &IpcConnection::messageReceived, this, [this](const QByteArray& message) {
		// at most one request is in flight, so only a few stray messages can queue up
		if (m_receivedMessages.size() < IpcProtocol::MaximumInFlightRequests)
		{
			m_receivedMessages.append(message);
		}
	});
	connect(m_connection, &IpcConnection::closed, this, []() { vWarning() << "IPC connection closed"; });

	m_connection->start();

	if (m_connection->waitForEstablished(int(qMax<qint64>(0, deadline.remainingTime()))) == false)
	{
		vCritical() << "handshake with" << endpointName << "failed";
		delete m_connection;
		return false;
	}

	return true;
}



bool IpcClient::isConnected() const
{
	return m_connection && m_connection->isEstablished() && m_connection->isClosed() == false;
}



IpcClient::Response IpcClient::call(QLatin1String command, const QJsonObject& arguments, int timeout)
{
	if (isConnected() == false)
	{
		return {};
	}

	const auto requestId = m_nextRequestId++;
	m_receivedMessages.clear();

	if (m_connection->sendMessage(IpcMessage::encodeRequest({requestId, command, arguments})) == false)
	{
		vCritical() << "failed to send request" << command;
		return {};
	}

	QDeadlineTimer deadline(timeout);
	while (deadline.hasExpired() == false)
	{
		while (m_receivedMessages.isEmpty() == false)
		{
			const auto response = IpcMessage::decodeResponse(m_receivedMessages.takeFirst());
			if (response.has_value() && response->id == requestId)
			{
				return *response;
			}
			vWarning() << "discarding unexpected or malformed response";
		}

		if (m_connection->waitForReadyRead(int(qMax<qint64>(1, deadline.remainingTime()))) == false &&
			m_connection->isClosed())
		{
			break;
		}
	}

	vCritical() << "no response for request" << command;
	return {};
}
