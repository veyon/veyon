// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDateTime>
#include <QStringList>

#include "Certificate.h"
#include "CryptoPrivate.h"
#include "PrivateKey.h"

#include <ctime>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>

// Test certificates for the PkiPublicApi tests, crafted with OpenSSL directly (test-only use), so
// that also certificates the CA of veyon-crypto-authority never issues (wildcards, missing EKU,
// unsupported critical extensions, intermediate CAs) can be created without linking the authority
// library (capability boundary, 06-pki.md §1).
namespace PkiTestCertificates {

struct Specification
{
	QString commonName = QStringLiteral("pc01.school.example");
	// OpenSSL configuration syntax, e.g. "DNS:pc01.school.example,IP:192.0.2.10"; empty: none
	QString subjectAltNames = {};
	bool isCa = false;
	int pathLength = -1;
	// empty: no extendedKeyUsage extension
	QString extendedKeyUsage = QStringLiteral("serverAuth");
	bool unsupportedCriticalExtension = false;
	qint64 notBeforeOffsetSeconds = -5 * 60;
	qint64 notAfterOffsetSeconds = 30 * 24 * 60 * 60;
	long serialNumber = 1;
};

inline bool addExtension(X509* certificate, int nid, const QString& value)
{
	X509V3_CTX context;
	X509V3_set_ctx_nodb(&context);
	X509V3_set_ctx(&context, nullptr, certificate, nullptr, nullptr, 0);
	const auto valueData = value.toLatin1();
	Veyon::Crypto::Internal::X509ExtensionPtr extension(X509V3_EXT_conf_nid(nullptr, &context, nid, valueData.constData()));
	return extension && X509_add_ext(certificate, extension.get(), -1) == 1;
}

// subject key signed by issuerKey (OpenSSL keys, also outside the VeyonCrypto key policy);
// issuerCommonName empty: self-issued
inline Veyon::Crypto::Certificate createWithNativeKeys(EVP_PKEY* subjectPublicKey, const Specification& specification,
													   EVP_PKEY* signingKey, const QString& issuerCommonName,
													   const QDateTime& currentTime)
{
	using namespace Veyon::Crypto::Internal;

	const auto base = time_t(currentTime.toSecsSinceEpoch());
	const auto subjectName = createName(specification.commonName);
	const auto issuerName = createName(issuerCommonName.isEmpty() ? specification.commonName : issuerCommonName);
	X509Ptr certificate(X509_new());
	Asn1TimePtr notBefore(ASN1_TIME_adj(nullptr, base, int(specification.notBeforeOffsetSeconds / 86400),
										long(specification.notBeforeOffsetSeconds % 86400)));
	Asn1TimePtr notAfter(ASN1_TIME_adj(nullptr, base, int(specification.notAfterOffsetSeconds / 86400),
									   long(specification.notAfterOffsetSeconds % 86400)));
	Asn1IntegerPtr serial(ASN1_INTEGER_new());

	if (subjectName == nullptr || issuerName == nullptr || certificate == nullptr || notBefore == nullptr ||
		notAfter == nullptr || serial == nullptr || subjectPublicKey == nullptr || signingKey == nullptr ||
		ASN1_INTEGER_set(serial.get(), specification.serialNumber) != 1 ||
		X509_set_version(certificate.get(), 2) != 1 ||
		X509_set_serialNumber(certificate.get(), serial.get()) != 1 ||
		X509_set1_notBefore(certificate.get(), notBefore.get()) != 1 ||
		X509_set1_notAfter(certificate.get(), notAfter.get()) != 1 ||
		X509_set_subject_name(certificate.get(), subjectName.get()) != 1 ||
		X509_set_issuer_name(certificate.get(), issuerName.get()) != 1 ||
		X509_set_pubkey(certificate.get(), subjectPublicKey) != 1)
	{
		return {};
	}

	auto extensionsAdded = true;
	if (specification.isCa)
	{
		auto basicConstraints = QStringLiteral("critical,CA:TRUE");
		if (specification.pathLength >= 0)
		{
			basicConstraints += QStringLiteral(",pathlen:%1").arg(specification.pathLength);
		}
		extensionsAdded = addExtension(certificate.get(), NID_basic_constraints, basicConstraints) &&
			addExtension(certificate.get(), NID_key_usage, QStringLiteral("critical,keyCertSign,cRLSign"));
	}
	else
	{
		extensionsAdded = addExtension(certificate.get(), NID_basic_constraints, QStringLiteral("critical,CA:FALSE")) &&
			addExtension(certificate.get(), NID_key_usage, QStringLiteral("critical,digitalSignature"));
	}
	if (specification.extendedKeyUsage.isEmpty() == false)
	{
		extensionsAdded = extensionsAdded &&
			addExtension(certificate.get(), NID_ext_key_usage, specification.extendedKeyUsage);
	}
	if (specification.subjectAltNames.isEmpty() == false)
	{
		extensionsAdded = extensionsAdded &&
			addExtension(certificate.get(), NID_subject_alt_name, specification.subjectAltNames);
	}
	if (specification.unsupportedCriticalExtension)
	{
		extensionsAdded = extensionsAdded &&
			addExtension(certificate.get(), NID_name_constraints,
						 QStringLiteral("critical,permitted;DNS:school.example"));
	}

	if (extensionsAdded == false || X509_sign(certificate.get(), signingKey, EVP_sha256()) <= 0)
	{
		return {};
	}

	unsigned char* der = nullptr;
	const auto size = i2d_X509(certificate.get(), &der);
	if (size <= 0)
	{
		return {};
	}
	const QByteArray encoded(reinterpret_cast<const char*>(der), size);
	OPENSSL_free(der);

	return Veyon::Crypto::Certificate::fromDer(encoded).value_or(Veyon::Crypto::Certificate{});
}

inline Veyon::Crypto::Certificate create(const Veyon::Crypto::PrivateKey& subjectKey, const Specification& specification,
										 const Veyon::Crypto::PrivateKey& issuerKey, const QString& issuerCommonName,
										 const QDateTime& currentTime)
{
	using Veyon::Crypto::Internal::Access;
	return createWithNativeKeys(Access::nativeHandle(subjectKey), specification, Access::nativeHandle(issuerKey),
								issuerCommonName, currentTime);
}

// RSA key below the VeyonCrypto minimum of 3072 bits
inline Veyon::Crypto::Internal::EvpPkeyPtr generateWeakRsaKey()
{
	using namespace Veyon::Crypto::Internal;
	EvpPkeyContextPtr context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
	EVP_PKEY* key = nullptr;
	if (context == nullptr || EVP_PKEY_keygen_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), 2048) <= 0 ||
		EVP_PKEY_keygen(context.get(), &key) <= 0)
	{
		return nullptr;
	}
	return EvpPkeyPtr(key);
}

inline Veyon::Crypto::PrivateKey generateKey()
{
	return Veyon::Crypto::PrivateKey::generate(Veyon::Crypto::KeyAlgorithm::EcP256).value_or(Veyon::Crypto::PrivateKey{});
}

// self-signed CA certificate
inline Veyon::Crypto::Certificate createCa(const Veyon::Crypto::PrivateKey& caKey, const QString& commonName,
										   const QDateTime& currentTime, int pathLength = 0)
{
	Specification specification;
	specification.commonName = commonName;
	specification.isCa = true;
	specification.pathLength = pathLength;
	specification.extendedKeyUsage.clear();
	specification.notAfterOffsetSeconds = 3650LL * 24 * 60 * 60;
	return create(caKey, specification, caKey, {}, currentTime);
}

} // namespace PkiTestCertificates
