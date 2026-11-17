// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "OpenSslCompat.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER >= 0x30200000L
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#endif

namespace Veyon::Crypto::OpenSslCompat {

bool isSamePublicKey(const evp_pkey_st* first, const evp_pkey_st* second)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	return first && second && EVP_PKEY_eq(first, second) == 1;
#else
	return first && second && EVP_PKEY_cmp(first, second) == 1;
#endif
}



bool isArgon2idExpected()
{
	return OPENSSL_VERSION_NUMBER >= 0x30200000L && OpenSSL_version_num() >= 0x30200000L;
}



#if OPENSSL_VERSION_NUMBER >= 0x30200000L

static EVP_KDF* fetchArgon2id()
{
	ERR_set_mark(); // a missing implementation is not an error
	auto kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
	ERR_pop_to_mark();
	return kdf;
}



bool isArgon2idAvailable()
{
	static const bool available = []() {
		auto kdf = fetchArgon2id();
		EVP_KDF_free(kdf);
		return kdf != nullptr;
	}();
	return available;
}



bool argon2id(const unsigned char* password, size_t passwordSize, const unsigned char* salt, size_t saltSize,
			  const unsigned char* secret, size_t secretSize, const unsigned char* associatedData,
			  size_t associatedDataSize, uint32_t iterations, uint32_t memoryKiB, uint32_t lanes,
			  unsigned char* output, size_t outputSize)
{
	auto kdf = fetchArgon2id();
	auto context = kdf ? EVP_KDF_CTX_new(kdf) : nullptr;
	EVP_KDF_free(kdf);
	if (context == nullptr)
	{
		return false;
	}

	uint32_t threads = 1; // OSSL_PARAM only references the buffers, inputs are never written
	OSSL_PARAM parameters[10];
	int count = 0;
	const auto octets = [&](const char* name, const unsigned char* data, size_t size) {
		parameters[count++] = OSSL_PARAM_construct_octet_string(name, const_cast<unsigned char*>(data), size);
	};
	octets(OSSL_KDF_PARAM_PASSWORD, password, passwordSize);
	octets(OSSL_KDF_PARAM_SALT, salt, saltSize);
	if (secretSize > 0)
	{
		octets(OSSL_KDF_PARAM_SECRET, secret, secretSize);
	}
	if (associatedDataSize > 0)
	{
		octets(OSSL_KDF_PARAM_ARGON2_AD, associatedData, associatedDataSize);
	}
	parameters[count++] = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ITER, &iterations);
	parameters[count++] = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &memoryKiB);
	parameters[count++] = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &lanes);
	parameters[count++] = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_THREADS, &threads);
	parameters[count] = OSSL_PARAM_construct_end();

	const auto success = EVP_KDF_derive(context, output, outputSize, parameters) == 1;
	EVP_KDF_CTX_free(context);
	return success;
}

#else

bool isArgon2idAvailable()
{
	return false;
}



bool argon2id(const unsigned char*, size_t, const unsigned char*, size_t, const unsigned char*, size_t,
			  const unsigned char*, size_t, uint32_t, uint32_t, uint32_t, unsigned char*, size_t)
{
	return false;
}

#endif

} // namespace Veyon::Crypto::OpenSslCompat
