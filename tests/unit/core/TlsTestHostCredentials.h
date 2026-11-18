// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PkiTestCertificates.h"
#include "TlsHostCredentials.h"
#include "TlsServerPolicy.h"

// Host keys and certificates for the TLS server tests, generated at test time
namespace TlsTestHostCredentials {

inline Veyon::Crypto::PrivateKey generateKey(Veyon::Crypto::KeyAlgorithm algorithm = Veyon::Crypto::KeyAlgorithm::EcP256)
{
	return Veyon::Crypto::PrivateKey::generate(algorithm).value_or(Veyon::Crypto::PrivateKey{});
}

// self-signed host certificate (pins, 05-tls.md §5.1) or one signed by issuerKey
inline Veyon::Crypto::Certificate createCertificate(const Veyon::Crypto::PrivateKey& key, const QDateTime& now,
													qint64 notBeforeOffsetSeconds = -5 * 60,
													qint64 notAfterOffsetSeconds = 730LL * 24 * 60 * 60,
													const Veyon::Crypto::PrivateKey* issuerKey = nullptr,
													const QString& issuerCommonName = {})
{
	PkiTestCertificates::Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example,IP:127.0.0.1");
	specification.notBeforeOffsetSeconds = notBeforeOffsetSeconds;
	specification.notAfterOffsetSeconds = notAfterOffsetSeconds;
	return PkiTestCertificates::create(key, specification, issuerKey ? *issuerKey : key, issuerCommonName, now);
}

inline TlsHostCredentials::Encoded encode(const Veyon::Crypto::PrivateKey& key,
										  const QList<Veyon::Crypto::Certificate>& chain)
{
	QByteArray chainPem;
	for (const auto& certificate : chain)
	{
		chainPem += certificate.toPem();
	}
	return {key.toPkcs8Der().value_or(Veyon::Crypto::SecureBuffer{}), chainPem};
}

inline TlsServerPolicy policy(VeyonCore::TlsServerMode mode, const TlsHostCredentials* credentials)
{
	TlsServerPolicy policy;
	policy.mode = mode;
	if (credentials)
	{
		policy.certificateChain = credentials->certificateChain();
		policy.privateKey = credentials->privateKey();
	}
	return policy;
}

} // namespace TlsTestHostCredentials
