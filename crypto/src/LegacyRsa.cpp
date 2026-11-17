// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LegacyRsa.h"
#include "OpenSslUtils.h"

#include <algorithm>

namespace Veyon::Crypto::LegacyRsa {

static bool isHexDigit(QChar character)
{
	const auto value = character.unicode();
	return (value >= u'0' && value <= u'9') || (value >= u'a' && value <= u'f') || (value >= u'A' && value <= u'F');
}



std::optional<QString> encryptPasswordHex(const LegacyRsaPublicKey& key, ByteView password)
{
	const auto ciphertext = key.encryptOaepSha1(password); // clears lastError() on success
	if (ciphertext.has_value() == false)
	{
		return std::nullopt;
	}

	return QString::fromLatin1(ciphertext->toHex());
}



std::optional<SecureBuffer> decryptPasswordHex(const LegacyRsaPrivateKey& key, const QString& encryptedPassword)
{
	static constexpr auto Operation = "LegacyRsa::decryptPasswordHex";

	if (key.isNull())
	{
		Internal::setError(Operation, "null key");
		return std::nullopt;
	}

	// QByteArray::fromHex() skips invalid characters, so validate first
	const auto modulusSize = (key.bits() + 7) / 8;
	if (encryptedPassword.size() != 2 * modulusSize ||
		std::all_of(encryptedPassword.cbegin(), encryptedPassword.cend(), isHexDigit) == false)
	{
		Internal::setError(Operation, "invalid encoding");
		return std::nullopt;
	}

	return key.decryptOaepSha1(QByteArray::fromHex(encryptedPassword.toLatin1()));
}

} // namespace Veyon::Crypto::LegacyRsa
