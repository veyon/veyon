// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "OpenSslUtils.h"
#include "PrivateKey.h"

#include <cstring>
#include <limits>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <utility>

namespace Veyon::Crypto::Internal {

static constexpr int RsaPssSaltLength = 32;
static constexpr int MaximumNameAttributeSize = 64;



int noPasswordCallback(char* buffer, int size, int writing, void* userData)
{
	Q_UNUSED(buffer)
	Q_UNUSED(size)
	Q_UNUSED(writing)
	Q_UNUSED(userData)

	return -1;
}



bool fitsInt(qsizetype size)
{
	return size >= 0 && size <= std::numeric_limits<int>::max();
}



BioPtr readOnlyBio(ByteView data)
{
	if (fitsInt(data.size()) == false)
	{
		return {};
	}
	return BioPtr(BIO_new_mem_buf(data.data(), int(data.size())));
}



QByteArray bioContents(BIO* bio)
{
	char* data = nullptr;
	const auto size = BIO_get_mem_data(bio, &data);
	if (size <= 0 || data == nullptr)
	{
		return {};
	}
	return QByteArray(data, int(size));
}



// RFC 7468 §5.1 and §7: parsers should also accept the legacy labels
static bool hasLabel(const char* name, const char* label)
{
	return qstrcmp(name, label) == 0 ||
		(qstrcmp(label, PEM_STRING_X509) == 0 && qstrcmp(name, PEM_STRING_X509_OLD) == 0) ||
		(qstrcmp(label, PEM_STRING_X509_REQ) == 0 && qstrcmp(name, PEM_STRING_X509_REQ_OLD) == 0);
}



PemReadResult readPemBlock(BIO* bio, const char* label, qsizetype maximumSize, QByteArray& der)
{
	while (true)
	{
		char* name = nullptr;
		char* header = nullptr;
		unsigned char* data = nullptr;
		long size = 0;

		ERR_set_mark();
		if (PEM_read_bio_ex(bio, &name, &header, &data, &size, 0) != 1)
		{
			const auto error = ERR_peek_last_error();
			if (ERR_GET_LIB(error) == ERR_LIB_PEM && ERR_GET_REASON(error) == PEM_R_NO_START_LINE)
			{
				ERR_pop_to_mark();
				return PemReadResult::EndOfInput;
			}
			ERR_clear_last_mark();
			return PemReadResult::Error;
		}
		ERR_pop_to_mark();

		const auto isRequestedLabel = hasLabel(name, label);
		const auto hasHeader = header && header[0] != 0;
		const auto isValidSize = size > 0 && size <= maximumSize;
		if (isRequestedLabel && isValidSize && hasHeader == false)
		{
			der = QByteArray(reinterpret_cast<const char*>(data), int(size));
		}
		OPENSSL_free(name);
		OPENSSL_free(header);
		OPENSSL_free(data);

		if (isRequestedLabel)
		{
			return isValidSize && hasHeader == false ? PemReadResult::Block : PemReadResult::Error;
		}
	}
}



PemReadResult readSinglePemBlock(BIO* bio, const char* label, qsizetype maximumSize, QByteArray& der)
{
	const auto result = readPemBlock(bio, label, maximumSize, der);
	QByteArray furtherBlock;
	if (result == PemReadResult::Block && readPemBlock(bio, label, maximumSize, furtherBlock) != PemReadResult::EndOfInput)
	{
		return PemReadResult::Error;
	}
	return result;
}



QByteArray subjectPublicKeyInfo(EVP_PKEY* key)
{
	const auto size = key ? i2d_PUBKEY(key, nullptr) : 0;
	if (size <= 0)
	{
		return {};
	}

	QByteArray der(size, 0);
	auto output = reinterpret_cast<unsigned char*>(der.data());
	if (i2d_PUBKEY(key, &output) != size)
	{
		return {};
	}

	return der;
}



bool isCanonicalSubjectPublicKeyInfo(EVP_PKEY* key, ByteView subjectPublicKeyInfo)
{
	const auto canonical = Internal::subjectPublicKeyInfo(key);
	return canonical.isEmpty() == false &&
		canonical == QByteArray::fromRawData(subjectPublicKeyInfo.data(), int(subjectPublicKeyInfo.size()));
}



static bool isNamedCurveP256(EVP_PKEY* key)
{
	// determine the curve from the encoded AlgorithmIdentifier parameters: works identically in
	// OpenSSL 1.1.1, 3.x and 4.x without the low-level EC_KEY API, and rejects explicit curve
	// parameters (only a named-curve OID is accepted)
	const auto der = subjectPublicKeyInfo(key);
	auto input = reinterpret_cast<const unsigned char*>(der.constData());
	X509PubkeyPtr publicKey(der.isEmpty() ? nullptr : d2i_X509_PUBKEY(nullptr, &input, long(der.size())));
	X509_ALGOR* algorithm = nullptr;
	if (publicKey == nullptr ||
		X509_PUBKEY_get0_param(nullptr, nullptr, nullptr, &algorithm, publicKey.get()) != 1 ||
		algorithm == nullptr)
	{
		return false;
	}

	const ASN1_OBJECT* algorithmObject = nullptr;
	int parameterType = V_ASN1_UNDEF;
	const void* parameter = nullptr;
	X509_ALGOR_get0(&algorithmObject, &parameterType, &parameter, algorithm);

	return OBJ_obj2nid(algorithmObject) == NID_X9_62_id_ecPublicKey &&
		parameterType == V_ASN1_OBJECT &&
		OBJ_obj2nid(static_cast<const ASN1_OBJECT*>(parameter)) == NID_X9_62_prime256v1;
}



KeyAlgorithm classifyKey(EVP_PKEY* key)
{
	if (key == nullptr)
	{
		return KeyAlgorithm::Invalid;
	}

	// EVP_PKEY_id(), not EVP_PKEY_base_id(): RSASSA-PSS-restricted keys (EVP_PKEY_RSA_PSS) are
	// rejected along with all other key types
	switch (EVP_PKEY_id(key))
	{
	case EVP_PKEY_EC:
		return isNamedCurveP256(key) ? KeyAlgorithm::EcP256 : KeyAlgorithm::Invalid;
	case EVP_PKEY_ED25519:
		return KeyAlgorithm::Ed25519;
	case EVP_PKEY_RSA:
	{
		const auto bits = EVP_PKEY_bits(key);
		return bits >= PrivateKey::MinimumRsaBits && bits <= PrivateKey::MaximumRsaBits ? KeyAlgorithm::Rsa : KeyAlgorithm::Invalid;
	}
	default:
		break;
	}

	return KeyAlgorithm::Invalid;
}



bool initializeSignatureContext(EVP_MD_CTX* context, EVP_PKEY* key, KeyAlgorithm algorithm, bool forSigning)
{
	if (context == nullptr || key == nullptr || algorithm == KeyAlgorithm::Invalid)
	{
		return false;
	}

	// Ed25519 is used in its pure form without a separate digest
	const auto digest = algorithm == KeyAlgorithm::Ed25519 ? nullptr : EVP_sha256();

	EVP_PKEY_CTX* keyContext = nullptr;
	const auto initialized = forSigning ? EVP_DigestSignInit(context, &keyContext, digest, nullptr, key) : EVP_DigestVerifyInit(context, &keyContext, digest, nullptr, key);
	if (initialized != 1 || keyContext == nullptr)
	{
		return false;
	}

	if (algorithm == KeyAlgorithm::Rsa)
	{
		// exact salt length also when verifying (no RSA_PSS_SALTLEN_AUTO)
		return EVP_PKEY_CTX_set_rsa_padding(keyContext, RSA_PKCS1_PSS_PADDING) > 0 &&
			EVP_PKEY_CTX_set_rsa_pss_saltlen(keyContext, RsaPssSaltLength) > 0 &&
			EVP_PKEY_CTX_set_rsa_mgf1_md(keyContext, EVP_sha256()) > 0;
	}

	return true;
}



static bool isDisplaySafe(uint codePoint)
{
	// no control, format (e.g. bidi overrides, tag characters) or line/paragraph separator
	// characters and no unpaired surrogates
	switch (QChar::category(codePoint))
	{
	case QChar::Other_Control:
	case QChar::Other_Format:
	case QChar::Other_Surrogate:
	case QChar::Separator_Line:
	case QChar::Separator_Paragraph:
		return false;
	default:
		return true;
	}
}



// code point at position (a surrogate pair is one code point) and its number of UTF-16 units
static std::pair<uint, qsizetype> codePointAt(const QString& text, qsizetype position)
{
	const auto character = text.at(position);
	if (character.isHighSurrogate() && position + 1 < text.size() && text.at(position + 1).isLowSurrogate())
	{
		return {uint(QChar::surrogateToUcs4(character, text.at(position + 1))), 2};
	}
	return {uint(character.unicode()), 1};
}



static bool isDisplaySafe(const QString& text)
{
	for (qsizetype position = 0; position < text.size();)
	{
		const auto [codePoint, length] = codePointAt(text, position);
		if (isDisplaySafe(codePoint) == false)
		{
			return false;
		}
		position += length;
	}
	return true;
}



QString nameToString(const X509_NAME* name)
{
	BioPtr bio(BIO_new(BIO_s_mem()));
	// RFC 2253 with UTF-8 output instead of escaped multi-byte characters; OpenSSL escapes
	// backslashes in values, so the escapes added below cannot be forged
	if (name == nullptr || bio == nullptr ||
		X509_NAME_print_ex(bio.get(), name, 0, XN_FLAG_RFC2253 & ~ASN1_STRFLGS_ESC_MSB) < 0)
	{
		return {};
	}

	// display-safe: escape everything that could inject lines or reorder or hide text in logs and UIs
	const auto text = QString::fromUtf8(bioContents(bio.get()));
	QString safeText;
	safeText.reserve(text.size());
	for (qsizetype position = 0; position < text.size();)
	{
		const auto [codePoint, length] = codePointAt(text, position);
		if (isDisplaySafe(codePoint))
		{
			safeText.append(text.mid(position, length));
		}
		else if (codePoint > 0xffff)
		{
			safeText.append(QStringLiteral("\\U%1").arg(codePoint, 8, 16, QLatin1Char('0')));
		}
		else
		{
			safeText.append(QStringLiteral("\\u%1").arg(codePoint, 4, 16, QLatin1Char('0')));
		}
		position += length;
	}
	return safeText;
}



QString commonNameOf(const X509_NAME* name)
{
	QString commonName;
	int count = 0;
	const auto entryCount = name ? X509_NAME_entry_count(name) : 0;
	for (int i = 0; i < entryCount; ++i)
	{
		const auto entry = X509_NAME_get_entry(name, i);
		if (entry == nullptr || OBJ_obj2nid(X509_NAME_ENTRY_get_object(entry)) != NID_commonName)
		{
			continue;
		}

		++count;
		unsigned char* utf8 = nullptr;
		const auto size = ASN1_STRING_to_UTF8(&utf8, X509_NAME_ENTRY_get_data(entry));
		if (size >= 0 && utf8)
		{
			commonName = QString::fromUtf8(reinterpret_cast<const char*>(utf8), size);
		}
		OPENSSL_free(utf8);
	}

	if (count != 1 || commonName.size() > MaximumNameAttributeSize ||
		isDisplaySafe(commonName) == false)
	{
		return {};
	}
	return commonName;
}



static int algorithmNid(const X509_ALGOR* algorithm, int* parameterType = nullptr, const void** parameter = nullptr)
{
	const ASN1_OBJECT* object = nullptr;
	int type = V_ASN1_UNDEF;
	const void* value = nullptr;
	if (algorithm)
	{
		X509_ALGOR_get0(&object, &type, &value, algorithm);
	}
	if (parameterType)
	{
		*parameterType = type;
	}
	if (parameter)
	{
		*parameter = value;
	}
	return object ? OBJ_obj2nid(object) : NID_undef;
}



static bool isSha256AlgorithmIdentifier(const X509_ALGOR* algorithm)
{
	// parameters absent or NULL (RFC 5754)
	int parameterType = V_ASN1_UNDEF;
	return algorithmNid(algorithm, &parameterType) == NID_sha256 &&
		(parameterType == V_ASN1_UNDEF || parameterType == V_ASN1_NULL);
}



// OpenSSL keeps SEQUENCE parameters of AlgorithmIdentifiers as read, so check that the decoded
// structure re-encodes to exactly these bytes
template<typename Type, typename Encoder>
static bool isDerEncoded(Type* decoded, Encoder encode, const ASN1_STRING* encoding)
{
	unsigned char* der = nullptr;
	const auto size = decoded ? encode(decoded, &der) : 0;
	const auto isDer = size > 0 && size == ASN1_STRING_length(encoding) &&
		memcmp(der, ASN1_STRING_get0_data(encoding), size_t(size)) == 0;
	OPENSSL_free(der);
	return isDer;
}



static bool isAcceptedPssParameters(int parameterType, const void* parameter)
{
	// RSASSA-PSS-params (RFC 4055) in DER: defaults are SHA-1/MGF1-SHA1/salt 20, so every field
	// except the trailer field (whose DEFAULT value 1 DER omits) must be present and match the only
	// scheme VeyonCrypto signs with
	if (parameterType != V_ASN1_SEQUENCE || parameter == nullptr)
	{
		return false;
	}

	const auto sequence = static_cast<const ASN1_STRING*>(parameter);
	auto data = ASN1_STRING_get0_data(sequence);
	const auto end = data + ASN1_STRING_length(sequence);
	std::unique_ptr<RSA_PSS_PARAMS, OpenSslDeleter<RSA_PSS_PARAMS_free>> pss(
		d2i_RSA_PSS_PARAMS(nullptr, &data, long(end - data)));
	if (pss == nullptr || data != end || isDerEncoded(pss.get(), i2d_RSA_PSS_PARAMS, sequence) == false ||
		isSha256AlgorithmIdentifier(pss->hashAlgorithm) == false ||
		pss->saltLength == nullptr || ASN1_INTEGER_get(pss->saltLength) != RsaPssSaltLength ||
		pss->trailerField != nullptr)
	{
		return false;
	}

	// maskGenAlgorithm: id-mgf1 with a SHA-256 AlgorithmIdentifier as parameter
	int maskParameterType = V_ASN1_UNDEF;
	const void* maskParameter = nullptr;
	if (algorithmNid(pss->maskGenAlgorithm, &maskParameterType, &maskParameter) != NID_mgf1 ||
		maskParameterType != V_ASN1_SEQUENCE || maskParameter == nullptr)
	{
		return false;
	}
	const auto maskSequence = static_cast<const ASN1_STRING*>(maskParameter);
	auto maskData = ASN1_STRING_get0_data(maskSequence);
	const auto maskEnd = maskData + ASN1_STRING_length(maskSequence);
	std::unique_ptr<X509_ALGOR, OpenSslDeleter<X509_ALGOR_free>> maskHash(
		d2i_X509_ALGOR(nullptr, &maskData, long(maskEnd - maskData)));

	return maskHash && maskData == maskEnd && isDerEncoded(maskHash.get(), i2d_X509_ALGOR, maskSequence) &&
		isSha256AlgorithmIdentifier(maskHash.get());
}



bool isAcceptedSignatureAlgorithm(const X509_ALGOR* signatureAlgorithm, KeyAlgorithm issuerKeyAlgorithm)
{
	int parameterType = V_ASN1_UNDEF;
	const void* parameter = nullptr;
	const auto nid = algorithmNid(signatureAlgorithm, &parameterType, &parameter);

	// parameters as required by RFC 5758 §3.2, RFC 8410 §3 and RFC 4055 §5, i.e. exactly one
	// encoding per scheme (for requests, the AlgorithmIdentifier is not covered by the signature)
	switch (issuerKeyAlgorithm)
	{
	case KeyAlgorithm::EcP256: return nid == NID_ecdsa_with_SHA256 && parameterType == V_ASN1_UNDEF;
	case KeyAlgorithm::Ed25519: return nid == NID_ED25519 && parameterType == V_ASN1_UNDEF;
	case KeyAlgorithm::Rsa:
		return (nid == NID_sha256WithRSAEncryption && parameterType == V_ASN1_NULL) ||
			(nid == NID_rsassaPss && isAcceptedPssParameters(parameterType, parameter));
	case KeyAlgorithm::Invalid: break;
	}
	return false;
}



static bool isValidNameAttribute(const QString& value, bool required)
{
	if (value.isEmpty())
	{
		return required == false;
	}
	if (value.size() > MaximumNameAttributeSize)
	{
		return false;
	}
	return isDisplaySafe(value);
}



X509NamePtr createName(const QString& commonName, const QString& organization)
{
	if (isValidNameAttribute(commonName, true) == false || isValidNameAttribute(organization, false) == false)
	{
		setError("createName", "invalid name attribute");
		return {};
	}

	X509NamePtr name(X509_NAME_new());
	const auto commonNameUtf8 = commonName.toUtf8();
	const auto organizationUtf8 = organization.toUtf8();
	if (name == nullptr ||
		X509_NAME_add_entry_by_NID(name.get(), NID_commonName, MBSTRING_UTF8,
								   reinterpret_cast<const unsigned char*>(commonNameUtf8.constData()),
								   commonNameUtf8.size(), -1, 0) != 1 ||
		(organization.isEmpty() == false &&
		 X509_NAME_add_entry_by_NID(name.get(), NID_organizationName, MBSTRING_UTF8,
									reinterpret_cast<const unsigned char*>(organizationUtf8.constData()),
									organizationUtf8.size(), -1, 0) != 1))
	{
		setOpenSslError("createName");
		return {};
	}

	return name;
}

} // namespace Veyon::Crypto::Internal
