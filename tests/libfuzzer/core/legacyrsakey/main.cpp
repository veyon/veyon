// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFile>

#include "Hash.h"
#include "LegacyRsa.h"

// Legacy RSA of VeyonCrypto with untrusted input (key files of Veyon 4.x, public keys received in
// the 4.x logon authentication and private keys received through the Web API, signatures,
// ciphertexts and configuration password strings). The first input byte selects the target.
// Mutated private keys practically never pass the consistency checks, so the private key targets
// exercise the PEM/DER parsers and the checks; the private key operations are fuzzed with the
// built-in key of veyon-core (core/resources/default-pkey.pem).

namespace {

const Veyon::Crypto::LegacyRsaPrivateKey& defaultKey()
{
	static const auto key = []() {
		QFile file(QStringLiteral(":/core/default-pkey.pem"));
		if (file.open(QFile::ReadOnly) == false)
		{
			qFatal("missing default key");
		}
		auto loadedKey = Veyon::Crypto::LegacyRsaPrivateKey::fromPem(file.readAll());
		if (loadedKey.has_value() == false)
		{
			qFatal("invalid default key");
		}
		return std::move(*loadedKey);
	}();
	return key;
}



// input expanded (SHA-512 in counter mode) to exactly size bytes, so that the RSA operations get
// past their size checks
QByteArray expanded(Veyon::Crypto::ByteView input, qsizetype size)
{
	QByteArray result;
	for (char counter = 0; result.size() < size; ++counter)
	{
		QByteArray block(1, counter);
		block.append(input.data(), int(input.size()));
		result += Veyon::Crypto::Hash::digest(Veyon::Crypto::Hash::Algorithm::Sha512, block).value_or(QByteArray(64, 0));
	}
	return result.left(int(size));
}

} // namespace



extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	const Veyon::Crypto::ByteView input(data + 1, qsizetype(size - 1));
	const auto& key = defaultKey();
	const auto modulusSize = qsizetype((key.bits() + 7) / 8);

	switch (data[0] % 6)
	{
	case 0:
		if (const auto privateKey = Veyon::Crypto::LegacyRsaPrivateKey::fromPem(input))
		{
			(void)privateKey->toPem();
			(void)privateKey->publicKey().toPem();
			(void)privateKey->decryptOaepSha1(expanded(input, (privateKey->bits() + 7) / 8));
		}
		break;
	case 1:
		if (const auto publicKey = Veyon::Crypto::LegacyRsaPublicKey::fromPem(input))
		{
			(void)publicKey->toPem();
			(void)Veyon::Crypto::LegacyRsaPublicKey::fromDer(publicKey->toDer());
			(void)publicKey->verifyPkcs1Sha512(input, expanded(input, (publicKey->bits() + 7) / 8));
		}
		break;
	case 2:
		if (const auto publicKey = Veyon::Crypto::LegacyRsaPublicKey::fromDer(input))
		{
			(void)publicKey->toPem();
			(void)publicKey->encryptOaepSha1(input);
		}
		break;
	case 3:
		// raw input (size check) and a modulus-size ciphertext derived from it
		(void)key.decryptOaepSha1(input);
		(void)key.decryptOaepSha1(expanded(input, modulusSize));
		break;
	case 4:
		(void)key.publicKey().verifyPkcs1Sha512(input, expanded(input, modulusSize));
		break;
	default:
		(void)Veyon::Crypto::LegacyRsa::decryptPasswordHex(key, QString::fromLatin1(input.data(), int(input.size())));
		(void)Veyon::Crypto::LegacyRsa::decryptPasswordHex(
			key, QString::fromLatin1(expanded(input, modulusSize).toHex()));
		break;
	}

	return 0;
}
