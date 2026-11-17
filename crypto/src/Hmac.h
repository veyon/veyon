// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

#include <optional>

namespace Veyon::Crypto {

// HMAC-SHA256 (RFC 2104). Keys must not be empty.
class VEYON_CRYPTO_EXPORT Hmac
{
public:
	static constexpr qsizetype Sha256Size = 32;

	[[nodiscard]] static std::optional<QByteArray> sha256(ByteView key, ByteView message);

	// computes the MAC and compares it with expectedMac in constant time; prefer this over
	// comparing the result of sha256() manually
	[[nodiscard]] static bool verifySha256(ByteView key, ByteView message, ByteView expectedMac);
};

} // namespace Veyon::Crypto
