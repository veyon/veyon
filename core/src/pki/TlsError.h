// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

// Stable TLS error codes (05-tls.md §5.4), shared by TlsPeerVerifier (PkiPublicApi) and the TLS
// transport (VncTlsTunnel, WP7). The kebab-case codes are a stable interface (logs, master UI,
// admin documentation) and must not be changed.
enum class TlsError {
	None,
	Expired,
	NotYetValid,
	ClockSkew,
	NameMismatch,
	UntrustedCa,
	Revoked,
	PinMismatch,
	HandshakeFailed,
	TlsRequiredPeerPlain,
	DowngradeSuspected,
	CredentialsRefused, // 05-tls.md §6: credential rule refused to send the logon password
};

// e.g. "pin-mismatch"; empty for TlsError::None
[[nodiscard]] VEYON_CORE_EXPORT QString tlsErrorCode(TlsError error);

// translated, user-visible message; clockSkewMinutes is used for TlsError::ClockSkew only
// ("system clock differs by N min from peer")
[[nodiscard]] VEYON_CORE_EXPORT QString tlsErrorMessage(TlsError error, int clockSkewMinutes = 0);
