// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ConstantTime.h"

#include <openssl/crypto.h>

namespace VeyonCrypto {

bool constantTimeEquals(ByteView first, ByteView second)
{
	if (first.isEmpty() || first.size() != second.size())
	{
		return false;
	}

	return CRYPTO_memcmp(first.data(), second.data(), size_t(first.size())) == 0;
}

} // namespace VeyonCrypto
