// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CryptoPrivate.h"

#include <cstring>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace Veyon::Crypto {

PrivateKey::PrivateKey() = default;
PrivateKey::~PrivateKey() = default;
PrivateKey::PrivateKey(PrivateKey&& other) noexcept = default;
PrivateKey& PrivateKey::operator=(PrivateKey&& other) noexcept = default;



std::optional<PrivateKey> PrivateKey::generate(KeyAlgorithm algorithm, int rsaBits)
{
	static constexpr auto Operation = "PrivateKey::generate";

	Internal::EvpPkeyContextPtr context;
	switch (algorithm)
	{
	case KeyAlgorithm::EcP256:
		context.reset(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr));
		if (context == nullptr ||
			EVP_PKEY_keygen_init(context.get()) <= 0 ||
			EVP_PKEY_CTX_set_ec_paramgen_curve_nid(context.get(), NID_X9_62_prime256v1) <= 0 ||
			EVP_PKEY_CTX_set_ec_param_enc(context.get(), OPENSSL_EC_NAMED_CURVE) <= 0)
		{
			Internal::setOpenSslError(Operation);
			return std::nullopt;
		}
		break;
	case KeyAlgorithm::Ed25519:
		context.reset(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr));
		if (context == nullptr || EVP_PKEY_keygen_init(context.get()) <= 0)
		{
			Internal::setOpenSslError(Operation);
			return std::nullopt;
		}
		break;
	case KeyAlgorithm::Rsa:
		if (rsaBits < MinimumRsaBits || rsaBits > MaximumRsaBits)
		{
			Internal::setError(Operation, "invalid RSA key size");
			return std::nullopt;
		}
		context.reset(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
		if (context == nullptr ||
			EVP_PKEY_keygen_init(context.get()) <= 0 ||
			EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), rsaBits) <= 0)
		{
			Internal::setOpenSslError(Operation);
			return std::nullopt;
		}
		break;
	case KeyAlgorithm::Invalid:
		Internal::setError(Operation, "invalid algorithm");
		return std::nullopt;
	}

	EVP_PKEY* key = nullptr;
	if (EVP_PKEY_keygen(context.get(), &key) <= 0)
	{
		EVP_PKEY_free(key);
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return Internal::Access::toPrivateKey(Operation, Internal::EvpPkeyPtr(key));
}



std::optional<PrivateKey> PrivateKey::fromPkcs8Der(ByteView der)
{
	static constexpr auto Operation = "PrivateKey::fromPkcs8Der";

	if (der.isEmpty() || der.size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	// PKCS8_PRIV_KEY_INFO_free() zeroises the decoded key octets
	auto input = der.bytes();
	Internal::Pkcs8Ptr keyInfo(d2i_PKCS8_PRIV_KEY_INFO(nullptr, &input, long(der.size())));
	if (keyInfo == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}
	if (input != der.bytes() + der.size())
	{
		Internal::setError(Operation, "trailing data");
		return std::nullopt;
	}

	Internal::EvpPkeyPtr key(EVP_PKCS82PKEY(keyInfo.get()));
	if (key == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return Internal::Access::toPrivateKey(Operation, std::move(key));
}



std::optional<PrivateKey> PrivateKey::fromPkcs8Pem(ByteView pem)
{
	static constexpr auto Operation = "PrivateKey::fromPkcs8Pem";

	if (pem.isEmpty() || pem.size() > 2 * MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	// decode into the secure heap (PEM_read_bio_PKCS8_PRIV_KEY_INFO() would leave the DER in freed
	// normal heap memory) and accept "PRIVATE KEY" blocks only
	const auto bio = Internal::readOnlyBio(pem);
	unsigned char* der = nullptr;
	long derSize = 0;
	if (bio == nullptr ||
		PEM_bytes_read_bio_secmem(&der, &derSize, nullptr, "PRIVATE KEY", bio.get(), Internal::noPasswordCallback, nullptr) != 1)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	auto key = fromPkcs8Der(ByteView(reinterpret_cast<const char*>(der), qsizetype(derSize)));
	OPENSSL_secure_clear_free(der, size_t(derSize));

	return key;
}



std::optional<PrivateKey> PrivateKey::fromEd25519Seed(ByteView seed)
{
	static constexpr auto Operation = "PrivateKey::fromEd25519Seed";

	if (seed.size() != Ed25519SeedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	Internal::EvpPkeyPtr key(EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, seed.bytes(), size_t(seed.size())));
	if (key == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return Internal::Access::toPrivateKey(Operation, std::move(key));
}



bool PrivateKey::isNull() const
{
	return d == nullptr;
}



KeyAlgorithm PrivateKey::algorithm() const
{
	return d ? d->algorithm : KeyAlgorithm::Invalid;
}



int PrivateKey::bits() const
{
	// EVP_PKEY_bits() reports 253 (OpenSSL 1.1.1) or 256 (3.x) for Ed25519
	if (d == nullptr)
	{
		return 0;
	}
	return d->algorithm == KeyAlgorithm::Ed25519 ? 256 : EVP_PKEY_bits(d->key.get());
}



std::optional<SecureBuffer> PrivateKey::toPkcs8Der() const
{
	static constexpr auto Operation = "PrivateKey::toPkcs8Der";

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}

	Internal::Pkcs8Ptr keyInfo(EVP_PKEY2PKCS8(d->key.get()));
	const auto size = keyInfo ? i2d_PKCS8_PRIV_KEY_INFO(keyInfo.get(), nullptr) : 0;
	if (size <= 0)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	// encode directly into the secure buffer (no intermediate copy)
	SecureBuffer der(size);
	auto output = der.bytes();
	if (i2d_PKCS8_PRIV_KEY_INFO(keyInfo.get(), &output) != size)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return der;
}



std::optional<SecureBuffer> PrivateKey::toPkcs8Pem() const
{
	static constexpr auto Operation = "PrivateKey::toPkcs8Pem";
	static constexpr int LineLength = 64;
	static constexpr char Header[] = "-----BEGIN PRIVATE KEY-----\n";
	static constexpr char Footer[] = "-----END PRIVATE KEY-----\n";

	const auto der = toPkcs8Der();
	if (der.has_value() == false)
	{
		return std::nullopt;
	}

	// RFC 7468 encoding done here so that no key material passes through OpenSSL's PEM buffers
	SecureBuffer base64(4 * ((der->size() + 2) / 3) + 1);
	const auto base64Size = EVP_EncodeBlock(base64.bytes(), der->bytes(), int(der->size()));
	if (base64Size <= 0)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	const auto lineCount = (base64Size + LineLength - 1) / LineLength;
	SecureBuffer pem(qsizetype(sizeof(Header) - 1 + base64Size + lineCount + sizeof(Footer) - 1));
	auto output = pem.data();
	std::memcpy(output, Header, sizeof(Header) - 1);
	output += sizeof(Header) - 1;
	for (int offset = 0; offset < base64Size; offset += LineLength)
	{
		const auto chunk = qMin(LineLength, base64Size - offset);
		std::memcpy(output, base64.constData() + offset, size_t(chunk));
		output += chunk;
		*output++ = '\n';
	}
	std::memcpy(output, Footer, sizeof(Footer) - 1);

	return pem;
}



PublicKey PrivateKey::publicKey() const
{
	if (d == nullptr)
	{
		return {};
	}

	const auto der = Internal::subjectPublicKeyInfo(d->key.get());
	return PublicKey::fromDer(der).value_or(PublicKey{});
}



std::optional<QByteArray> PrivateKey::sign(ByteView message) const
{
	static constexpr auto Operation = "PrivateKey::sign";

	if (d == nullptr)
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}

	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	size_t signatureSize = 0;
	if (context == nullptr ||
		Internal::initializeSignatureContext(context.get(), d->key.get(), d->algorithm, true) == false ||
		EVP_DigestSign(context.get(), nullptr, &signatureSize, message.bytes(), size_t(message.size())) != 1)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	QByteArray signature(int(signatureSize), 0);
	if (EVP_DigestSign(context.get(), reinterpret_cast<unsigned char*>(signature.data()), &signatureSize,
					   message.bytes(), size_t(message.size())) != 1)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	// ECDSA signatures may be shorter than the maximum size reported first
	signature.truncate(int(signatureSize));
	return signature;
}



std::optional<PrivateKey> Internal::Access::toPrivateKey(const char* operation, EvpPkeyPtr key)
{
	const auto algorithm = classifyKey(key.get());
	if (algorithm == KeyAlgorithm::Invalid)
	{
		setError(operation, "unsupported key type, curve or size");
		return std::nullopt;
	}

	PrivateKey privateKey;
	privateKey.d = std::make_unique<PrivateKey::Private>();
	privateKey.d->key = std::move(key);
	privateKey.d->algorithm = algorithm;

	return privateKey;
}

} // namespace Veyon::Crypto
