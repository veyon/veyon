// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"

#include <optional>

namespace VeyonCrypto {

// Password-based key derivation for encrypted exports (03 §4.6) and the CA key backup (06 §11.1).
//
// Argon2id (RFC 9106) requires OpenSSL >= 3.2, scrypt (RFC 7914) is always available. Writers use
// preferredAlgorithm() with the default parameters and store algorithm and parameters next to the
// ciphertext; readers pass the stored parameters. Parameters are bounded (Maximum* below) so that a
// crafted file cannot make the reader allocate excessive memory or CPU time. The password and
// salt must not be empty.
class VEYON_CRYPTO_EXPORT PasswordKdf
{
public:
	enum class Algorithm {
		Argon2id,
		Scrypt,
	};

	static constexpr qsizetype SaltSize = 16;
	static constexpr qsizetype MinimumOutputSize = 16;
	static constexpr qsizetype MaximumOutputSize = 1024;
	static constexpr qsizetype MaximumPasswordSize = 64 * 1024;

	// defaults: 03-secretstore-config.md §2 item 6
	struct Argon2idParameters
	{
		quint32 iterations = 3;
		quint32 memoryKiB = 64 * 1024;
		quint32 parallelism = 1;
	};

	struct ScryptParameters
	{
		quint64 cost = 1 << 15;	 // N, power of two
		quint32 blockSize = 8;	 // r
		quint32 parallelism = 1; // p
	};

	static constexpr quint32 MaximumArgon2idIterations = 16;
	static constexpr quint32 MaximumArgon2idMemoryKiB = 1024 * 1024;
	static constexpr quint32 MaximumParallelism = 16;
	static constexpr quint64 MaximumScryptCost = 1 << 20;
	static constexpr quint32 MaximumScryptBlockSize = 32;
	static constexpr quint64 MaximumScryptMemory = 1024 * 1024 * 1024;

	[[nodiscard]] static bool isArgon2idAvailable();
	[[nodiscard]] static Algorithm preferredAlgorithm();

	[[nodiscard]] static std::optional<SecureBuffer> argon2id(ByteView password, ByteView salt,
															  const Argon2idParameters& parameters, qsizetype outputSize);

	[[nodiscard]] static std::optional<SecureBuffer> scrypt(ByteView password, ByteView salt,
															const ScryptParameters& parameters, qsizetype outputSize);
};

} // namespace VeyonCrypto
