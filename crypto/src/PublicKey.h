// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "SecureBuffer.h"

#include <memory>
#include <optional>

namespace Veyon::Crypto {

namespace Internal {
struct Access;
}

// The only key types VeyonCrypto accepts. Each one is bound to exactly one signature scheme so that
// callers can never pick a weak or mismatching algorithm:
// EcP256 → ECDSA-SHA256 (DER-encoded signature), Ed25519 → Ed25519 (RFC 8032),
// Rsa (3072..8192 bits) → RSASSA-PSS with SHA-256, MGF1-SHA256 and a 32-byte salt.
enum class KeyAlgorithm {
	Invalid,
	EcP256,
	Ed25519,
	Rsa,
};

// Public key (SubjectPublicKeyInfo). Immutable, implicitly shared and safe to use from several
// threads. Keys of other types/curves/sizes are rejected on import.
class VEYON_CRYPTO_EXPORT PublicKey
{
public:
	static constexpr qsizetype MaximumEncodedSize = 16 * 1024;
	static constexpr qsizetype Ed25519KeySize = 32;

	PublicKey();
	~PublicKey();
	PublicKey(const PublicKey& other);
	PublicKey(PublicKey&& other) noexcept;
	PublicKey& operator=(const PublicKey& other);
	PublicKey& operator=(PublicKey&& other) noexcept;

	// DER SubjectPublicKeyInfo; whole input consumed and equal to OpenSSL's encoding of the decoded
	// key (also the key inside the BIT STRING, e.g. the RSAPublicKey, must be DER; EC keys keep their
	// point form), so that equal keys from certificates, requests and DER input have equal pins
	[[nodiscard]] static std::optional<PublicKey> fromDer(ByteView subjectPublicKeyInfo);
	// exactly one PEM "PUBLIC KEY" block (as Certificate::fromPem()), decoded content as for fromDer()
	[[nodiscard]] static std::optional<PublicKey> fromPem(ByteView pem);
	// raw 32-byte Ed25519 public key (RFC 8032)
	[[nodiscard]] static std::optional<PublicKey> fromEd25519RawKey(ByteView rawKey);

	bool isNull() const;
	KeyAlgorithm algorithm() const;
	int bits() const;

	// DER SubjectPublicKeyInfo as described for spkiSha256(), empty for a null key
	QByteArray toDer() const;
	QByteArray toPem() const;
	// raw 32-byte key for Ed25519 keys, empty otherwise
	QByteArray toEd25519RawKey() const;

	// The SPKI pin (the single definition used everywhere in Veyon): SHA-256 over the DER
	// SubjectPublicKeyInfo exactly as contained in the source this key was read from (certificate,
	// request or fromDer()/fromPem() input; for keys created from a private key or raw bytes:
	// OpenSSL's encoding), and its "sha256:<lower-case hex>" form. Equal to
	// Certificate::spkiSha256() for Certificate::publicKey(). Empty for a null key.
	QByteArray spkiSha256() const;
	QString spkiFingerprint() const;

	// false for a null key or any failure
	[[nodiscard]] bool verify(ByteView message, ByteView signature) const;

	// compares the keys semantically (EVP_PKEY_eq), independent of the encoding, e.g. compressed
	// and uncompressed EC points; null keys are never equal
	bool operator==(const PublicKey& other) const;
	bool operator!=(const PublicKey& other) const
	{
		return (*this == other) == false;
	}

private:
	friend struct Internal::Access;
	class Private;
	explicit PublicKey(std::shared_ptr<const Private> d);

	std::shared_ptr<const Private> d;
};

} // namespace Veyon::Crypto
