// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Hkdf.h"
#include "OpenSslUtils.h"

#include <openssl/kdf.h>

namespace VeyonCrypto {

static std::optional<SecureBuffer> deriveHkdf(int mode, ByteView key, ByteView salt, ByteView info, qsizetype outputSize)
{
	static constexpr auto Operation = "Hkdf";

	if (key.isEmpty() || key.size() > Hkdf::MaximumInputKeyMaterialSize ||
		salt.size() > Hkdf::MaximumSaltSize || info.size() > Hkdf::MaximumInfoSize)
	{
		Internal::setError(Operation, "invalid input size");
		return std::nullopt;
	}
	if (outputSize <= 0 || outputSize > Hkdf::MaximumOutputSize)
	{
		Internal::setError(Operation, "invalid output size");
		return std::nullopt;
	}

	Internal::EvpPkeyContextPtr context(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr));
	if (context == nullptr ||
		EVP_PKEY_derive_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_hkdf_mode(context.get(), mode) <= 0 ||
		EVP_PKEY_CTX_set_hkdf_md(context.get(), EVP_sha256()) <= 0 ||
		EVP_PKEY_CTX_set1_hkdf_key(context.get(), key.bytes(), int(key.size())) <= 0 ||
		(salt.isEmpty() == false && EVP_PKEY_CTX_set1_hkdf_salt(context.get(), salt.bytes(), int(salt.size())) <= 0) ||
		(info.isEmpty() == false && EVP_PKEY_CTX_add1_hkdf_info(context.get(), info.bytes(), int(info.size())) <= 0))
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	SecureBuffer output(outputSize);
	auto derivedSize = size_t(output.size());
	if (EVP_PKEY_derive(context.get(), output.bytes(), &derivedSize) != 1 || derivedSize != size_t(outputSize))
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return output;
}



std::optional<SecureBuffer> Hkdf::sha256(ByteView inputKeyMaterial, ByteView salt, ByteView info, qsizetype outputSize)
{
	return deriveHkdf(EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND, inputKeyMaterial, salt, info, outputSize);
}



std::optional<SecureBuffer> Hkdf::extractSha256(ByteView inputKeyMaterial, ByteView salt)
{
	return deriveHkdf(EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY, inputKeyMaterial, salt, {}, PseudoRandomKeySize);
}



std::optional<SecureBuffer> Hkdf::expandSha256(ByteView pseudoRandomKey, ByteView info, qsizetype outputSize)
{
	if (pseudoRandomKey.size() < PseudoRandomKeySize)
	{
		Internal::setError("Hkdf::expandSha256", "pseudo-random key too short");
		return std::nullopt;
	}

	return deriveHkdf(EVP_PKEY_HKDEF_MODE_EXPAND_ONLY, pseudoRandomKey, {}, info, outputSize);
}

} // namespace VeyonCrypto
