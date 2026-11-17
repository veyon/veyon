// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LegacyRsaPublicKey.h"
#include "OpenSslUtils.h"

#include <cstdint>
#include <cstring>
#include <openssl/asn1.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace Veyon::Crypto {

class LegacyRsaPublicKey::Private
{
public:
	Internal::EvpPkeyPtr key;
	QByteArray der;
};



namespace {

constexpr uint64_t MaximumPublicExponent = 0xffffffff;

struct Asn1SequenceDeleter
{
	void operator()(ASN1_SEQUENCE_ANY* sequence) const
	{
		sk_ASN1_TYPE_pop_free(sequence, ASN1_TYPE_free);
	}
};

using Asn1SequencePtr = std::unique_ptr<ASN1_SEQUENCE_ANY, Asn1SequenceDeleter>;



// positive odd modulus and odd public exponent 3..MaximumPublicExponent, read from the RSAPublicKey
// inside the (already canonical) SubjectPublicKeyInfo: works identically in OpenSSL 1.1.1, 3.x and
// 4.x without the low-level RSA API
bool hasAcceptedRsaParameters(ByteView subjectPublicKeyInfo)
{
	auto input = subjectPublicKeyInfo.bytes();
	Internal::X509PubkeyPtr publicKeyInfo(d2i_X509_PUBKEY(nullptr, &input, long(subjectPublicKeyInfo.size())));
	const unsigned char* rsaPublicKey = nullptr;
	int rsaPublicKeySize = 0;
	if (publicKeyInfo == nullptr ||
		X509_PUBKEY_get0_param(nullptr, &rsaPublicKey, &rsaPublicKeySize, nullptr, publicKeyInfo.get()) != 1 ||
		rsaPublicKey == nullptr || rsaPublicKeySize <= 0)
	{
		return false;
	}

	const auto rsaPublicKeyEnd = rsaPublicKey + rsaPublicKeySize;
	Asn1SequencePtr components(d2i_ASN1_SEQUENCE_ANY(nullptr, &rsaPublicKey, rsaPublicKeySize));
	if (components == nullptr || rsaPublicKey != rsaPublicKeyEnd || sk_ASN1_TYPE_num(components.get()) != 2)
	{
		return false;
	}

	const auto modulus = sk_ASN1_TYPE_value(components.get(), 0);
	const auto exponent = sk_ASN1_TYPE_value(components.get(), 1);
	if (ASN1_TYPE_get(modulus) != V_ASN1_INTEGER || ASN1_TYPE_get(exponent) != V_ASN1_INTEGER ||
		ASN1_STRING_type(modulus->value.integer) != V_ASN1_INTEGER)
	{
		return false;
	}

	// an even modulus is not a product of two odd primes
	const auto modulusData = ASN1_STRING_get0_data(modulus->value.integer);
	const auto modulusSize = ASN1_STRING_length(modulus->value.integer);
	if (modulusData == nullptr || modulusSize <= 0 || (modulusData[modulusSize - 1] & 1) == 0)
	{
		return false;
	}

	uint64_t exponentValue = 0;
	return ASN1_INTEGER_get_uint64(&exponentValue, exponent->value.integer) == 1 &&
		exponentValue >= 3 && exponentValue <= MaximumPublicExponent && (exponentValue & 1) == 1;
}



// RSAPublicKey (PKCS#1) DER to the SubjectPublicKeyInfo of the same key; the input must be DER
QByteArray subjectPublicKeyInfoFromPkcs1(const char* operation, ByteView rsaPublicKey)
{
	auto input = rsaPublicKey.bytes();
	Internal::EvpPkeyPtr key(d2i_PublicKey(EVP_PKEY_RSA, nullptr, &input, long(rsaPublicKey.size())));
	if (key == nullptr)
	{
		Internal::setOpenSslError(operation);
		return {};
	}

	unsigned char* reencoded = nullptr;
	const auto reencodedSize = i2d_PublicKey(key.get(), &reencoded);
	const auto isStrictDer = input == rsaPublicKey.bytes() + rsaPublicKey.size() &&
		reencodedSize == rsaPublicKey.size() &&
		std::memcmp(reencoded, rsaPublicKey.bytes(), size_t(reencodedSize)) == 0;
	OPENSSL_free(reencoded);
	if (isStrictDer == false)
	{
		Internal::setOpenSslError(operation);
		Internal::setError(operation, "invalid or non-DER encoding");
		return {};
	}

	return Internal::subjectPublicKeyInfo(key.get());
}

} // namespace



LegacyRsaPublicKey::LegacyRsaPublicKey() = default;
LegacyRsaPublicKey::~LegacyRsaPublicKey() = default;
LegacyRsaPublicKey::LegacyRsaPublicKey(const LegacyRsaPublicKey& other) = default;
LegacyRsaPublicKey::LegacyRsaPublicKey(LegacyRsaPublicKey&& other) noexcept = default;
LegacyRsaPublicKey& LegacyRsaPublicKey::operator=(const LegacyRsaPublicKey& other) = default;
LegacyRsaPublicKey& LegacyRsaPublicKey::operator=(LegacyRsaPublicKey&& other) noexcept = default;



LegacyRsaPublicKey::LegacyRsaPublicKey(std::shared_ptr<const Private> d) :
	d(std::move(d))
{
}



std::optional<LegacyRsaPublicKey> LegacyRsaPublicKey::fromDer(ByteView subjectPublicKeyInfo)
{
	static constexpr auto Operation = "LegacyRsaPublicKey::fromDer";

	if (subjectPublicKeyInfo.isEmpty() || subjectPublicKeyInfo.size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	// whole input consumed and re-encoding identical, also for the RSAPublicKey inside the BIT STRING
	auto input = subjectPublicKeyInfo.bytes();
	Internal::X509PubkeyPtr publicKeyInfo(d2i_X509_PUBKEY(nullptr, &input, long(subjectPublicKeyInfo.size())));
	unsigned char* reencoded = nullptr;
	const auto reencodedSize = publicKeyInfo ? i2d_X509_PUBKEY(publicKeyInfo.get(), &reencoded) : 0;
	const auto isStrictDer = input == subjectPublicKeyInfo.bytes() + subjectPublicKeyInfo.size() &&
		reencodedSize == subjectPublicKeyInfo.size() &&
		std::memcmp(reencoded, subjectPublicKeyInfo.bytes(), size_t(reencodedSize)) == 0;
	OPENSSL_free(reencoded);
	if (publicKeyInfo == nullptr || isStrictDer == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "invalid or non-DER encoding");
		return std::nullopt;
	}

	Internal::EvpPkeyPtr key(X509_PUBKEY_get(publicKeyInfo.get()));
	if (key == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	// EVP_PKEY_id(), not EVP_PKEY_base_id(): RSASSA-PSS-restricted keys are rejected
	if (EVP_PKEY_id(key.get()) != EVP_PKEY_RSA)
	{
		Internal::setError(Operation, "not an RSA key");
		return std::nullopt;
	}
	if (Internal::isCanonicalSubjectPublicKeyInfo(key.get(), subjectPublicKeyInfo) == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "non-DER key encoding");
		return std::nullopt;
	}
	const auto bits = EVP_PKEY_bits(key.get());
	if (bits < MinimumBits || bits > MaximumBits)
	{
		Internal::setError(Operation, "unsupported key size");
		return std::nullopt;
	}
	if (hasAcceptedRsaParameters(subjectPublicKeyInfo) == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "unsupported public exponent or even modulus");
		return std::nullopt;
	}

	auto d = std::make_shared<Private>();
	d->key = std::move(key);
	d->der = QByteArray(subjectPublicKeyInfo.data(), int(subjectPublicKeyInfo.size()));

	clearLastError();
	return LegacyRsaPublicKey(std::move(d));
}



std::optional<LegacyRsaPublicKey> LegacyRsaPublicKey::fromPem(ByteView pem)
{
	static constexpr auto Operation = "LegacyRsaPublicKey::fromPem";
	static constexpr char SubjectPublicKeyInfoLabel[] = "PUBLIC KEY";
	static constexpr char RsaPublicKeyLabel[] = "RSA PUBLIC KEY";

	if (pem.isEmpty() || pem.size() > MaximumPemSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	// exactly one block with one of the two labels (separate passes, each skipping other labels)
	QByteArray subjectPublicKeyInfoDer;
	QByteArray rsaPublicKeyDer;
	const auto subjectPublicKeyInfoBio = Internal::readOnlyBio(pem);
	const auto rsaPublicKeyBio = Internal::readOnlyBio(pem);
	const auto subjectPublicKeyInfoResult = subjectPublicKeyInfoBio ?
		Internal::readSinglePemBlock(subjectPublicKeyInfoBio.get(), SubjectPublicKeyInfoLabel, MaximumEncodedSize, subjectPublicKeyInfoDer) :
		Internal::PemReadResult::Error;
	const auto rsaPublicKeyResult = rsaPublicKeyBio ?
		Internal::readSinglePemBlock(rsaPublicKeyBio.get(), RsaPublicKeyLabel, MaximumEncodedSize, rsaPublicKeyDer) :
		Internal::PemReadResult::Error;

	if (subjectPublicKeyInfoResult == Internal::PemReadResult::Block &&
		rsaPublicKeyResult == Internal::PemReadResult::EndOfInput)
	{
		return fromDer(subjectPublicKeyInfoDer);
	}

	if (rsaPublicKeyResult == Internal::PemReadResult::Block &&
		subjectPublicKeyInfoResult == Internal::PemReadResult::EndOfInput)
	{
		const auto der = subjectPublicKeyInfoFromPkcs1(Operation, rsaPublicKeyDer);
		if (der.isEmpty())
		{
			return std::nullopt;
		}
		return fromDer(der);
	}

	Internal::setOpenSslError(Operation);
	Internal::setError(Operation, "invalid, missing or more than one public key block");
	return std::nullopt;
}



bool LegacyRsaPublicKey::isNull() const
{
	return d == nullptr;
}



int LegacyRsaPublicKey::bits() const
{
	return d ? EVP_PKEY_bits(d->key.get()) : 0;
}



QByteArray LegacyRsaPublicKey::toDer() const
{
	return d ? d->der : QByteArray{};
}



QByteArray LegacyRsaPublicKey::toPem() const
{
	if (d == nullptr)
	{
		return {};
	}

	Internal::BioPtr bio(BIO_new(BIO_s_mem()));
	if (bio == nullptr || PEM_write_bio_PUBKEY(bio.get(), d->key.get()) != 1)
	{
		Internal::setOpenSslError("LegacyRsaPublicKey::toPem");
		return {};
	}

	return Internal::bioContents(bio.get());
}



bool LegacyRsaPublicKey::verifyPkcs1Sha512(ByteView message, ByteView signature) const
{
	static constexpr auto Operation = "LegacyRsaPublicKey::verifyPkcs1Sha512";

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return false;
	}
	if (signature.size() != EVP_PKEY_size(d->key.get()))
	{
		Internal::setError(Operation, "invalid signature size");
		return false;
	}

	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	EVP_PKEY_CTX* keyContext = nullptr;
	if (context == nullptr ||
		EVP_DigestVerifyInit(context.get(), &keyContext, EVP_sha512(), nullptr, d->key.get()) != 1 ||
		keyContext == nullptr ||
		EVP_PKEY_CTX_set_rsa_padding(keyContext, RSA_PKCS1_PADDING) <= 0)
	{
		Internal::setOpenSslError(Operation);
		return false;
	}

	if (EVP_DigestVerify(context.get(), signature.bytes(), size_t(signature.size()),
						 message.bytes(), size_t(message.size())) != 1)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "invalid signature");
		return false;
	}

	clearLastError();
	return true;
}



std::optional<QByteArray> LegacyRsaPublicKey::encryptOaepSha1(ByteView plaintext) const
{
	static constexpr auto Operation = "LegacyRsaPublicKey::encryptOaepSha1";

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}
	if (plaintext.size() > maximumOaepSha1PlaintextSize())
	{
		Internal::setError(Operation, "plaintext too long");
		return std::nullopt;
	}

	Internal::EvpPkeyContextPtr context(EVP_PKEY_CTX_new(d->key.get(), nullptr));
	size_t ciphertextSize = 0;
	if (context == nullptr ||
		EVP_PKEY_encrypt_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_padding(context.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
		EVP_PKEY_CTX_set_rsa_oaep_md(context.get(), EVP_sha1()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_mgf1_md(context.get(), EVP_sha1()) <= 0 ||
		EVP_PKEY_encrypt(context.get(), nullptr, &ciphertextSize, plaintext.bytes(), size_t(plaintext.size())) <= 0 ||
		ciphertextSize != size_t(EVP_PKEY_size(d->key.get())))
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	QByteArray ciphertext(int(ciphertextSize), 0);
	if (EVP_PKEY_encrypt(context.get(), reinterpret_cast<unsigned char*>(ciphertext.data()), &ciphertextSize,
						 plaintext.bytes(), size_t(plaintext.size())) <= 0 ||
		ciphertextSize != size_t(ciphertext.size()))
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	clearLastError();
	return ciphertext;
}



qsizetype LegacyRsaPublicKey::maximumOaepSha1PlaintextSize() const
{
	return d ? qsizetype(EVP_PKEY_size(d->key.get())) - OaepSha1Overhead : 0;
}



bool LegacyRsaPublicKey::operator==(const LegacyRsaPublicKey& other) const
{
	return d && other.d && d->der == other.d->der;
}

} // namespace Veyon::Crypto
