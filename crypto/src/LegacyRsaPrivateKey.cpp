// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LegacyRsaPrivateKey.h"
#include "OpenSslUtils.h"

#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace Veyon::Crypto {

class LegacyRsaPrivateKey::Private
{
public:
	Internal::EvpPkeyPtr key;
	LegacyRsaPublicKey publicKey;
};



namespace {

constexpr char Pkcs8Label[] = "PRIVATE KEY";
constexpr char Pkcs1Label[] = "RSA PRIVATE KEY";



// DER from a PEM block, kept in the secure heap and zeroised on release
class SecurePemData
{
public:
	SecurePemData() = default;
	~SecurePemData()
	{
		OPENSSL_free(m_name);
		OPENSSL_secure_clear_free(m_data, size_t(m_size));
	}

	SecurePemData(const SecurePemData&) = delete;
	SecurePemData& operator=(const SecurePemData&) = delete;

	// next block with any private key label (RFC 1421 encrypted blocks fail: no password is given)
	[[nodiscard]] bool read(BIO* bio)
	{
		return PEM_bytes_read_bio_secmem(&m_data, &m_size, &m_name, PEM_STRING_EVP_PKEY, bio,
										 Internal::noPasswordCallback, nullptr) == 1;
	}

	const char* name() const
	{
		return m_name;
	}

	ByteView der() const
	{
		return ByteView(reinterpret_cast<const char*>(m_data), qsizetype(m_size));
	}

private:
	char* m_name = nullptr;
	unsigned char* m_data = nullptr;
	long m_size = 0;
};



// whether the BIO contains no further private key block
bool hasNoFurtherPrivateKeyBlock(BIO* bio)
{
	ERR_set_mark();
	SecurePemData furtherBlock;
	if (furtherBlock.read(bio))
	{
		ERR_pop_to_mark();
		return false;
	}
	const auto error = ERR_peek_last_error();
	const auto isEndOfInput = ERR_GET_LIB(error) == ERR_LIB_PEM && ERR_GET_REASON(error) == PEM_R_NO_START_LINE;
	ERR_pop_to_mark();
	return isEndOfInput;
}



// RSAPrivateKey (PKCS#1) DER of a key, encoded directly into secure memory (OpenSSL writes the
// two-prime form with version 0, or version 1 with otherPrimeInfos for multi-prime keys)
std::optional<SecureBuffer> privateKeyToPkcs1Der(const char* operation, EVP_PKEY* key)
{
	const auto size = i2d_PrivateKey(key, nullptr);
	SecureBuffer der(size > 0 ? size : 0);
	auto output = der.bytes();
	if (size <= 0 || der.size() != size || i2d_PrivateKey(key, &output) != size)
	{
		Internal::setOpenSslError(operation);
		return std::nullopt;
	}
	return der;
}



// whether input is exactly the canonical encoding (both are not secret-dependent in length)
bool isCanonicalEncoding(ByteView input, const std::optional<SecureBuffer>& canonical)
{
	return canonical.has_value() && canonical->size() == input.size() &&
		CRYPTO_memcmp(canonical->constData(), input.data(), size_t(input.size())) == 0;
}



Internal::EvpPkeyPtr decodePkcs8(const char* operation, ByteView der)
{
	// PKCS8_PRIV_KEY_INFO_free() zeroises the decoded key octets
	auto input = der.bytes();
	Internal::Pkcs8Ptr keyInfo(d2i_PKCS8_PRIV_KEY_INFO(nullptr, &input, long(der.size())));
	if (keyInfo == nullptr || input != der.bytes() + der.size())
	{
		Internal::setOpenSslError(operation);
		Internal::setError(operation, "invalid PKCS#8 encoding or trailing data");
		return {};
	}

	Internal::EvpPkeyPtr key(EVP_PKCS82PKEY(keyInfo.get()));
	if (key == nullptr)
	{
		Internal::setOpenSslError(operation);
		return {};
	}

	// the decoders accept BER, attributes, version 1 with a public key, missing NULL parameters and
	// trailing data inside the OCTET STRING, so require the input to be exactly the encoding 4.x
	// (and OpenSSL) writes for the decoded key (re-encoded in secure memory)
	if (isCanonicalEncoding(der, Internal::privateKeyToPkcs8Der(operation, key.get())) == false)
	{
		Internal::setOpenSslError(operation);
		Internal::setError(operation, "not a DER-encoded PKCS#8 PrivateKeyInfo as written by Veyon 4.x");
		return {};
	}

	return key;
}



Internal::EvpPkeyPtr decodePkcs1(const char* operation, ByteView der)
{
	auto input = der.bytes();
	Internal::EvpPkeyPtr key(d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &input, long(der.size())));
	if (key == nullptr || input != der.bytes() + der.size())
	{
		Internal::setOpenSslError(operation);
		Internal::setError(operation, "invalid PKCS#1 encoding or trailing data");
		return {};
	}

	// OpenSSL 3.x also decodes other structures (e.g. PKCS#8) here, so require that the input is
	// exactly the RSAPrivateKey encoding of the decoded key (re-encoded in secure memory)
	if (isCanonicalEncoding(der, privateKeyToPkcs1Der(operation, key.get())) == false)
	{
		Internal::setOpenSslError(operation);
		Internal::setError(operation, "not a DER-encoded PKCS#1 RSAPrivateKey");
		return {};
	}

	return key;
}



// Minimal reader for the canonical RSAPrivateKey written by OpenSSL: returns the content of the
// next TLV with the given tag (definite lengths only) and advances offset; nothing is copied.
bool readDerElement(ByteView der, qsizetype& offset, unsigned char tag, ByteView& content)
{
	static constexpr int MaximumLengthOctets = 3;

	if (offset + 2 > der.size() || der.bytes()[offset] != tag)
	{
		return false;
	}
	qsizetype length = der.bytes()[offset + 1];
	offset += 2;
	if (length & 0x80)
	{
		const auto lengthOctets = int(length & 0x7f);
		if (lengthOctets < 1 || lengthOctets > MaximumLengthOctets || offset + lengthOctets > der.size())
		{
			return false;
		}
		length = 0;
		for (int i = 0; i < lengthOctets; ++i)
		{
			length = (length << 8) | der.bytes()[offset++];
		}
	}
	if (length > der.size() - offset)
	{
		return false;
	}
	content = ByteView(der.data() + offset, length);
	offset += length;
	return true;
}



// Checks the components of a two-prime RSAPrivateKey { version 0, n, e, d, p, q, dP, dQ, qInv }
// against each other with big number arithmetic in the secure heap (cheap, unlike primality tests,
// which are not done): n = p * q, d * e = 1 mod (p - 1) and mod (q - 1) (i.e. mod lcm(p - 1, q - 1)),
// dP = d mod (p - 1), dQ = d mod (q - 1), qInv * q = 1 mod p with 0 < qInv < p and p, q > 1.
// Multi-prime keys (version 1) are rejected.
bool hasConsistentTwoPrimeComponents(ByteView rsaPrivateKey)
{
	enum Component {
		Version,
		Modulus,
		PublicExponent,
		PrivateExponent,
		Prime1,
		Prime2,
		Exponent1,
		Exponent2,
		Coefficient,
		ComponentCount,
	};

	qsizetype offset = 0;
	ByteView sequence;
	if (readDerElement(rsaPrivateKey, offset, V_ASN1_SEQUENCE | V_ASN1_CONSTRUCTED, sequence) == false ||
		offset != rsaPrivateKey.size())
	{
		return false;
	}

	std::unique_ptr<BN_CTX, Internal::OpenSslDeleter<BN_CTX_free>> context(BN_CTX_secure_new());
	Internal::BigNumPtr values[ComponentCount];
	offset = 0;
	for (auto& value : values)
	{
		ByteView integer;
		// positive integers only (OpenSSL writes minimal two's complement encodings)
		if (readDerElement(sequence, offset, V_ASN1_INTEGER, integer) == false || integer.isEmpty() ||
			(integer.bytes()[0] & 0x80) != 0)
		{
			return false;
		}
		value.reset(BN_secure_new());
		if (value == nullptr || BN_bin2bn(integer.bytes(), int(integer.size()), value.get()) == nullptr)
		{
			return false;
		}
	}
	if (offset != sequence.size() || BN_is_zero(values[Version].get()) == false || context == nullptr)
	{
		return false;
	}

	const auto n = values[Modulus].get();
	const auto e = values[PublicExponent].get();
	const auto d = values[PrivateExponent].get();
	const auto p = values[Prime1].get();
	const auto q = values[Prime2].get();

	BN_CTX_start(context.get());
	const auto product = BN_CTX_get(context.get());
	const auto pMinusOne = BN_CTX_get(context.get());
	const auto qMinusOne = BN_CTX_get(context.get());
	const auto result = BN_CTX_get(context.get());
	const auto isConsistent =
		result != nullptr &&
		BN_cmp(p, BN_value_one()) > 0 && BN_cmp(q, BN_value_one()) > 0 &&
		BN_mul(product, p, q, context.get()) == 1 && BN_cmp(product, n) == 0 &&
		BN_sub(pMinusOne, p, BN_value_one()) == 1 && BN_sub(qMinusOne, q, BN_value_one()) == 1 &&
		BN_mod_mul(result, d, e, pMinusOne, context.get()) == 1 && BN_is_one(result) &&
		BN_mod_mul(result, d, e, qMinusOne, context.get()) == 1 && BN_is_one(result) &&
		BN_nnmod(result, d, pMinusOne, context.get()) == 1 && BN_cmp(result, values[Exponent1].get()) == 0 &&
		BN_nnmod(result, d, qMinusOne, context.get()) == 1 && BN_cmp(result, values[Exponent2].get()) == 0 &&
		BN_is_zero(values[Coefficient].get()) == false && BN_cmp(values[Coefficient].get(), p) < 0 &&
		BN_mod_mul(result, values[Coefficient].get(), q, p, context.get()) == 1 && BN_is_one(result);
	BN_CTX_end(context.get());

	return isConsistent;
}



std::optional<QByteArray> createPkcs1Sha512Signature(const char* operation, EVP_PKEY* key, ByteView message)
{
	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	EVP_PKEY_CTX* keyContext = nullptr;
	size_t signatureSize = 0;
	if (context == nullptr ||
		EVP_DigestSignInit(context.get(), &keyContext, EVP_sha512(), nullptr, key) != 1 ||
		keyContext == nullptr ||
		EVP_PKEY_CTX_set_rsa_padding(keyContext, RSA_PKCS1_PADDING) <= 0 ||
		EVP_DigestSign(context.get(), nullptr, &signatureSize, message.bytes(), size_t(message.size())) != 1 ||
		signatureSize != size_t(EVP_PKEY_size(key)))
	{
		Internal::setOpenSslError(operation);
		return std::nullopt;
	}

	QByteArray signature(int(signatureSize), 0);
	if (EVP_DigestSign(context.get(), reinterpret_cast<unsigned char*>(signature.data()), &signatureSize,
					   message.bytes(), size_t(message.size())) != 1 ||
		signatureSize != size_t(signature.size()))
	{
		Internal::setOpenSslError(operation);
		return std::nullopt;
	}

	return signature;
}

} // namespace



LegacyRsaPrivateKey::LegacyRsaPrivateKey() = default;
LegacyRsaPrivateKey::~LegacyRsaPrivateKey() = default;
LegacyRsaPrivateKey::LegacyRsaPrivateKey(LegacyRsaPrivateKey&& other) noexcept = default;
LegacyRsaPrivateKey& LegacyRsaPrivateKey::operator=(LegacyRsaPrivateKey&& other) noexcept = default;



std::optional<LegacyRsaPrivateKey> LegacyRsaPrivateKey::generate()
{
	static constexpr auto Operation = "LegacyRsaPrivateKey::generate";

	// OpenSSL's default public exponent is 65537 (RSA_F4) in all supported versions, as in 4.x
	Internal::EvpPkeyContextPtr context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
	EVP_PKEY* key = nullptr;
	if (context == nullptr ||
		EVP_PKEY_keygen_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), GeneratedKeyBits) <= 0 ||
		EVP_PKEY_keygen(context.get(), &key) <= 0)
	{
		EVP_PKEY_free(key);
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return fromNativeKey(Operation, key);
}



std::optional<LegacyRsaPrivateKey> LegacyRsaPrivateKey::fromPem(ByteView pem)
{
	static constexpr auto Operation = "LegacyRsaPrivateKey::fromPem";

	if (pem.isEmpty() || pem.size() > MaximumPemSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	const auto bio = Internal::readOnlyBio(pem);
	SecurePemData block;
	if (bio == nullptr || block.read(bio.get()) == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "invalid, encrypted or missing private key block");
		return std::nullopt;
	}
	if (hasNoFurtherPrivateKeyBlock(bio.get()) == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "more than one private key block");
		return std::nullopt;
	}
	if (block.der().isEmpty() || block.der().size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	Internal::EvpPkeyPtr key;
	if (qstrcmp(block.name(), Pkcs8Label) == 0)
	{
		key = decodePkcs8(Operation, block.der());
	}
	else if (qstrcmp(block.name(), Pkcs1Label) == 0)
	{
		key = decodePkcs1(Operation, block.der());
	}
	else
	{
		Internal::setError(Operation, "unsupported private key block (encrypted or not RSA)");
		return std::nullopt;
	}

	if (key == nullptr)
	{
		return std::nullopt;
	}

	return fromNativeKey(Operation, key.release());
}



std::optional<LegacyRsaPrivateKey> LegacyRsaPrivateKey::fromNativeKey(const char* operation, evp_pkey_st* nativeKey)
{
	Internal::EvpPkeyPtr key(nativeKey);

	// EVP_PKEY_id(), not EVP_PKEY_base_id(): RSASSA-PSS-restricted keys are rejected
	if (key == nullptr || EVP_PKEY_id(key.get()) != EVP_PKEY_RSA)
	{
		Internal::setError(operation, "not an RSA key");
		return std::nullopt;
	}

	// key size and public exponent policy
	auto publicKey = LegacyRsaPublicKey::fromDer(Internal::subjectPublicKeyInfo(key.get()));
	if (publicKey.has_value() == false)
	{
		Internal::setError(operation, "unsupported key size, public exponent or modulus");
		return std::nullopt;
	}

	// consistency of the components (cheap, before any private key operation, so that a bogus key
	// cannot cost more than a few modular multiplications), then a pairwise check: a signature made
	// with the private key must verify with the separately decoded public key (catches damaged key
	// files; EVP_PKEY_check() would also test the primes, which takes about 300 ms for a 4096-bit
	// key and would make every import a DoS lever)
	const auto rsaPrivateKey = privateKeyToPkcs1Der(operation, key.get());
	if (rsaPrivateKey.has_value() == false || hasConsistentTwoPrimeComponents(*rsaPrivateKey) == false)
	{
		Internal::setError(operation, "inconsistent or multi-prime key");
		return std::nullopt;
	}

	static constexpr char ConsistencyCheckMessage[] = "Veyon legacy RSA key consistency check";
	const ByteView checkMessage(ConsistencyCheckMessage, qsizetype(sizeof(ConsistencyCheckMessage) - 1));
	const auto checkSignature = createPkcs1Sha512Signature(operation, key.get(), checkMessage);
	if (checkSignature.has_value() == false || publicKey->verifyPkcs1Sha512(checkMessage, *checkSignature) == false)
	{
		Internal::setError(operation, "inconsistent key");
		return std::nullopt;
	}

	LegacyRsaPrivateKey privateKey;
	privateKey.d = std::make_unique<Private>();
	privateKey.d->key = std::move(key);
	privateKey.d->publicKey = std::move(*publicKey);

	clearLastError();
	return privateKey;
}



bool LegacyRsaPrivateKey::isNull() const
{
	return d == nullptr;
}



int LegacyRsaPrivateKey::bits() const
{
	return d ? EVP_PKEY_bits(d->key.get()) : 0;
}



std::optional<SecureBuffer> LegacyRsaPrivateKey::toPem() const
{
	static constexpr auto Operation = "LegacyRsaPrivateKey::toPem";

	clearLastError();

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}

	const auto der = Internal::privateKeyToPkcs8Der(Operation, d->key.get());
	if (der.has_value() == false)
	{
		return std::nullopt;
	}

	return Internal::pemEncode(Operation, *der, Pkcs8Label);
}



LegacyRsaPublicKey LegacyRsaPrivateKey::publicKey() const
{
	return d ? d->publicKey : LegacyRsaPublicKey{};
}



std::optional<QByteArray> LegacyRsaPrivateKey::signPkcs1Sha512(ByteView message) const
{
	static constexpr auto Operation = "LegacyRsaPrivateKey::signPkcs1Sha512";

	clearLastError();

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}

	return createPkcs1Sha512Signature(Operation, d->key.get(), message);
}



std::optional<SecureBuffer> LegacyRsaPrivateKey::decryptOaepSha1(ByteView ciphertext) const
{
	static constexpr auto Operation = "LegacyRsaPrivateKey::decryptOaepSha1";
	static constexpr auto Failure = "decryption failed";

	clearLastError();

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}

	const auto modulusSize = EVP_PKEY_size(d->key.get());
	SecureBuffer plaintext(modulusSize);
	auto plaintextSize = size_t(plaintext.size());
	Internal::EvpPkeyContextPtr context(EVP_PKEY_CTX_new(d->key.get(), nullptr));
	if (ciphertext.size() != modulusSize ||
		plaintext.size() != modulusSize ||
		context == nullptr ||
		EVP_PKEY_decrypt_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_padding(context.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
		EVP_PKEY_CTX_set_rsa_oaep_md(context.get(), EVP_sha1()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_mgf1_md(context.get(), EVP_sha1()) <= 0 ||
		EVP_PKEY_decrypt(context.get(), plaintext.bytes(), &plaintextSize,
						 ciphertext.bytes(), size_t(ciphertext.size())) <= 0 ||
		plaintextSize > size_t(plaintext.size()) ||
		plaintext.resize(qsizetype(plaintextSize)) == false)
	{
		// one error text for all failures and no OpenSSL reason codes
		ERR_clear_error();
		Internal::setError(Operation, Failure);
		return std::nullopt;
	}

	return plaintext;
}

} // namespace Veyon::Crypto
