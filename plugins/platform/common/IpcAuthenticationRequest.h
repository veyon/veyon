// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>

#include "AuthenticationLimiter.h"
#include "IpcProtocol.h"
#include "SecureBuffer.h"

#include <optional>

class IpcClient;

// Arguments of the IPC command Authenticate (docs/specs/02-ipc.md §6, §6b):
// {"username": "<name>", "password": "<base64>"}, result {"authenticated": true|false}.
// The session server sends it to veyon-service instead of using the D-Bus interface.
class IpcAuthenticationRequest
{
public:
	struct Arguments
	{
		QString username;
		Veyon::Crypto::SecureBuffer password;
	};

	// covers a PAM conversation of the service (AuthenticationLimiter::AuthenticateTimeout)
	static constexpr int CallTimeout = AuthenticationLimiter::AuthenticateTimeout + 5000;

	// empty object if the credentials are not valid (AuthenticationLimiter::isValidRequest())
	[[nodiscard]] static QJsonObject encode(const QString& username, const char* password, qsizetype passwordSize);

	// strict: exactly the two members, canonical base64, AuthenticationLimiter::isValidRequest()
	[[nodiscard]] static std::optional<Arguments> decode(const QJsonObject& arguments);

	// client side: true only for a successful response with "authenticated": true
	[[nodiscard]] static bool authenticate(IpcClient& client, const QString& username,
										   const char* password, qsizetype passwordSize);
};
