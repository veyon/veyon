// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Hash.h"
#include "OpenSslUtils.h"

namespace Veyon::Crypto {

static const EVP_MD* messageDigest(Hash::Algorithm algorithm)
{
	switch (algorithm)
	{
	case Hash::Algorithm::Sha256: return EVP_sha256();
	case Hash::Algorithm::Sha384: return EVP_sha384();
	case Hash::Algorithm::Sha512: return EVP_sha512();
	}
	return nullptr;
}



std::optional<QByteArray> Hash::digest(Algorithm algorithm, ByteView data)
{
	const auto md = messageDigest(algorithm);
	if (md == nullptr)
	{
		Internal::setError("Hash::digest", "unsupported algorithm");
		return std::nullopt;
	}

	QByteArray result(int(size(algorithm)), 0);
	unsigned int resultSize = 0;
	if (EVP_Digest(data.data(), size_t(data.size()), reinterpret_cast<unsigned char*>(result.data()), &resultSize, md, nullptr) != 1 ||
		resultSize != unsigned(result.size()))
	{
		Internal::setOpenSslError("Hash::digest");
		return std::nullopt;
	}

	return result;
}

} // namespace Veyon::Crypto
