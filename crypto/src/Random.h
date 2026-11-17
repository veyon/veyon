// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "SecureBuffer.h"

#include <optional>

namespace VeyonCrypto {

// Cryptographically secure random numbers from the OpenSSL CSPRNG (RAND_bytes). Never use
// QRandomGenerator for tokens, nonces, salts, keys or serial numbers.
class VEYON_CRYPTO_EXPORT Random
{
public:
	static constexpr qsizetype MaximumSize = 1024 * 1024;

	// count must be 1..MaximumSize; fails (nullopt) if the CSPRNG is not seeded
	[[nodiscard]] static std::optional<SecureBuffer> bytes(qsizetype count);

	// lower-case hex representation of byteCount random bytes (2 * byteCount characters), for
	// non-secret identifiers such as endpoint names; for secret tokens use
	// bytes() and SecureBuffer::toHex(), which keep the hex form in secure memory
	[[nodiscard]] static std::optional<QString> hex(qsizetype byteCount);
};

} // namespace VeyonCrypto
