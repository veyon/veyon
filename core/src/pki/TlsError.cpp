// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>

#include "TlsError.h"


QString tlsErrorCode(TlsError error)
{
	switch (error)
	{
	case TlsError::None: return {};
	case TlsError::Expired: return QStringLiteral("expired");
	case TlsError::NotYetValid: return QStringLiteral("not-yet-valid");
	case TlsError::ClockSkew: return QStringLiteral("clock-skew");
	case TlsError::NameMismatch: return QStringLiteral("name-mismatch");
	case TlsError::UntrustedCa: return QStringLiteral("untrusted-ca");
	case TlsError::Revoked: return QStringLiteral("revoked");
	case TlsError::PinMismatch: return QStringLiteral("pin-mismatch");
	case TlsError::HandshakeFailed: return QStringLiteral("handshake-failed");
	case TlsError::TlsRequiredPeerPlain: return QStringLiteral("tls-required-peer-plain");
	case TlsError::DowngradeSuspected: return QStringLiteral("downgrade-suspected");
	case TlsError::CredentialsRefused: return QStringLiteral("credentials-refused");
	}

	return {};
}



QString tlsErrorMessage(TlsError error, int clockSkewMinutes)
{
	switch (error)
	{
	case TlsError::None:
		return {};
	case TlsError::Expired:
		return QCoreApplication::translate("TlsError", "certificate expired");
	case TlsError::NotYetValid:
		return QCoreApplication::translate("TlsError", "certificate not yet valid");
	case TlsError::ClockSkew:
		return QCoreApplication::translate("TlsError", "system clock differs by %1 min from peer").arg(clockSkewMinutes);
	case TlsError::NameMismatch:
		return QCoreApplication::translate("TlsError", "certificate does not match the computer name or address");
	case TlsError::UntrustedCa:
		return QCoreApplication::translate("TlsError", "certificate not issued by a trusted certificate authority");
	case TlsError::Revoked:
		return QCoreApplication::translate("TlsError", "certificate revoked");
	case TlsError::PinMismatch:
		return QCoreApplication::translate("TlsError", "server key does not match the pinned fingerprint");
	case TlsError::HandshakeFailed:
		return QCoreApplication::translate("TlsError", "TLS handshake failed");
	case TlsError::TlsRequiredPeerPlain:
		return QCoreApplication::translate("TlsError", "TLS required but the computer only supports unencrypted connections");
	case TlsError::DowngradeSuspected:
		return QCoreApplication::translate("TlsError", "possible downgrade attack");
	case TlsError::CredentialsRefused:
		return QCoreApplication::translate("TlsError", "credentials not sent: server identity not verified");
	}

	return {};
}
