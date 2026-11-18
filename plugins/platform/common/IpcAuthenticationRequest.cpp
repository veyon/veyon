// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IpcAuthenticationRequest.h"
#include "IpcClient.h"


// overwrites the bytes of an unshared buffer before it is released (not elided by the compiler)
static void wipeByteArray(QByteArray& data)
{
	volatile auto* bytes = data.data();
	for (qsizetype i = 0; i < data.size(); ++i)
	{
		bytes[i] = 0;
	}
	data.clear();
}



QJsonObject IpcAuthenticationRequest::encode(const QString& username, const char* password, qsizetype passwordSize)
{
	if (AuthenticationLimiter::isValidRequest(username, password, passwordSize) == false)
	{
		return {};
	}

	auto encodedPassword = QByteArray::fromRawData(password, passwordSize).toBase64();
	QJsonObject arguments{
		{IpcProtocol::Key::Username, username},
		{IpcProtocol::Key::Password, QString::fromLatin1(encodedPassword)},
	};
	wipeByteArray(encodedPassword);

	return arguments;
}



std::optional<IpcAuthenticationRequest::Arguments> IpcAuthenticationRequest::decode(const QJsonObject& arguments)
{
	// base64 of MaximumPasswordSize bytes, checked before anything is decoded
	static constexpr auto MaximumEncodedPasswordSize = (AuthenticationLimiter::MaximumPasswordSize + 2) / 3 * 4;

	const auto username = arguments.value(IpcProtocol::Key::Username);
	const auto encodedPassword = arguments.value(IpcProtocol::Key::Password);
	if (arguments.size() != 2 ||
		username.isString() == false ||
		encodedPassword.isString() == false ||
		encodedPassword.toString().size() > MaximumEncodedPasswordSize)
	{
		return {};
	}

	auto encodedPasswordData = encodedPassword.toString().toLatin1();
	auto decodedPassword = QByteArray::fromBase64Encoding(encodedPasswordData, QByteArray::AbortOnBase64DecodingErrors);
	const auto isStrictBase64 = decodedPassword.decodingStatus == QByteArray::Base64DecodingStatus::Ok &&
		(decodedPassword.decoded.size() + 2) / 3 * 4 == encodedPasswordData.size();
	wipeByteArray(encodedPasswordData);

	if (isStrictBase64 == false ||
		AuthenticationLimiter::isValidRequest(username.toString(), decodedPassword.decoded.constData(),
											  decodedPassword.decoded.size()) == false)
	{
		wipeByteArray(decodedPassword.decoded);
		return {};
	}

	Arguments result{username.toString(), Veyon::Crypto::SecureBuffer::fromByteArray(decodedPassword.decoded)};
	wipeByteArray(decodedPassword.decoded);

	return result;
}



bool IpcAuthenticationRequest::authenticate(IpcClient& client, const QString& username,
											const char* password, qsizetype passwordSize)
{
	const auto arguments = encode(username, password, passwordSize);
	if (arguments.isEmpty())
	{
		vWarning() << "rejecting invalid credentials";
		return false;
	}

	const auto response = client.call(IpcProtocol::Command::Authenticate, arguments, CallTimeout);
	if (response.ok == false)
	{
		vWarning() << "authentication request failed:" << response.error;
		return false;
	}

	const auto authenticated = response.result.value(IpcProtocol::Key::Authenticated);
	return authenticated.isBool() && authenticated.toBool();
}
