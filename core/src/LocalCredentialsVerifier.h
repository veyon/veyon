// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"
#include "VeyonCore.h"

// Checks logon credentials for veyon-master and veyon-configurator by running only the
// authentication phase of the Veyon protocol (RfbVeyonAuth::Logon) against the Veyon Server of
// the own session, exactly as a remote master does (docs/specs/02-ipc.md §6b). The server checks
// them like any other remote logon (Linux: service IPC, AuthenticationLimiter, veyon-auth-helper;
// Windows: LogonUser()), so any account can be verified without a privileged local interface.
// The connection is closed right after the server's security result.
class VEYON_CORE_EXPORT LocalCredentialsVerifier
{
public:
	enum class Result {
		Valid,
		Invalid,
		// no Veyon Server reachable, protocol mismatch, Logon not offered or no answer in time
		Unavailable,
	};

	// covers the server's own wait for its password check (IpcAuthenticationRequest::CallTimeout)
	static constexpr int Timeout = 15000;

	// verifies with the server of the own session (localhost, Network/VeyonServerPort + session ID)
	// in a separate thread while processing events, so the calling UI stays responsive
	[[nodiscard]] static Result verify(const QString& username, const Veyon::Crypto::SecureBuffer& password);

	// blocking
	[[nodiscard]] static Result verify(const QString& username, const Veyon::Crypto::SecureBuffer& password,
									   const QString& host, quint16 port, int timeout = Timeout);
};
