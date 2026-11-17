// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

#include <optional>

namespace Veyon::Crypto {

// SHA-2 message digests
class VEYON_CRYPTO_EXPORT Hash
{
public:
	enum class Algorithm {
		Sha256,
		Sha384,
		Sha512,
	};

	static constexpr qsizetype size(Algorithm algorithm)
	{
		switch (algorithm)
		{
		case Algorithm::Sha256: return 32;
		case Algorithm::Sha384: return 48;
		case Algorithm::Sha512: return 64;
		}
		return 0;
	}

	[[nodiscard]] static std::optional<QByteArray> digest(Algorithm algorithm, ByteView data);

	[[nodiscard]] static std::optional<QByteArray> sha256(ByteView data)
	{
		return digest(Algorithm::Sha256, data);
	}
};

} // namespace Veyon::Crypto
