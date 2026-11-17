// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QRegularExpression>
#include <QtEndian>

#include "CertificateAuthority.h"
#include "CryptoPrivate.h"
#include "Random.h"

#include <algorithm>

namespace VeyonCrypto {

using namespace Internal;

static bool isValidDnsName(const QString& name)
{
	// lower-case LDH labels (RFC 1123), 1..63 characters per label, at most 253 characters, no
	// trailing dot, no wildcard; IP literals belong into ipAddresses
	// anchoredPattern(): "$" would also match before a trailing newline
	static const QRegularExpression labelPattern(
		QRegularExpression::anchoredPattern(QStringLiteral("[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?")));

	if (name.isEmpty() || name.size() > 253 || QHostAddress(name).isNull() == false)
	{
		return false;
	}

	const auto labels = name.split(QLatin1Char('.'));
	for (const auto& label : labels)
	{
		if (labelPattern.match(label).hasMatch() == false)
		{
			return false;
		}
	}

	return true;
}



static bool isValidIpAddress(const QHostAddress& address)
{
	return address.isNull() == false && address.scopeId().isEmpty() &&
		(address.protocol() == QAbstractSocket::IPv4Protocol || address.protocol() == QAbstractSocket::IPv6Protocol);
}



static GeneralNamesPtr createSubjectAlternativeNames(const QStringList& dnsNames, const QList<QHostAddress>& ipAddresses)
{
	GeneralNamesPtr names(GENERAL_NAMES_new());
	if (names == nullptr)
	{
		return {};
	}

	const auto append = [&names](int type, ASN1_STRING* value) {
		GeneralNamePtr name(GENERAL_NAME_new());
		if (name == nullptr || value == nullptr)
		{
			ASN1_STRING_free(value);
			return false;
		}
		GENERAL_NAME_set0_value(name.get(), type, value);
		if (sk_GENERAL_NAME_push(names.get(), name.get()) <= 0)
		{
			return false;
		}
		name.release();
		return true;
	};

	for (const auto& dnsName : dnsNames)
	{
		const auto ascii = dnsName.toLatin1();
		auto value = ASN1_IA5STRING_new();
		if (value && ASN1_STRING_set(value, ascii.constData(), ascii.size()) != 1)
		{
			ASN1_STRING_free(value);
			value = nullptr;
		}
		if (append(GEN_DNS, value) == false)
		{
			return {};
		}
	}

	for (const auto& ipAddress : ipAddresses)
	{
		QByteArray octets;
		if (ipAddress.protocol() == QAbstractSocket::IPv4Protocol)
		{
			octets.resize(4);
			qToBigEndian<quint32>(ipAddress.toIPv4Address(), octets.data());
		}
		else
		{
			const auto ipv6 = ipAddress.toIPv6Address();
			octets = QByteArray(reinterpret_cast<const char*>(&ipv6), sizeof(ipv6));
		}
		auto value = ASN1_OCTET_STRING_new();
		if (value && ASN1_OCTET_STRING_set(value, reinterpret_cast<const unsigned char*>(octets.constData()), octets.size()) != 1)
		{
			ASN1_STRING_free(value);
			value = nullptr;
		}
		if (append(GEN_IPADD, value) == false)
		{
			return {};
		}
	}

	return names;
}



static X509Ptr createCertificate(X509_NAME* subject, EVP_PKEY* subjectKey, const QDateTime& currentTime, int validityDays)
{
	const auto serialBytes = Random::bytes(CertificateAuthority::SerialNumberSize);
	if (serialBytes.has_value() == false)
	{
		return {};
	}

	// unsigned 128-bit random value: always positive, DER encoding at most 17 bytes (RFC 5280: <= 20)
	BigNumPtr serial(BN_bin2bn(serialBytes->bytes(), int(serialBytes->size()), nullptr));
	Asn1IntegerPtr serialInteger(serial && BN_is_zero(serial.get()) == false ? BN_to_ASN1_INTEGER(serial.get(), nullptr) : nullptr);

	const auto now = time_t(currentTime.toSecsSinceEpoch());
	Asn1TimePtr notBefore(ASN1_TIME_adj(nullptr, now, 0, -long(CertificateAuthority::NotBeforeBackdateSeconds)));
	Asn1TimePtr notAfter(ASN1_TIME_adj(nullptr, now, validityDays, 0));

	X509Ptr certificate(X509_new());
	if (serialInteger == nullptr || notBefore == nullptr || notAfter == nullptr || certificate == nullptr ||
		X509_set_version(certificate.get(), 2) != 1 ||
		X509_set_serialNumber(certificate.get(), serialInteger.get()) != 1 ||
		X509_set1_notBefore(certificate.get(), notBefore.get()) != 1 ||
		X509_set1_notAfter(certificate.get(), notAfter.get()) != 1 ||
		X509_set_subject_name(certificate.get(), subject) != 1 ||
		X509_set_pubkey(certificate.get(), subjectKey) != 1)
	{
		return {};
	}

	return certificate;
}



static bool addExtension(X509* certificate, X509* issuer, int nid, const char* value)
{
	// value is always one of the constant strings below, never caller-provided data
	X509V3_CTX context;
	X509V3_set_ctx_nodb(&context);
	X509V3_set_ctx(&context, issuer, certificate, nullptr, nullptr, 0);

	X509ExtensionPtr extension(X509V3_EXT_conf_nid(nullptr, &context, nid, value));
	return extension && X509_add_ext(certificate, extension.get(), -1) == 1;
}



static std::optional<Certificate> signCertificate(const char* operation, X509Ptr certificate, const PrivateKey& signingKey)
{
	EvpMdContextPtr context(EVP_MD_CTX_new());
	if (context == nullptr ||
		initializeSignatureContext(context.get(), Access::nativeHandle(signingKey), signingKey.algorithm(), true) == false ||
		X509_sign_ctx(certificate.get(), context.get()) <= 0)
	{
		setOpenSslError(operation);
		return std::nullopt;
	}

	// return exactly what a relying party will parse
	unsigned char* der = nullptr;
	const auto derSize = i2d_X509(certificate.get(), &der);
	if (derSize <= 0)
	{
		setOpenSslError(operation);
		return std::nullopt;
	}
	auto result = Certificate::fromDer(ByteView(reinterpret_cast<const char*>(der), derSize));
	OPENSSL_free(der);

	if (result.has_value() == false || result->isSignedBy(signingKey.publicKey()) == false)
	{
		setError(operation, "issued certificate failed self-check");
		return std::nullopt;
	}

	return result;
}



static QDateTime effectiveTime(const QDateTime& currentTime)
{
	return currentTime.isValid() ? currentTime : QDateTime::currentDateTimeUtc();
}



std::optional<Certificate> CertificateAuthority::createCaCertificate(const PrivateKey& caKey, const CaParameters& parameters)
{
	static constexpr auto Operation = "CertificateAuthority::createCaCertificate";

	if (caKey.algorithm() != KeyAlgorithm::EcP256)
	{
		setError(Operation, "CA key must be EC P-256");
		return std::nullopt;
	}
	if (parameters.validityDays < 1 || parameters.validityDays > MaximumCaValidityDays)
	{
		setError(Operation, "invalid validity");
		return std::nullopt;
	}

	const auto subject = createName(parameters.commonName, parameters.organization);
	if (subject == nullptr)
	{
		return std::nullopt;
	}

	auto certificate = createCertificate(subject.get(), Access::nativeHandle(caKey), effectiveTime(parameters.currentTime),
										 parameters.validityDays);
	if (certificate == nullptr ||
		X509_set_issuer_name(certificate.get(), subject.get()) != 1 ||
		addExtension(certificate.get(), certificate.get(), NID_basic_constraints, "critical,CA:TRUE,pathlen:0") == false ||
		addExtension(certificate.get(), certificate.get(), NID_key_usage, "critical,keyCertSign,cRLSign") == false ||
		addExtension(certificate.get(), certificate.get(), NID_subject_key_identifier, "hash") == false)
	{
		setOpenSslError(Operation);
		return std::nullopt;
	}

	return signCertificate(Operation, std::move(certificate), caKey);
}



std::optional<Certificate> CertificateAuthority::issue(const Certificate& caCertificate, const PrivateKey& caKey,
													   const CertificateRequest& request, const IssuanceProfile& profile)
{
	static constexpr auto Operation = "CertificateAuthority::issue";

	// CA: must be a CA certificate matching the EC P-256 CA key
	if (caKey.algorithm() != KeyAlgorithm::EcP256 || caCertificate.isCa() == false ||
		caCertificate.hasKeyUsage(Certificate::KeyUsage::KeyCertSign) == false ||
		caCertificate.publicKey().isNull() || caCertificate.publicKey() != caKey.publicKey())
	{
		setError(Operation, "CA certificate and key do not form a valid CA");
		return std::nullopt;
	}

	// request: re-validated from its encoding (key policy and self-signature); EC P-256 or RSA only
	const auto validatedRequest = CertificateRequest::fromDer(request.toDer());
	const auto subjectKeyAlgorithm = validatedRequest ? validatedRequest->publicKey().algorithm() : KeyAlgorithm::Invalid;
	if (subjectKeyAlgorithm != KeyAlgorithm::EcP256 && subjectKeyAlgorithm != KeyAlgorithm::Rsa)
	{
		setError(Operation, "invalid request or unsupported key type");
		return std::nullopt;
	}

	// profile
	const auto sanCount = profile.dnsNames.size() + profile.ipAddresses.size();
	if (sanCount < 1 || sanCount > MaximumSubjectAlternativeNames ||
		std::all_of(profile.dnsNames.begin(), profile.dnsNames.end(), isValidDnsName) == false ||
		std::all_of(profile.ipAddresses.begin(), profile.ipAddresses.end(), isValidIpAddress) == false)
	{
		setError(Operation, "invalid subject alternative names");
		return std::nullopt;
	}

	const auto now = effectiveTime(profile.currentTime);
	if (profile.validityDays < 1 || profile.validityDays > MaximumEndEntityValidityDays ||
		caCertificate.isValidAt(now) == false || now.addDays(profile.validityDays) > caCertificate.notAfter())
	{
		setError(Operation, "invalid validity or validity exceeds the CA certificate");
		return std::nullopt;
	}

	const auto subject = createName(profile.commonName);
	const auto subjectAlternativeNames = createSubjectAlternativeNames(profile.dnsNames, profile.ipAddresses);
	const auto caX509 = Access::nativeHandle(caCertificate);
	if (subject == nullptr || subjectAlternativeNames == nullptr)
	{
		setError(Operation, "invalid subject");
		return std::nullopt;
	}

	const auto keyUsage = subjectKeyAlgorithm == KeyAlgorithm::Rsa ? "critical,digitalSignature,keyEncipherment" : "critical,digitalSignature";

	auto certificate = createCertificate(subject.get(), Access::nativeHandle(validatedRequest->publicKey()), now,
										 profile.validityDays);
	if (certificate == nullptr ||
		X509_set_issuer_name(certificate.get(), X509_get_subject_name(caX509)) != 1 ||
		addExtension(certificate.get(), caX509, NID_basic_constraints, "critical,CA:FALSE") == false ||
		addExtension(certificate.get(), caX509, NID_key_usage, keyUsage) == false ||
		addExtension(certificate.get(), caX509, NID_ext_key_usage, "serverAuth") == false ||
		addExtension(certificate.get(), caX509, NID_subject_key_identifier, "hash") == false ||
		addExtension(certificate.get(), caX509, NID_authority_key_identifier, "keyid:always") == false ||
		X509_add1_ext_i2d(certificate.get(), NID_subject_alt_name, subjectAlternativeNames.get(), 0, X509V3_ADD_DEFAULT) != 1)
	{
		setOpenSslError(Operation);
		return std::nullopt;
	}

	return signCertificate(Operation, std::move(certificate), caKey);
}

} // namespace VeyonCrypto
