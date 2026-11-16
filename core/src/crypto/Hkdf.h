// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

#include <optional>

namespace VeyonCrypto {

// HKDF-SHA256 (RFC 5869). The input key material must not be empty; an empty salt means the
// RFC default (HashLen zero bytes).
class VEYON_CORE_EXPORT Hkdf
{
public:
	static constexpr qsizetype PseudoRandomKeySize = 32;
	static constexpr qsizetype MaximumOutputSize = 255 * 32;
	// OpenSSL 1.1.1 limits info to 1024 bytes, 3.x does not; enforced identically on all versions
	static constexpr qsizetype MaximumInfoSize = 1024;
	static constexpr qsizetype MaximumSaltSize = 1024;
	static constexpr qsizetype MaximumInputKeyMaterialSize = 1024;

	// extract and expand
	[[nodiscard]] static std::optional<SecureBuffer> sha256(ByteView inputKeyMaterial, ByteView salt, ByteView info,
															qsizetype outputSize);

	[[nodiscard]] static std::optional<SecureBuffer> extractSha256(ByteView inputKeyMaterial, ByteView salt);

	// pseudoRandomKey must have at least PseudoRandomKeySize bytes
	[[nodiscard]] static std::optional<SecureBuffer> expandSha256(ByteView pseudoRandomKey, ByteView info,
																  qsizetype outputSize);
};

} // namespace VeyonCrypto
