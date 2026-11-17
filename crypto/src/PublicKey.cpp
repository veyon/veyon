// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CryptoPrivate.h"
#include "Hash.h"
#include "OpenSslCompat.h"

#include <cstring>
#include <openssl/pem.h>

namespace Veyon::Crypto {

PublicKey::PublicKey() = default;
PublicKey::~PublicKey() = default;
PublicKey::PublicKey(const PublicKey& other) = default;
PublicKey::PublicKey(PublicKey&& other) noexcept = default;
PublicKey& PublicKey::operator=(const PublicKey& other) = default;
PublicKey& PublicKey::operator=(PublicKey&& other) noexcept = default;



PublicKey::PublicKey(std::shared_ptr<const Private> d) :
	d(std::move(d))
{
}



std::optional<PublicKey> PublicKey::fromDer(ByteView subjectPublicKeyInfo)
{
	static constexpr auto Operation = "PublicKey::fromDer";

	if (subjectPublicKeyInfo.isEmpty() || subjectPublicKeyInfo.size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	// whole input consumed and re-encoding identical; the input bytes stay
	// the SubjectPublicKeyInfo of this key (pin definition, see spkiSha256())
	auto input = subjectPublicKeyInfo.bytes();
	Internal::X509PubkeyPtr publicKeyInfo(d2i_X509_PUBKEY(nullptr, &input, long(subjectPublicKeyInfo.size())));
	unsigned char* reencoded = nullptr;
	const auto reencodedSize = publicKeyInfo ? i2d_X509_PUBKEY(publicKeyInfo.get(), &reencoded) : 0;
	const auto isStrictDer = input == subjectPublicKeyInfo.bytes() + subjectPublicKeyInfo.size() &&
		reencodedSize == subjectPublicKeyInfo.size() &&
		memcmp(reencoded, subjectPublicKeyInfo.bytes(), size_t(reencodedSize)) == 0;
	OPENSSL_free(reencoded);
	if (publicKeyInfo == nullptr || isStrictDer == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "invalid or non-DER encoding");
		return std::nullopt;
	}

	// the BIT STRING content (e.g. an RSAPublicKey) must be DER as well, otherwise keys that
	// compare equal would have different pins (and the pin of a CSR differ from the one of the
	// certificate issued for it, which contains OpenSSL's encoding)
	Internal::EvpPkeyPtr key(X509_PUBKEY_get(publicKeyInfo.get()));
	if (key == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}
	if (Internal::isCanonicalSubjectPublicKeyInfo(key.get(), subjectPublicKeyInfo) == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "non-DER key encoding");
		return std::nullopt;
	}

	return Internal::Access::toPublicKey(Operation, std::move(key),
										 QByteArray(subjectPublicKeyInfo.data(), int(subjectPublicKeyInfo.size())));
}



std::optional<PublicKey> PublicKey::fromPem(ByteView pem)
{
	static constexpr auto Operation = "PublicKey::fromPem";

	if (pem.isEmpty() || pem.size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	const auto bio = Internal::readOnlyBio(pem);
	QByteArray der;
	if (bio == nullptr ||
		Internal::readSinglePemBlock(bio.get(), "PUBLIC KEY", MaximumEncodedSize, der) != Internal::PemReadResult::Block)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "invalid, missing or more than one PUBLIC KEY block");
		return std::nullopt;
	}

	return fromDer(der);
}



std::optional<PublicKey> PublicKey::fromEd25519RawKey(ByteView rawKey)
{
	static constexpr auto Operation = "PublicKey::fromEd25519RawKey";

	if (rawKey.size() != Ed25519KeySize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	Internal::EvpPkeyPtr key(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, rawKey.bytes(), size_t(rawKey.size())));
	if (key == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return Internal::Access::toPublicKey(Operation, std::move(key));
}



bool PublicKey::isNull() const
{
	return d == nullptr;
}



KeyAlgorithm PublicKey::algorithm() const
{
	return d ? d->algorithm : KeyAlgorithm::Invalid;
}



int PublicKey::bits() const
{
	// EVP_PKEY_bits() reports 253 (OpenSSL 1.1.1) or 256 (3.x) for Ed25519
	if (d == nullptr)
	{
		return 0;
	}
	return d->algorithm == KeyAlgorithm::Ed25519 ? 256 : EVP_PKEY_bits(d->key.get());
}



QByteArray PublicKey::toDer() const
{
	return d ? d->der : QByteArray{};
}



QByteArray PublicKey::toPem() const
{
	if (d == nullptr)
	{
		return {};
	}

	Internal::BioPtr bio(BIO_new(BIO_s_mem()));
	if (bio == nullptr || PEM_write_bio_PUBKEY(bio.get(), d->key.get()) != 1)
	{
		Internal::setOpenSslError("PublicKey::toPem");
		return {};
	}

	return Internal::bioContents(bio.get());
}



QByteArray PublicKey::toEd25519RawKey() const
{
	if (algorithm() != KeyAlgorithm::Ed25519)
	{
		return {};
	}

	QByteArray rawKey(int(Ed25519KeySize), 0);
	auto rawKeySize = size_t(rawKey.size());
	if (EVP_PKEY_get_raw_public_key(d->key.get(), reinterpret_cast<unsigned char*>(rawKey.data()), &rawKeySize) != 1 ||
		rawKeySize != size_t(Ed25519KeySize))
	{
		Internal::setOpenSslError("PublicKey::toEd25519RawKey");
		return {};
	}

	return rawKey;
}



QByteArray PublicKey::spkiSha256() const
{
	return d ? Hash::sha256(d->der).value_or(QByteArray{}) : QByteArray{};
}



QString PublicKey::spkiFingerprint() const
{
	const auto hash = spkiSha256();
	return hash.isEmpty() ? QString{} : QStringLiteral("sha256:") + QString::fromLatin1(hash.toHex());
}



bool PublicKey::verify(ByteView message, ByteView signature) const
{
	static constexpr auto Operation = "PublicKey::verify";
	static constexpr qsizetype MaximumSignatureSize = 1024;

	if (d == nullptr || signature.isEmpty() || signature.size() > MaximumSignatureSize)
	{
		Internal::setError(Operation, "null key or invalid signature size");
		return false;
	}

	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	if (context == nullptr ||
		Internal::initializeSignatureContext(context.get(), d->key.get(), d->algorithm, false) == false)
	{
		Internal::setOpenSslError(Operation);
		return false;
	}

	if (EVP_DigestVerify(context.get(), signature.bytes(), size_t(signature.size()),
						 message.bytes(), size_t(message.size())) != 1)
	{
		Internal::setOpenSslError(Operation);
		return false;
	}

	return true;
}



bool PublicKey::operator==(const PublicKey& other) const
{
	return d && other.d && OpenSslCompat::isSamePublicKey(d->key.get(), other.d->key.get());
}



std::optional<PublicKey> Internal::Access::toPublicKey(const char* operation, EvpPkeyPtr key, QByteArray subjectPublicKeyInfo)
{
	const auto algorithm = classifyKey(key.get());
	if (algorithm == KeyAlgorithm::Invalid)
	{
		setError(operation, "unsupported key type, curve or size");
		return std::nullopt;
	}

	auto der = subjectPublicKeyInfo.isEmpty() ? Internal::subjectPublicKeyInfo(key.get()) : std::move(subjectPublicKeyInfo);
	if (der.isEmpty())
	{
		setOpenSslError(operation);
		return std::nullopt;
	}

	auto d = std::make_shared<PublicKey::Private>();
	d->key = std::move(key);
	d->algorithm = algorithm;
	d->der = std::move(der);

	return PublicKey(std::move(d));
}

} // namespace Veyon::Crypto
