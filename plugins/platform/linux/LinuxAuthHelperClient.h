// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ServiceIpcCommandHandler.h"

// veyon-service's client of the root helper veyon-auth-helper (D-Bus io.veyon.AuthHelper, root
// only), which checks the credentials of the IPC command Authenticate with PAM
// (docs/specs/02-ipc.md §6b)
class LinuxAuthHelperClient
{
public:
	// ServiceIpcCommandHandler::Authenticator; an error or timeout of the helper is no verdict
	static void authenticate(const QString& username, Veyon::Crypto::SecureBuffer&& password, QObject* context,
							 const ServiceIpcCommandHandler::AuthenticationResultHandler& resultHandler);
};
