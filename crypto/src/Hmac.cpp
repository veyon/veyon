// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ConstantTime.h"
#include "Hmac.h"
#include "OpenSslUtils.h"

namespace Veyon::Crypto {

std::optional<QByteArray> Hmac::sha256(ByteView key, ByteView message)
{
	if (key.isEmpty())
	{
		Internal::setError("Hmac::sha256", "empty key");
		return std::nullopt;
	}

	// EVP_PKEY based HMAC: identical in OpenSSL 1.1.1, 3.x and 4.x (HMAC_* is deprecated, EVP_MAC 3.x only)
	Internal::EvpPkeyPtr hmacKey(EVP_PKEY_new_raw_private_key(EVP_PKEY_HMAC, nullptr, key.bytes(), size_t(key.size())));
	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	if (hmacKey == nullptr || context == nullptr ||
		EVP_DigestSignInit(context.get(), nullptr, EVP_sha256(), nullptr, hmacKey.get()) != 1)
	{
		Internal::setOpenSslError("Hmac::sha256");
		return std::nullopt;
	}

	QByteArray mac(int(Sha256Size), 0);
	auto macSize = size_t(mac.size());
	if (EVP_DigestSign(context.get(), reinterpret_cast<unsigned char*>(mac.data()), &macSize, message.bytes(), size_t(message.size())) != 1 ||
		macSize != size_t(Sha256Size))
	{
		Internal::setOpenSslError("Hmac::sha256");
		return std::nullopt;
	}

	return mac;
}



bool Hmac::verifySha256(ByteView key, ByteView message, ByteView expectedMac)
{
	const auto mac = sha256(key, message);
	return mac.has_value() && constantTimeEquals(*mac, expectedMac);
}

} // namespace Veyon::Crypto
