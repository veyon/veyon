// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "OpenSslUtils.h"
#include "Random.h"

#include <openssl/rand.h>

namespace Veyon::Crypto {

std::optional<SecureBuffer> Random::bytes(qsizetype count)
{
	if (count <= 0 || count > MaximumSize)
	{
		Internal::setError("Random::bytes", "invalid size");
		return std::nullopt;
	}

	SecureBuffer buffer(count);
	if (RAND_bytes(buffer.bytes(), int(count)) != 1)
	{
		Internal::setOpenSslError("Random::bytes");
		return std::nullopt;
	}

	return buffer;
}



std::optional<QString> Random::hex(qsizetype byteCount)
{
	const auto randomBytes = bytes(byteCount);
	if (randomBytes.has_value() == false)
	{
		return std::nullopt;
	}

	return QString::fromLatin1(QByteArray::fromRawData(randomBytes->constData(), int(randomBytes->size())).toHex());
}

} // namespace Veyon::Crypto
