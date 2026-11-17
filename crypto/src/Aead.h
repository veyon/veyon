// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

#include <optional>

namespace VeyonCrypto {

// Authenticated encryption with associated data: ChaCha20-Poly1305 (RFC 8439) and AES-256-GCM.
//
// Key 32 bytes, nonce 12 bytes, tag 16 bytes appended to the ciphertext. The caller provides the
// nonce explicitly and MUST never use a nonce twice with the same key (e.g. a per-direction record
// counter as in 02-ipc.md §5, or a fresh random nonce for a fresh key derived from a random salt).
// open() verifies the tag before releasing any plaintext; on failure nothing is returned.
class VEYON_CRYPTO_EXPORT Aead
{
public:
	enum class Algorithm {
		ChaCha20Poly1305,
		Aes256Gcm,
	};

	static constexpr qsizetype KeySize = 32;
	static constexpr qsizetype NonceSize = 12;
	static constexpr qsizetype TagSize = 16;
	static constexpr qsizetype MaximumPlaintextSize = SecureBuffer::MaximumSize;

	// returns ciphertext || tag
	[[nodiscard]] static std::optional<QByteArray> seal(Algorithm algorithm, ByteView key, ByteView nonce,
														ByteView plaintext, ByteView associatedData);

	// ciphertextWithTag = ciphertext || tag as returned by seal()
	[[nodiscard]] static std::optional<SecureBuffer> open(Algorithm algorithm, ByteView key, ByteView nonce,
														  ByteView ciphertextWithTag, ByteView associatedData);
};

} // namespace VeyonCrypto
