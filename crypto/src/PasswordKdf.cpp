// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "OpenSslCompat.h"
#include "OpenSslUtils.h"
#include "PasswordKdf.h"

namespace VeyonCrypto {

static bool checkCommonParameters(const char* operation, ByteView password, ByteView salt, qsizetype outputSize)
{
	if (password.isEmpty() || password.size() > PasswordKdf::MaximumPasswordSize ||
		salt.isEmpty() || salt.size() > PasswordKdf::MaximumPasswordSize)
	{
		Internal::setError(operation, "invalid password or salt size");
		return false;
	}
	if (outputSize < PasswordKdf::MinimumOutputSize || outputSize > PasswordKdf::MaximumOutputSize)
	{
		Internal::setError(operation, "invalid output size");
		return false;
	}
	return true;
}



bool PasswordKdf::isArgon2idAvailable()
{
	return OpenSslCompat::isArgon2idAvailable();
}



PasswordKdf::Algorithm PasswordKdf::preferredAlgorithm()
{
	return isArgon2idAvailable() ? Algorithm::Argon2id : Algorithm::Scrypt;
}



std::optional<SecureBuffer> PasswordKdf::argon2id(ByteView password, ByteView salt,
												  const Argon2idParameters& parameters, qsizetype outputSize)
{
	static constexpr auto Operation = "PasswordKdf::argon2id";

	if (checkCommonParameters(Operation, password, salt, outputSize) == false)
	{
		return std::nullopt;
	}
	// RFC 9106 §3.1: salt >= 8 bytes, memory >= 8 * parallelism KiB
	if (salt.size() < 8 ||
		parameters.iterations < 1 || parameters.iterations > MaximumArgon2idIterations ||
		parameters.parallelism < 1 || parameters.parallelism > MaximumParallelism ||
		parameters.memoryKiB < 8 * parameters.parallelism || parameters.memoryKiB > MaximumArgon2idMemoryKiB)
	{
		Internal::setError(Operation, "invalid parameters");
		return std::nullopt;
	}
	if (isArgon2idAvailable() == false)
	{
		Internal::setError(Operation, "Argon2id requires OpenSSL 3.2 or newer");
		return std::nullopt;
	}

	SecureBuffer output(outputSize);
	if (OpenSslCompat::argon2id(password.bytes(), size_t(password.size()), salt.bytes(), size_t(salt.size()),
								nullptr, 0, nullptr, 0,
								parameters.iterations, parameters.memoryKiB, parameters.parallelism,
								output.bytes(), size_t(output.size())) == false)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return output;
}



std::optional<SecureBuffer> PasswordKdf::scrypt(ByteView password, ByteView salt,
												const ScryptParameters& parameters, qsizetype outputSize)
{
	static constexpr auto Operation = "PasswordKdf::scrypt";

	if (checkCommonParameters(Operation, password, salt, outputSize) == false)
	{
		return std::nullopt;
	}

	const auto isPowerOfTwo = parameters.cost >= 2 && (parameters.cost & (parameters.cost - 1)) == 0;
	if (isPowerOfTwo == false || parameters.cost > MaximumScryptCost ||
		parameters.blockSize < 1 || parameters.blockSize > MaximumScryptBlockSize ||
		parameters.parallelism < 1 || parameters.parallelism > MaximumParallelism)
	{
		Internal::setError(Operation, "invalid parameters");
		return std::nullopt;
	}

	// RFC 7914 memory use: 128 * r * N (V) + 128 * r * p (B) + 256 * r (XY)
	const quint64 requiredMemory = 128 * quint64(parameters.blockSize) *
		(parameters.cost + parameters.parallelism + 2);
	if (requiredMemory > MaximumScryptMemory)
	{
		Internal::setError(Operation, "parameters exceed the memory limit");
		return std::nullopt;
	}

	SecureBuffer output(outputSize);
	if (EVP_PBE_scrypt(password.data(), size_t(password.size()), salt.bytes(), size_t(salt.size()),
					   parameters.cost, parameters.blockSize, parameters.parallelism,
					   requiredMemory + 1024 * 1024, output.bytes(), size_t(output.size())) != 1)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return output;
}

} // namespace VeyonCrypto
