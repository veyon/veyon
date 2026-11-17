// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtEndian>

#include "CryptoPrivate.h"
#include "Hash.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <openssl/pem.h>

namespace Veyon::Crypto {

Certificate::Certificate() = default;
Certificate::~Certificate() = default;
Certificate::Certificate(const Certificate& other) = default;
Certificate::Certificate(Certificate&& other) noexcept = default;
Certificate& Certificate::operator=(const Certificate& other) = default;
Certificate& Certificate::operator=(Certificate&& other) noexcept = default;



Certificate::Certificate(std::shared_ptr<const Private> d) :
	d(std::move(d))
{
}



std::optional<Certificate> Certificate::fromDer(ByteView der)
{
	static constexpr auto Operation = "Certificate::fromDer";

	if (der.isEmpty() || der.size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	auto input = der.bytes();
	Internal::X509Ptr certificate(d2i_X509(nullptr, &input, long(der.size())));
	if (certificate == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}
	if (input != der.bytes() + der.size())
	{
		Internal::setError(Operation, "trailing data");
		return std::nullopt;
	}

	// the re-encoding (also of the tbsCertificate, which OpenSSL otherwise keeps verbatim) must
	// reproduce the input; this rejects BER forms in the certificate structure, but not inside
	// names or extension values (see Certificate.h)
	unsigned char* canonical = nullptr;
	const auto canonicalSize = i2d_re_X509_tbs(certificate.get(), nullptr) > 0 ? i2d_X509(certificate.get(), &canonical) : 0;
	const auto isDer = canonicalSize == der.size() && memcmp(canonical, der.bytes(), size_t(canonicalSize)) == 0;
	OPENSSL_free(canonical);
	if (isDer == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "not DER-encoded");
		return std::nullopt;
	}

	return Internal::Access::toCertificate(Operation, std::move(certificate));
}



std::optional<Certificate> Certificate::fromPem(ByteView pem)
{
	auto chain = chainFromPem(pem);
	if (chain.has_value() == false)
	{
		return std::nullopt;
	}
	if (chain->size() != 1)
	{
		Internal::setError("Certificate::fromPem", "expected exactly one certificate");
		return std::nullopt;
	}

	return chain->takeFirst();
}



std::optional<QList<Certificate>> Certificate::chainFromPem(ByteView pem)
{
	static constexpr auto Operation = "Certificate::chainFromPem";

	if (pem.isEmpty() || pem.size() > MaximumPemSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	const auto bio = Internal::readOnlyBio(pem);
	if (bio == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	QList<Certificate> chain;
	while (true)
	{
		QByteArray der;
		const auto result = Internal::readPemBlock(bio.get(), "CERTIFICATE", MaximumEncodedSize, der);
		if (result == Internal::PemReadResult::EndOfInput && chain.isEmpty() == false)
		{
			break;
		}
		if (result != Internal::PemReadResult::Block)
		{
			Internal::setOpenSslError(Operation);
			Internal::setError(Operation, "invalid or missing CERTIFICATE block");
			return std::nullopt;
		}

		if (chain.size() >= MaximumChainLength)
		{
			Internal::setError(Operation, "too many certificates");
			return std::nullopt;
		}

		auto certificate = fromDer(der);
		if (certificate.has_value() == false)
		{
			return std::nullopt;
		}
		chain.append(std::move(*certificate));
	}

	return chain;
}



bool Certificate::isNull() const
{
	return d == nullptr;
}



QByteArray Certificate::toDer() const
{
	return d ? d->der : QByteArray{};
}



QByteArray Certificate::toPem() const
{
	if (d == nullptr)
	{
		return {};
	}

	Internal::BioPtr bio(BIO_new(BIO_s_mem()));
	if (bio == nullptr || PEM_write_bio_X509(bio.get(), d->certificate.get()) != 1)
	{
		Internal::setOpenSslError("Certificate::toPem");
		return {};
	}

	return Internal::bioContents(bio.get());
}



QSslCertificate Certificate::toQSslCertificate() const
{
	return d ? QSslCertificate(d->der, QSsl::Der) : QSslCertificate{};
}



QString Certificate::subject() const
{
	return d ? d->subject : QString{};
}



QString Certificate::issuer() const
{
	return d ? d->issuer : QString{};
}



QString Certificate::subjectCommonName() const
{
	return d ? d->subjectCommonName : QString{};
}



QByteArray Certificate::serialNumber() const
{
	return d ? d->serialNumber : QByteArray{};
}



QString Certificate::serialNumberHex() const
{
	if (d == nullptr)
	{
		return {};
	}

	return QString::fromLatin1(d->serialNumber.toHex());
}



QDateTime Certificate::notBefore() const
{
	return d ? d->notBefore : QDateTime{};
}



QDateTime Certificate::notAfter() const
{
	return d ? d->notAfter : QDateTime{};
}



bool Certificate::isValidAt(const QDateTime& time) const
{
	return d && time.isValid() && d->notBefore <= time && time <= d->notAfter;
}



QStringList Certificate::dnsNames() const
{
	return d ? d->dnsNames : QStringList{};
}



QList<QHostAddress> Certificate::ipAddresses() const
{
	return d ? d->ipAddresses : QList<QHostAddress>{};
}



bool Certificate::hasExtendedKeyUsageExtension() const
{
	return d && d->hasExtendedKeyUsageExtension;
}



bool Certificate::hasExtendedKeyUsage(ExtendedKeyUsage usage) const
{
	return d && d->extendedKeyUsages.contains(usage);
}



bool Certificate::hasKeyUsageExtension() const
{
	return d && d->hasKeyUsageExtension;
}



bool Certificate::hasKeyUsage(KeyUsage usage) const
{
	return d && d->keyUsages.contains(usage);
}



QStringList Certificate::criticalExtensions() const
{
	return d ? d->criticalExtensions : QStringList{};
}



bool Certificate::hasUnsupportedCriticalExtension() const
{
	return d == nullptr || d->hasUnsupportedCriticalExtension;
}



bool Certificate::isCa() const
{
	return d && d->isCa;
}



int Certificate::pathLengthConstraint() const
{
	return d ? d->pathLengthConstraint : -1;
}



PublicKey Certificate::publicKey() const
{
	return d ? d->publicKey : PublicKey{};
}



QByteArray Certificate::spkiSha256() const
{
	return d ? d->spkiSha256 : QByteArray{};
}



QString Certificate::spkiFingerprint() const
{
	return d ? QStringLiteral("sha256:") + QString::fromLatin1(d->spkiSha256.toHex()) : QString{};
}



bool Certificate::isSignedBy(const PublicKey& issuerKey) const
{
	static constexpr auto Operation = "Certificate::isSignedBy";

	const auto nativeKey = Internal::Access::nativeHandle(issuerKey);
	if (d == nullptr || nativeKey == nullptr)
	{
		Internal::setError(Operation, "null certificate or key");
		return false;
	}

	const X509_ALGOR* signatureAlgorithm = nullptr;
	X509_get0_signature(nullptr, &signatureAlgorithm, d->certificate.get());
	if (Internal::isAcceptedSignatureAlgorithm(signatureAlgorithm, issuerKey.algorithm()) == false)
	{
		Internal::setError(Operation, "unsupported signature algorithm");
		return false;
	}

	if (X509_verify(d->certificate.get(), nativeKey) != 1)
	{
		Internal::setOpenSslError(Operation);
		return false;
	}

	return true;
}



bool Certificate::operator==(const Certificate& other) const
{
	return d && other.d && d->der == other.d->der;
}



// rejects duplicated extensions and extensions that are present but cannot be decoded
template<typename T>
static bool decodeExtension(X509* certificate, int nid, T& output)
{
	const auto first = X509_get_ext_by_NID(certificate, nid, -1);
	if (first < 0)
	{
		return true;
	}
	if (X509_get_ext_by_NID(certificate, nid, first) >= 0)
	{
		return false;
	}

	int critical = -1;
	output.reset(static_cast<typename T::pointer>(X509_get_ext_d2i(certificate, nid, &critical, nullptr)));
	return output != nullptr;
}



static bool isRfc5280Time(const ASN1_TIME* time)
{
	// RFC 5280 §4.1.2.5: UTCTime YYMMDDHHMMSSZ or GeneralizedTime YYYYMMDDHHMMSSZ, nothing else
	// (OpenSSL's conversion also accepts missing seconds, offsets and fractions)
	const auto type = ASN1_STRING_type(time);
	const auto expectedSize = type == V_ASN1_UTCTIME ? 13 : type == V_ASN1_GENERALIZEDTIME ? 15 :
																							 0;
	const auto data = ASN1_STRING_get0_data(time);
	if (expectedSize == 0 || ASN1_STRING_length(time) != expectedSize || data[expectedSize - 1] != 'Z')
	{
		return false;
	}
	for (int i = 0; i < expectedSize - 1; ++i)
	{
		if (data[i] < '0' || data[i] > '9')
		{
			return false;
		}
	}
	return true;
}



static std::optional<QDateTime> toDateTime(const ASN1_TIME* time)
{
	if (time == nullptr || isRfc5280Time(time) == false)
	{
		return std::nullopt;
	}

	Internal::Asn1TimePtr epoch(ASN1_TIME_set(nullptr, 0));
	int days = 0;
	int seconds = 0;
	if (time == nullptr || epoch == nullptr || ASN1_TIME_diff(&days, &seconds, epoch.get(), time) != 1)
	{
		return std::nullopt;
	}

	return QDateTime::fromSecsSinceEpoch(qint64(days) * 24 * 60 * 60 + seconds).toUTC();
}



static bool readSubjectAlternativeNames(X509* certificate, QStringList& dnsNames, QList<QHostAddress>& ipAddresses)
{
	Internal::GeneralNamesPtr names;
	if (decodeExtension(certificate, NID_subject_alt_name, names) == false)
	{
		return false;
	}

	for (int i = 0; names && i < sk_GENERAL_NAME_num(names.get()); ++i)
	{
		// the value is only an ASN1_STRING for GEN_DNS/GEN_IPADD (other types: OTHERNAME, X509_NAME, ...)
		int type = -1;
		const auto untypedValue = GENERAL_NAME_get0_value(sk_GENERAL_NAME_value(names.get(), i), &type);
		if (untypedValue == nullptr)
		{
			return false;
		}
		if (type != GEN_DNS && type != GEN_IPADD)
		{
			continue;
		}
		const auto value = static_cast<const ASN1_STRING*>(untypedValue);
		const auto data = reinterpret_cast<const char*>(ASN1_STRING_get0_data(value));
		const auto size = ASN1_STRING_length(value);

		if (type == GEN_DNS)
		{
			// IA5String: reject NUL bytes (name truncation attacks), non-ASCII and empty names
			if (size <= 0 || size > 253)
			{
				return false;
			}
			for (int j = 0; j < size; ++j)
			{
				if (data[j] <= 0x20 || data[j] >= 0x7f)
				{
					return false;
				}
			}
			dnsNames.append(QString::fromLatin1(data, size));
		}
		else if (type == GEN_IPADD)
		{
			if (size == 4)
			{
				ipAddresses.append(QHostAddress(qFromBigEndian<quint32>(data)));
			}
			else if (size == 16)
			{
				ipAddresses.append(QHostAddress(reinterpret_cast<const quint8*>(data)));
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}



static void readCriticalExtensions(X509* certificate, QStringList& criticalExtensions, bool& hasUnsupported)
{
	// extensions this class models; everything else that is critical must make a verifier fail
	static constexpr int SupportedNids[] = {NID_basic_constraints, NID_key_usage, NID_ext_key_usage, NID_subject_alt_name};

	hasUnsupported = (X509_get_extension_flags(certificate) & EXFLAG_CRITICAL) != 0;
	for (int i = 0; i < X509_get_ext_count(certificate); ++i)
	{
		const auto extension = X509_get_ext(certificate, i);
		if (extension == nullptr || X509_EXTENSION_get_critical(extension) == 0)
		{
			continue;
		}
		const auto object = X509_EXTENSION_get_object(extension);
		char oid[128] = {};
		OBJ_obj2txt(oid, sizeof(oid), object, 1);
		criticalExtensions.append(QString::fromLatin1(oid));
		const auto nid = OBJ_obj2nid(object);
		hasUnsupported |= std::find(std::begin(SupportedNids), std::end(SupportedNids), nid) == std::end(SupportedNids);
	}
}



// DER encodes the critical BOOLEAN TRUE as 0xff, but OpenSSL keeps any other non-zero value as read
// (also when re-encoding), so check the encoding of each extension:
// SEQUENCE { extnID OBJECT IDENTIFIER, critical BOOLEAN DEFAULT FALSE, extnValue OCTET STRING }
static bool hasDerCriticalFlags(X509* certificate)
{
	const auto lengthSize = [](const unsigned char* length) { return *length < 0x80 ? 1 : 1 + (*length & 0x7f); };

	for (int i = 0; i < X509_get_ext_count(certificate); ++i)
	{
		unsigned char* der = nullptr;
		const auto size = i2d_X509_EXTENSION(X509_get_ext(certificate, i), &der);
		const auto objectOffset = size > 2 ? 1 + lengthSize(der + 1) : size;
		// object identifiers of extensions always have a short form length
		auto isDer = objectOffset + 2 <= size && der[objectOffset] == V_ASN1_OBJECT && der[objectOffset + 1] < 0x80;
		const auto criticalOffset = isDer ? objectOffset + 2 + der[objectOffset + 1] : size;
		if (isDer && criticalOffset < size && der[criticalOffset] == V_ASN1_BOOLEAN)
		{
			isDer = criticalOffset + 3 <= size && der[criticalOffset + 1] == 1 && der[criticalOffset + 2] == 0xff;
		}
		OPENSSL_free(der);
		if (isDer == false)
		{
			return false;
		}
	}
	return true;
}



struct CertificateUsage
{
	bool hasExtendedKeyUsageExtension = false;
	QList<Certificate::ExtendedKeyUsage> extendedKeyUsages;
	bool hasKeyUsageExtension = false;
	QList<Certificate::KeyUsage> keyUsages;
	bool isCa = false;
	int pathLengthConstraint = -1;
};



static bool readUsageExtensions(X509* certificate, CertificateUsage& d)
{
	Internal::ExtendedKeyUsagePtr extendedKeyUsage;
	Internal::BasicConstraintsPtr basicConstraints;
	std::unique_ptr<ASN1_BIT_STRING, Internal::OpenSslDeleter<ASN1_BIT_STRING_free>> keyUsage;
	if (decodeExtension(certificate, NID_ext_key_usage, extendedKeyUsage) == false ||
		decodeExtension(certificate, NID_basic_constraints, basicConstraints) == false ||
		decodeExtension(certificate, NID_key_usage, keyUsage) == false)
	{
		return false;
	}

	d.hasExtendedKeyUsageExtension = extendedKeyUsage != nullptr;
	for (int i = 0; extendedKeyUsage && i < sk_ASN1_OBJECT_num(extendedKeyUsage.get()); ++i)
	{
		switch (OBJ_obj2nid(sk_ASN1_OBJECT_value(extendedKeyUsage.get(), i)))
		{
		case NID_server_auth: d.extendedKeyUsages.append(Certificate::ExtendedKeyUsage::ServerAuth); break;
		case NID_client_auth: d.extendedKeyUsages.append(Certificate::ExtendedKeyUsage::ClientAuth); break;
		default: break;
		}
	}

	d.hasKeyUsageExtension = keyUsage != nullptr;
	if (keyUsage)
	{
		static constexpr std::pair<int, Certificate::KeyUsage> KeyUsageBits[] = {
			{0, Certificate::KeyUsage::DigitalSignature},
			{2, Certificate::KeyUsage::KeyEncipherment},
			{5, Certificate::KeyUsage::KeyCertSign},
			{6, Certificate::KeyUsage::CrlSign},
		};
		for (const auto& [bit, usage] : KeyUsageBits)
		{
			if (ASN1_BIT_STRING_get_bit(keyUsage.get(), bit))
			{
				d.keyUsages.append(usage);
			}
		}
	}

	if (basicConstraints)
	{
		d.isCa = basicConstraints->ca != 0;
		if (basicConstraints->pathlen)
		{
			int64_t pathLength = -1;
			if (ASN1_INTEGER_get_int64(&pathLength, basicConstraints->pathlen) != 1 ||
				pathLength < 0 || pathLength > 255)
			{
				return false;
			}
			d.pathLengthConstraint = int(pathLength);
		}
	}

	return true;
}



static bool readSerialNumber(X509* certificate, QByteArray& serialNumber)
{
	// RFC 5280 §4.1.2.2: positive, at most 20 content octets; zero is rejected as well since the
	// hex form is the key of TLS/RevokedSerials (06-pki.md §11.3)
	static constexpr int MaximumSerialNumberOctets = 20;

	Internal::BigNumPtr serial(ASN1_INTEGER_to_BN(X509_get0_serialNumber(certificate), nullptr));
	if (serial == nullptr || BN_is_zero(serial.get()) || BN_is_negative(serial.get()))
	{
		return false;
	}

	serialNumber = QByteArray(BN_num_bytes(serial.get()), 0);
	BN_bn2bin(serial.get(), reinterpret_cast<unsigned char*>(serialNumber.data()));

	// DER content octets: magnitude plus a leading zero octet if the most significant bit is set
	const auto contentOctets = serialNumber.size() + ((uchar(serialNumber.at(0)) & 0x80) ? 1 : 0);
	return contentOctets <= MaximumSerialNumberOctets;
}



std::optional<Certificate> Internal::Access::toCertificate(const char* operation, X509Ptr certificate)
{
	auto x509 = certificate.get();

	// let OpenSSL compute its extension cache: it flags invalid extensions (e.g. undecodable
	// basicConstraints/keyUsage, duplicate extensions in OpenSSL >= 3.0)
	X509_check_purpose(x509, -1, 0);
	if ((X509_get_extension_flags(x509) & EXFLAG_INVALID) || hasDerCriticalFlags(x509) == false)
	{
		setError(operation, "invalid extensions");
		return std::nullopt;
	}

	auto d = std::make_shared<Certificate::Private>();

	unsigned char* der = nullptr;
	const auto derSize = i2d_X509(x509, &der);
	unsigned char* spki = nullptr;
	const auto spkiSize = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(x509), &spki);
	if (derSize > 0 && spkiSize > 0)
	{
		d->der = QByteArray(reinterpret_cast<const char*>(der), derSize);
		const auto spkiDer = QByteArray(reinterpret_cast<const char*>(spki), spkiSize);
		// the pin is only unique for canonically encoded keys (see PublicKey::fromDer())
		const auto key = X509_get0_pubkey(x509);
		if (key == nullptr || isCanonicalSubjectPublicKeyInfo(key, spkiDer))
		{
			d->spkiSha256 = Hash::sha256(spkiDer).value_or(QByteArray{});
		}
		// null if the key does not satisfy the key policy
		d->publicKey = PublicKey::fromDer(spkiDer).value_or(PublicKey{});
	}
	OPENSSL_free(der);
	OPENSSL_free(spki);

	const auto notBefore = toDateTime(X509_get0_notBefore(x509));
	const auto notAfter = toDateTime(X509_get0_notAfter(x509));
	CertificateUsage usage;

	if (d->der.isEmpty() || d->spkiSha256.isEmpty() ||
		notBefore.has_value() == false || notAfter.has_value() == false ||
		readSerialNumber(x509, d->serialNumber) == false ||
		readSubjectAlternativeNames(x509, d->dnsNames, d->ipAddresses) == false ||
		readUsageExtensions(x509, usage) == false)
	{
		setOpenSslError(operation);
		setError(operation, "invalid or unsupported certificate content");
		return std::nullopt;
	}

	readCriticalExtensions(x509, d->criticalExtensions, d->hasUnsupportedCriticalExtension);
	d->hasExtendedKeyUsageExtension = usage.hasExtendedKeyUsageExtension;
	d->extendedKeyUsages = usage.extendedKeyUsages;
	d->hasKeyUsageExtension = usage.hasKeyUsageExtension;
	d->keyUsages = usage.keyUsages;
	d->isCa = usage.isCa;
	d->pathLengthConstraint = usage.pathLengthConstraint;
	d->notBefore = *notBefore;
	d->notAfter = *notAfter;
	d->subject = nameToString(X509_get_subject_name(x509));
	d->issuer = nameToString(X509_get_issuer_name(x509));
	d->subjectCommonName = commonNameOf(X509_get_subject_name(x509));
	d->certificate = std::move(certificate);

	// key policy failures of the embedded key are not certificate parse errors
	ERR_clear_error();

	return Certificate(std::move(d));
}

} // namespace Veyon::Crypto
