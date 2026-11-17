// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Certificate.h"
#include "CertificateRequest.h"
#include "OpenSslUtils.h"
#include "PrivateKey.h"

// Internal header of the VeyonCrypto layer: private data of the public classes and the single
// friend (Internal::Access) through which VeyonCrypto sources and the veyon-crypto-authority
// library reach the native OpenSSL objects. Never include it from a public header.

namespace Veyon::Crypto {

class PublicKey::Private
{
public:
	Internal::EvpPkeyPtr key;
	KeyAlgorithm algorithm = KeyAlgorithm::Invalid;
	QByteArray der;
};



class PrivateKey::Private
{
public:
	Internal::EvpPkeyPtr key;
	KeyAlgorithm algorithm = KeyAlgorithm::Invalid;
};



class CertificateRequest::Private
{
public:
	Internal::X509RequestPtr request;
	QByteArray der;
	PublicKey publicKey;
};



class Certificate::Private
{
public:
	Internal::X509Ptr certificate;
	QByteArray der;
	QByteArray spkiSha256;
	QString subject;
	QString issuer;
	QString subjectCommonName;
	QByteArray serialNumber;
	QDateTime notBefore;
	QDateTime notAfter;
	QStringList dnsNames;
	QList<QHostAddress> ipAddresses;
	bool hasExtendedKeyUsageExtension = false;
	QList<ExtendedKeyUsage> extendedKeyUsages;
	bool hasKeyUsageExtension = false;
	QList<KeyUsage> keyUsages;
	QStringList criticalExtensions;
	bool hasUnsupportedCriticalExtension = false;
	bool isCa = false;
	int pathLengthConstraint = -1;
	PublicKey publicKey;
};



namespace Internal {

struct VEYON_CRYPTO_EXPORT Access
{
	// native handles for OpenSSL functions that need them; the objects may be shared between
	// threads and copies, so they must never be modified
	static EVP_PKEY* nativeHandle(const PublicKey& key)
	{
		return key.d ? key.d->key.get() : nullptr;
	}

	static EVP_PKEY* nativeHandle(const PrivateKey& key)
	{
		return key.d ? key.d->key.get() : nullptr;
	}

	static X509* nativeHandle(const Certificate& certificate)
	{
		return certificate.d ? certificate.d->certificate.get() : nullptr;
	}

	static X509_REQ* nativeHandle(const CertificateRequest& request)
	{
		return request.d ? request.d->request.get() : nullptr;
	}

	// take ownership of a native object after applying the same policy and validation as the
	// respective fromDer() function
	// subjectPublicKeyInfo: the encoding the key was read from (empty: OpenSSL's encoding of key)
	[[nodiscard]] static std::optional<PublicKey> toPublicKey(const char* operation, EvpPkeyPtr key,
															  QByteArray subjectPublicKeyInfo = {});
	[[nodiscard]] static std::optional<PrivateKey> toPrivateKey(const char* operation, EvpPkeyPtr key);
	[[nodiscard]] static std::optional<Certificate> toCertificate(const char* operation, X509Ptr certificate);
	[[nodiscard]] static std::optional<CertificateRequest> toCertificateRequest(const char* operation, X509RequestPtr request);
};

} // namespace Internal

} // namespace Veyon::Crypto
