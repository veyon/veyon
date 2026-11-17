// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PublicKey.h"

namespace VeyonCrypto {

// Private key (EC P-256, Ed25519 or RSA 3072..8192 bits, see KeyAlgorithm for the bound signature
// schemes). Move-only so that ownership of key material stays explicit; safe to use from several
// threads for signing. Serialisation is only possible into a SecureBuffer, as unencrypted PKCS#8
// (encrypt it with PasswordKdf + Aead where it leaves the SecretStore).
class VEYON_CRYPTO_EXPORT PrivateKey
{
public:
	static constexpr int MinimumRsaBits = 3072;
	static constexpr int DefaultRsaBits = 3072;
	static constexpr int MaximumRsaBits = 8192;
	static constexpr qsizetype MaximumEncodedSize = 16 * 1024;
	static constexpr qsizetype Ed25519SeedSize = 32;

	PrivateKey();
	~PrivateKey();
	PrivateKey(PrivateKey&& other) noexcept;
	PrivateKey& operator=(PrivateKey&& other) noexcept;

	PrivateKey(const PrivateKey&) = delete;
	PrivateKey& operator=(const PrivateKey&) = delete;

	// rsaBits is used for KeyAlgorithm::Rsa only and must be MinimumRsaBits..MaximumRsaBits
	[[nodiscard]] static std::optional<PrivateKey> generate(KeyAlgorithm algorithm, int rsaBits = DefaultRsaBits);

	// unencrypted PKCS#8 PrivateKeyInfo only (no traditional/SEC1/PKCS#1 formats, no encrypted
	// PEM, never prompts for a password); the whole DER input must be consumed
	[[nodiscard]] static std::optional<PrivateKey> fromPkcs8Der(ByteView der);
	[[nodiscard]] static std::optional<PrivateKey> fromPkcs8Pem(ByteView pem);
	// 32-byte Ed25519 private key (seed) as in RFC 8032
	[[nodiscard]] static std::optional<PrivateKey> fromEd25519Seed(ByteView seed);

	bool isNull() const;
	KeyAlgorithm algorithm() const;
	int bits() const;

	[[nodiscard]] std::optional<SecureBuffer> toPkcs8Der() const;
	[[nodiscard]] std::optional<SecureBuffer> toPkcs8Pem() const;

	// null PublicKey for a null private key
	PublicKey publicKey() const;

	// signature with the scheme bound to algorithm(); ECDSA signatures are DER-encoded
	[[nodiscard]] std::optional<QByteArray> sign(ByteView message) const;

private:
	friend struct Internal::Access;
	class Private;

	std::unique_ptr<Private> d;
};

} // namespace VeyonCrypto
