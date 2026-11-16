// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

#include <cstddef>
#include <cstdint>

// Internal header of the VeyonCrypto layer. The only place for code that depends on the OpenSSL
// version (06-pki.md §2, budget 150 lines, checked by .ci/common/check-openssl-version-guards.sh).

struct evp_pkey_st;

namespace VeyonCrypto::OpenSslCompat {

// semantic comparison of the public components (EVP_PKEY_eq, OpenSSL 1.1.1: EVP_PKEY_cmp)
[[nodiscard]] VEYON_CORE_EXPORT bool isSamePublicKey(const evp_pkey_st* first, const evp_pkey_st* second);

// Argon2id (RFC 9106, EVP_KDF "ARGON2ID") needs OpenSSL >= 3.2 at build and run time (expected);
// the provider configuration may still lack it (e.g. FIPS only), hence the runtime check
[[nodiscard]] VEYON_CORE_EXPORT bool isArgon2idAvailable();
[[nodiscard]] VEYON_CORE_EXPORT bool isArgon2idExpected();

[[nodiscard]] VEYON_CORE_EXPORT bool argon2id(const unsigned char* password, size_t passwordSize,
											  const unsigned char* salt, size_t saltSize,
											  const unsigned char* secret, size_t secretSize,
											  const unsigned char* associatedData, size_t associatedDataSize,
											  uint32_t iterations, uint32_t memoryKiB, uint32_t lanes,
											  unsigned char* output, size_t outputSize);

} // namespace VeyonCrypto::OpenSslCompat
