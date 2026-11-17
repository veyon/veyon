// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

namespace Veyon::Crypto {

// Compares two byte sequences in time independent of their contents (CRYPTO_memcmp). Use it for
// every comparison of MACs, proofs, tokens and other secrets. The lengths are not treated as
// secret. Fails closed: different lengths and empty inputs never compare equal.
[[nodiscard]] VEYON_CRYPTO_EXPORT bool constantTimeEquals(ByteView first, ByteView second);

} // namespace Veyon::Crypto
