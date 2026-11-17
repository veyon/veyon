// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "VeyonCryptoExport.h"

namespace Veyon::Crypto {

// Description of the most recent failed VeyonCrypto operation in the calling thread (errno
// semantics: only meaningful directly after a function reported failure, successful calls do not
// reset it). It names the operation and the OpenSSL reason codes only and never contains input
// data or key material, so it may be logged.
[[nodiscard]] VEYON_CRYPTO_EXPORT QString lastError();

VEYON_CRYPTO_EXPORT void clearLastError();

// version of the OpenSSL library loaded at runtime (diagnostics), e.g. "OpenSSL 3.0.13 30 Jan 2024"
[[nodiscard]] VEYON_CRYPTO_EXPORT QString openSslVersion();

} // namespace Veyon::Crypto
