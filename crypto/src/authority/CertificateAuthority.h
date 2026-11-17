// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Certificate.h"
#include "CertificateRequest.h"
#include "PrivateKey.h"

namespace VeyonCrypto {

// Certificate issuance primitives of the CA host (06-pki.md §11.1 CA certificate, §11.2 issuance
// profile).
//
// CAPABILITY BOUNDARY (06-pki.md §1): this class lives in the separate static library
// veyon-crypto-authority. Only the CA-host authority code (PkiAuthorityApi) may link it; veyon-core
// and veyon-master/-server/-worker must not (checked at configure time in the top-level
// CMakeLists.txt).
//
// The CA key is passed per call so that callers can load it from the SecretStore for a single
// operation (06-pki.md §6); nothing here stores or returns key material. All names come from the
// caller's profile: subject, SANs and extensions of the request are never copied (06-pki.md §7).
class CertificateAuthority
{
public:
	static constexpr int DefaultCaValidityDays = 3650;
	static constexpr int MaximumCaValidityDays = 3653;
	static constexpr int DefaultEndEntityValidityDays = 730;
	static constexpr int MaximumEndEntityValidityDays = 1095;
	static constexpr qint64 NotBeforeBackdateSeconds = 5 * 60;
	static constexpr qsizetype SerialNumberSize = 16;
	static constexpr int MaximumSubjectAlternativeNames = 32;

	struct CaParameters
	{
		QString commonName; // e.g. "Veyon CA <name>" (06-pki.md §11.1)
		QString organization;
		int validityDays = DefaultCaValidityDays;
		QDateTime currentTime = {}; // invalid: current time
	};

	struct IssuanceProfile
	{
		QString commonName;
		QStringList dnsNames;			 // lower-case LDH names without trailing dot, no wildcards, no IP literals
		QList<QHostAddress> ipAddresses; // without scope id
		int validityDays = DefaultEndEntityValidityDays;
		QDateTime currentTime = {}; // invalid: current time
	};

	// self-signed CA certificate: EC P-256 key, basicConstraints critical CA:TRUE pathlen:0,
	// keyUsage critical keyCertSign+cRLSign, SKI, notBefore = now - 5 min
	[[nodiscard]] static std::optional<Certificate> createCaCertificate(const PrivateKey& caKey,
																		const CaParameters& parameters);

	// end-entity certificate for the key of a (self-signature verified) request: EC P-256 or RSA
	// key, basicConstraints critical CA:FALSE, keyUsage critical digitalSignature
	// (+keyEncipherment for RSA), extendedKeyUsage serverAuth, SAN from the profile (at least one
	// entry), SKI/AKI, random positive 128-bit serial, notBefore = now - 5 min,
	// notAfter = now + validityDays (must not exceed the CA certificate's notAfter)
	[[nodiscard]] static std::optional<Certificate> issue(const Certificate& caCertificate, const PrivateKey& caKey,
														  const CertificateRequest& request, const IssuanceProfile& profile);
};

} // namespace VeyonCrypto
