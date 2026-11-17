// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PrivateKey.h"

namespace VeyonCrypto {

// PKCS#10 certificate signing request.
//
// Parsing is meant for untrusted input: size-limited, whole input consumed and equal to OpenSSL's
// re-encoding (PEM content goes through fromDer()), canonical SubjectPublicKeyInfo (see
// PublicKey::fromDer()) and the one encoding of the signature AlgorithmIdentifier, which the
// signature does not cover. A request is only returned if its key is accepted by the VeyonCrypto
// key policy and its self-signature verifies (proof of possession). As for Certificate, this is not
// a full DER check (names and attributes are kept as read, ECDSA signatures are malleable): never
// use the DER encoding of a request as identity, only the SPKI pin of its publicKey(). Requested extensions and subject attributes other
// than the common name are ignored; an issuer must take names from its own sources, never from the
// request (06-pki.md §7).
class VEYON_CRYPTO_EXPORT CertificateRequest
{
public:
	static constexpr qsizetype MaximumEncodedSize = 16 * 1024;
	static constexpr qsizetype MaximumCommonNameSize = 64;

	CertificateRequest();
	~CertificateRequest();
	CertificateRequest(const CertificateRequest& other);
	CertificateRequest(CertificateRequest&& other) noexcept;
	CertificateRequest& operator=(const CertificateRequest& other);
	CertificateRequest& operator=(CertificateRequest&& other) noexcept;

	// subject CN=commonName (1..MaximumCommonNameSize characters), signed with key
	[[nodiscard]] static std::optional<CertificateRequest> create(const PrivateKey& key, const QString& commonName);

	[[nodiscard]] static std::optional<CertificateRequest> fromDer(ByteView der);
	// exactly one PEM "CERTIFICATE REQUEST" (or legacy "NEW CERTIFICATE REQUEST") block, otherwise
	// as Certificate::fromPem()
	[[nodiscard]] static std::optional<CertificateRequest> fromPem(ByteView pem);

	bool isNull() const;

	QByteArray toDer() const;
	QByteArray toPem() const;

	// display-safe RFC 2253 representation of the subject and its common name as described for
	// Certificate::subject()/subjectCommonName(); unauthenticated input: display/log only
	QString subject() const;
	QString subjectCommonName() const;

	PublicKey publicKey() const;

private:
	friend struct Internal::Access;
	class Private;
	explicit CertificateRequest(std::shared_ptr<const Private> d);

	std::shared_ptr<const Private> d;
};

} // namespace VeyonCrypto
