// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

#include <memory>
#include <optional>

namespace Veyon::Crypto {

// Public RSA key of the Veyon 4.x legacy schemes (byte-compatible with the QCA-based
// implementation of Veyon <= 4.11, see LegacyRsa.h). Deliberately a separate type from PublicKey:
// its operations are the legacy RSASSA-PKCS1-v1_5 and RSAES-OAEP-SHA1 schemes, which new code must
// not use. Immutable, implicitly shared and safe to use from several threads.
//
// Key policy (enforced on every import): rsaEncryption keys (no RSASSA-PSS-restricted keys) with
// an odd modulus of MinimumBits..MaximumBits bits and an odd public exponent 3..2^32-1. Veyon 4.x
// generates 4096-bit keys with exponent 65537; QCA also accepted smaller and larger keys, which are
// rejected here (MaximumBits also bounds the CPU time an imported key can cost).
//
// Unlike the other VeyonCrypto classes, successful operations of the LegacyRsa classes clear
// lastError(), so that callers ported from QCA can log it without stale messages.
class VEYON_CRYPTO_EXPORT LegacyRsaPublicKey
{
public:
	static constexpr int MinimumBits = 2048;
	static constexpr int MaximumBits = 8192;
	static constexpr qsizetype MaximumEncodedSize = 4 * 1024;
	static constexpr qsizetype MaximumPemSize = 16 * 1024;
	// RSAES-OAEP with SHA-1: modulus size - 2 * 20 - 2
	static constexpr qsizetype OaepSha1Overhead = 42;

	LegacyRsaPublicKey();
	~LegacyRsaPublicKey();
	LegacyRsaPublicKey(const LegacyRsaPublicKey& other);
	LegacyRsaPublicKey(LegacyRsaPublicKey&& other) noexcept;
	LegacyRsaPublicKey& operator=(const LegacyRsaPublicKey& other);
	LegacyRsaPublicKey& operator=(LegacyRsaPublicKey&& other) noexcept;

	// DER SubjectPublicKeyInfo (rsaEncryption); the whole input must be consumed and be the DER
	// encoding of the key (also the RSAPublicKey inside the BIT STRING)
	[[nodiscard]] static std::optional<LegacyRsaPublicKey> fromDer(ByteView subjectPublicKeyInfo);
	// exactly one public key PEM block: "PUBLIC KEY" (SubjectPublicKeyInfo, as written by 4.x) or
	// "RSA PUBLIC KEY" (PKCS#1 RSAPublicKey, also read by 4.x); text outside the block and blocks
	// with other labels are ignored, RFC 1421 headers are rejected
	[[nodiscard]] static std::optional<LegacyRsaPublicKey> fromPem(ByteView pem);

	bool isNull() const;
	int bits() const;

	// DER SubjectPublicKeyInfo, as the public key DER of 4.x (e.g. the key pair ID); empty for a
	// null key
	[[nodiscard]] QByteArray toDer() const;
	// "PUBLIC KEY" PEM, byte-identical to the public key files written by 4.x and to the public key
	// sent in the 4.x logon authentication; empty for a null key
	[[nodiscard]] QByteArray toPem() const;

	// RSASSA-PKCS1-v1_5 with SHA-512 (QCA EMSA3_SHA512): the 4.x key file authentication.
	// The signature must have exactly the size of the modulus. false for a null key or any failure.
	[[nodiscard]] bool verifyPkcs1Sha512(ByteView message, ByteView signature) const;

	// RSAES-OAEP with SHA-1, MGF1-SHA1 and an empty label (QCA EME_PKCS1_OAEP): the 4.x logon
	// authentication and the 4.x password obfuscation. Plaintexts longer than
	// maximumOaepSha1PlaintextSize() are rejected (4.x silently truncated them); the ciphertext has
	// the size of the modulus.
	[[nodiscard]] std::optional<QByteArray> encryptOaepSha1(ByteView plaintext) const;
	[[nodiscard]] qsizetype maximumOaepSha1PlaintextSize() const;

	// equal SubjectPublicKeyInfo encodings (canonical DER, so equal keys); null keys are never equal
	bool operator==(const LegacyRsaPublicKey& other) const;
	bool operator!=(const LegacyRsaPublicKey& other) const
	{
		return (*this == other) == false;
	}

private:
	class Private;
	explicit LegacyRsaPublicKey(std::shared_ptr<const Private> d);

	std::shared_ptr<const Private> d;
};

} // namespace Veyon::Crypto
