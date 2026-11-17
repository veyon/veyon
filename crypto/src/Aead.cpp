// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Aead.h"
#include "OpenSslUtils.h"

namespace VeyonCrypto {

static const EVP_CIPHER* aeadCipher(Aead::Algorithm algorithm)
{
	switch (algorithm)
	{
	case Aead::Algorithm::ChaCha20Poly1305: return EVP_chacha20_poly1305();
	case Aead::Algorithm::Aes256Gcm: return EVP_aes_256_gcm();
	}
	return nullptr;
}



static Internal::EvpCipherContextPtr initializeAead(const char* operation, Aead::Algorithm algorithm, bool encrypt,
													ByteView key, ByteView nonce, ByteView associatedData)
{
	const auto cipher = aeadCipher(algorithm);
	if (cipher == nullptr)
	{
		Internal::setError(operation, "unsupported algorithm");
		return {};
	}
	if (key.size() != Aead::KeySize || nonce.size() != Aead::NonceSize ||
		Internal::fitsInt(associatedData.size()) == false)
	{
		Internal::setError(operation, "invalid key, nonce or associated data size");
		return {};
	}

	const auto init = encrypt ? EVP_EncryptInit_ex : EVP_DecryptInit_ex;

	Internal::EvpCipherContextPtr context(EVP_CIPHER_CTX_new());
	int outputSize = 0;
	if (context == nullptr ||
		init(context.get(), cipher, nullptr, nullptr, nullptr) != 1 ||
		EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_AEAD_SET_IVLEN, int(Aead::NonceSize), nullptr) <= 0 ||
		init(context.get(), nullptr, nullptr, key.bytes(), nonce.bytes()) != 1 ||
		(associatedData.isEmpty() == false &&
		 (encrypt ? EVP_EncryptUpdate : EVP_DecryptUpdate)(context.get(), nullptr, &outputSize, associatedData.bytes(),
														   int(associatedData.size())) != 1))
	{
		Internal::setOpenSslError(operation);
		return {};
	}

	return context;
}



std::optional<QByteArray> Aead::seal(Algorithm algorithm, ByteView key, ByteView nonce, ByteView plaintext,
									 ByteView associatedData)
{
	static constexpr auto Operation = "Aead::seal";

	if (plaintext.size() > MaximumPlaintextSize)
	{
		Internal::setError(Operation, "plaintext too large");
		return std::nullopt;
	}

	const auto context = initializeAead(Operation, algorithm, true, key, nonce, associatedData);
	if (context == nullptr)
	{
		return std::nullopt;
	}

	QByteArray output(int(plaintext.size() + TagSize), 0);
	auto outputData = reinterpret_cast<unsigned char*>(output.data());
	int updateSize = 0;
	int finalSize = 0;
	unsigned char finalBlock[32];
	if ((plaintext.isEmpty() == false &&
		 EVP_EncryptUpdate(context.get(), outputData, &updateSize, plaintext.bytes(), int(plaintext.size())) != 1) ||
		EVP_EncryptFinal_ex(context.get(), finalBlock, &finalSize) != 1 ||
		finalSize != 0 || updateSize != plaintext.size() ||
		EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_AEAD_GET_TAG, int(TagSize), outputData + plaintext.size()) <= 0)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	return output;
}



std::optional<SecureBuffer> Aead::open(Algorithm algorithm, ByteView key, ByteView nonce, ByteView ciphertextWithTag,
									   ByteView associatedData)
{
	static constexpr auto Operation = "Aead::open";

	if (ciphertextWithTag.size() < TagSize || ciphertextWithTag.size() - TagSize > MaximumPlaintextSize)
	{
		Internal::setError(Operation, "invalid ciphertext size");
		return std::nullopt;
	}

	const auto context = initializeAead(Operation, algorithm, false, key, nonce, associatedData);
	if (context == nullptr)
	{
		return std::nullopt;
	}

	const auto ciphertextSize = ciphertextWithTag.size() - TagSize;
	// EVP_CIPHER_CTX_ctrl() takes a non-const pointer but only reads the tag when decrypting
	auto tag = const_cast<unsigned char*>(ciphertextWithTag.bytes() + ciphertextSize);

	SecureBuffer plaintext(ciphertextSize);
	int updateSize = 0;
	int finalSize = 0;
	if ((ciphertextSize > 0 &&
		 EVP_DecryptUpdate(context.get(), plaintext.bytes(), &updateSize, ciphertextWithTag.bytes(), int(ciphertextSize)) != 1) ||
		EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_AEAD_SET_TAG, int(TagSize), tag) <= 0)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	// tag verification (AEAD stream modes produce no output in the final step); on failure the
	// unauthenticated plaintext is zeroised by SecureBuffer
	unsigned char finalBlock[32];
	if (EVP_DecryptFinal_ex(context.get(), finalBlock, &finalSize) != 1 ||
		finalSize != 0 || updateSize != ciphertextSize)
	{
		ERR_clear_error();
		Internal::setError(Operation, "authentication failed");
		return std::nullopt;
	}

	return plaintext;
}

} // namespace VeyonCrypto
