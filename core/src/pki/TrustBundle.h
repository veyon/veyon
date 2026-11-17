// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Certificate.h"
#include "SecureBuffer.h"
#include "VeyonCore.h"

// Trust bundle veyon-trust-bundle.json (06-pki.md §4, part of PkiPublicApi):
//   {"v":1,
//    "caCertificates":"<PEM bundle, 1..n certificates>",
//    "enrollment":{"url":"https://pki-host.example.org:11010","token":"<JWT>"},
//    "policy":{"certValidityDays":730,"renewBeforeDays":180},
//    "modes":{"server":"Optional","client":"Prefer"}}
//
// fromJson() parses untrusted input (files from deployment shares, installer parameters) and fails
// closed: size limit before parsing, "v" must be 1, every known field must have the documented
// type and value range, unknown fields are ignored. Duplicate keys cannot be detected with
// QJsonDocument (one of the values is used, the last one with Qt 6); since every Veyon tool parses
// bundles with this function and the import confirmation shows the CA of the parsed result, this
// creates no difference between what is shown and what is imported. A UTF-8 BOM is accepted.
// The bundle has no signature of its own; the caller (import, WP11f) shows the CA subject and
// fingerprint for interactive confirmation and applies it in a privileged context
// (TLS/CaCertificate is replaced, not appended).
//
// The bundle contains the enrollment token, a secret: it is kept in a SecureBuffer and never
// logged (QJsonDocument keeps temporary copies that cannot be zeroised). Move-only.
class VEYON_CORE_EXPORT TrustBundle
{
public:
	static constexpr qsizetype MaximumSize = 64 * 1024;
	static constexpr int Version = 1;
	static constexpr qsizetype MaximumUrlSize = 2048;
	static constexpr qsizetype MaximumTokenSize = 4 * 1024;
	// value ranges of TLS/Enrollment/CertValidityDays and TLS/Enrollment/RenewBeforeDays
	// (03-secretstore-config.md §5)
	static constexpr int MinimumCertValidityDays = 30;
	static constexpr int MaximumCertValidityDays = 1095;
	static constexpr int MinimumRenewBeforeDays = 7;
	static constexpr int MaximumRenewBeforeDays = 365;

	// 1..Certificate::MaximumChainLength CA certificates (TlsClientPolicy::isAcceptableCaCertificate())
	QList<Veyon::Crypto::Certificate> caCertificates;

	// optional: normalised https URL with host and optional port only (normalizedEnrollmentUrl())
	QString enrollmentUrl;
	// optional, requires enrollmentUrl: JWS compact serialisation (three base64url parts)
	Veyon::Crypto::SecureBuffer enrollmentToken;

	// optional, informational for clients (06-pki.md §4)
	std::optional<int> certValidityDays;
	std::optional<int> renewBeforeDays;

	// optional: applied to TLS/ServerMode and TLS/ClientMode only if present
	std::optional<VeyonCore::TlsServerMode> serverMode;
	std::optional<VeyonCore::TlsClientMode> clientMode;

	[[nodiscard]] static std::optional<TrustBundle> fromJson(Veyon::Crypto::ByteView json);

	// validates the bundle like fromJson() and serialises it; std::nullopt for an invalid bundle
	[[nodiscard]] std::optional<Veyon::Crypto::SecureBuffer> toJson() const;

	// the CA certificates re-encoded as PEM bundle (value for TLS/CaCertificate; never the
	// verbatim input, so nothing but certificates is imported)
	QString caCertificatesPem() const;

	[[nodiscard]] bool isValid() const;

	[[nodiscard]] static bool isValidEnrollmentUrl(const QString& url);
	// "https://<host>[:<port>]" rebuilt from the normalised host (TlsPeerVerifier::normalizedHost(),
	// IPv6 in brackets) and port; empty for anything but a plain https URL with host, optional port
	// 1..65535 and optional "/" path: no user info, query, fragment, percent-encoding, zone id,
	// trailing dot, non-canonical IPv4 form (e.g. "0x7f.1") or whitespace
	[[nodiscard]] static QString normalizedEnrollmentUrl(const QString& url);
	[[nodiscard]] static bool isValidEnrollmentToken(Veyon::Crypto::ByteView token);
};
