// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

// Known-answer tests of Veyon::Crypto::LegacyRsa against the QCA-based RSA code of Veyon <= 4.11.
//
// The fixtures in data/legacyrsa were produced with QCA 2.3.8 (qca-ossl plugin, OpenSSL 3.0.13),
// using QCA exactly like Veyon 4.11 does:
// - key4096-*.pem, key2048-*.pem: KeyGenerator().createRSA(bits), PrivateKey::toPEMFile() and
//   PublicKey::toPEMFile() (the key files of AuthKeysManager::createKeyPair()); key4096-public.der:
//   PublicKey::toDER()
// - signatures*.txt: "<hex message> <hex signature>" per line, PrivateKey::signMessage() with
//   EMSA3_SHA512 (VeyonConnection) over random 128-byte challenges and other messages
// - ciphertexts4096.txt: "<hex plaintext> <hex ciphertext>", PublicKey::encrypt() with
//   EME_PKCS1_OAEP to the key4096 public key read with PublicKey::fromPEM() (logon authentication)
// - obfuscated-passwords.txt: "<hex plaintext> <encryptPassword() output>" with the public key of
//   core/resources/default-pkey.pem (configuration password obfuscation)
// - key4096-pkcs1-*.pem: the same key as PKCS#1 ("openssl rsa -traditional" / "-RSAPublicKey_out"),
//   both read by QCA; the other negative keys were made with the openssl CLI or crafted by hand
//   (key2048-e*-public.pem: the key2048 modulus with exponents 1, 65536 and 2^32 + 1).
// The reverse direction (QCA reading/verifying/decrypting VeyonCrypto output) was checked with the
// same generator outside the repository, so this test does not depend on QCA.

#include <QCryptographicHash>
#include <QFile>
#include <QtTest>

#include "CryptoError.h"
#include "LegacyRsa.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

using namespace Veyon::Crypto;

namespace {

QByteArray fixture(const char* name)
{
	QFile file(QStringLiteral(":/legacyrsa/") + QLatin1String(name));
	if (file.open(QFile::ReadOnly) == false)
	{
		qFatal("missing fixture %s", name);
	}
	return file.readAll();
}



struct Vector
{
	QByteArray message;
	QByteArray result;
};

QList<Vector> vectors(const char* name)
{
	QList<Vector> result;
	const auto lines = fixture(name).split('\n');
	for (const auto& line : lines)
	{
		if (line.isEmpty())
		{
			continue;
		}
		const auto parts = line.split(' ');
		if (parts.size() != 2)
		{
			qFatal("malformed fixture %s", name);
		}
		result.append({QByteArray::fromHex(parts[0]), parts[1]});
	}
	return result;
}



LegacyRsaPrivateKey privateKey(const char* name)
{
	auto key = LegacyRsaPrivateKey::fromPem(fixture(name));
	if (key.has_value() == false)
	{
		qFatal("failed to load %s: %s", name, qPrintable(lastError()));
	}
	return std::move(*key);
}



LegacyRsaPublicKey publicKey(const char* name)
{
	const auto key = LegacyRsaPublicKey::fromPem(fixture(name));
	if (key.has_value() == false)
	{
		qFatal("failed to load %s: %s", name, qPrintable(lastError()));
	}
	return *key;
}



QByteArray toByteArray(const std::optional<SecureBuffer>& buffer)
{
	return buffer.has_value() ? buffer->toUnprotectedByteArray() : QByteArray{};
}



QByteArray pem(const char* label, const QByteArray& der)
{
	QByteArray result = QByteArrayLiteral("-----BEGIN ") + label + QByteArrayLiteral("-----\n");
	const auto base64 = der.toBase64();
	for (int offset = 0; offset < base64.size(); offset += 64)
	{
		result += base64.mid(offset, 64) + '\n';
	}
	result += QByteArrayLiteral("-----END ") + label + QByteArrayLiteral("-----\n");
	return result;
}



QByteArray pemContent(const QByteArray& pemData)
{
	QByteArray base64;
	const auto lines = pemData.split('\n');
	for (const auto& line : lines)
	{
		if (line.startsWith("-----") == false)
		{
			base64 += line.trimmed();
		}
	}
	return QByteArray::fromBase64(base64);
}



// OpenSSL directly, for signatures and ciphertexts with other paddings than the legacy ones
using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

EvpPkeyPtr opensslPrivateKey(const char* name)
{
	const auto data = fixture(name);
	BIO* bio = BIO_new_mem_buf(data.constData(), data.size());
	EvpPkeyPtr key(PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr), &EVP_PKEY_free);
	BIO_free(bio);
	return key;
}



QByteArray opensslSign(const char* keyName, const EVP_MD* digest, int padding, const QByteArray& message)
{
	const auto key = opensslPrivateKey(keyName);
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
	EVP_PKEY_CTX* keyContext = nullptr;
	size_t size = 0;
	if (key == nullptr ||
		EVP_DigestSignInit(context.get(), &keyContext, digest, nullptr, key.get()) != 1 ||
		EVP_PKEY_CTX_set_rsa_padding(keyContext, padding) <= 0 ||
		EVP_DigestSign(context.get(), nullptr, &size, reinterpret_cast<const unsigned char*>(message.constData()),
					   size_t(message.size())) != 1)
	{
		return {};
	}
	QByteArray signature(int(size), 0);
	if (EVP_DigestSign(context.get(), reinterpret_cast<unsigned char*>(signature.data()), &size,
					   reinterpret_cast<const unsigned char*>(message.constData()), size_t(message.size())) != 1)
	{
		return {};
	}
	return signature;
}



QByteArray opensslEncrypt(const char* keyName, int padding, const EVP_MD* oaepDigest, const QByteArray& plaintext)
{
	const auto key = opensslPrivateKey(keyName);
	std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
		key ? EVP_PKEY_CTX_new(key.get(), nullptr) : nullptr, &EVP_PKEY_CTX_free);
	size_t size = 0;
	if (context == nullptr ||
		EVP_PKEY_encrypt_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_padding(context.get(), padding) <= 0 ||
		(oaepDigest && EVP_PKEY_CTX_set_rsa_oaep_md(context.get(), oaepDigest) <= 0) ||
		EVP_PKEY_encrypt(context.get(), nullptr, &size, reinterpret_cast<const unsigned char*>(plaintext.constData()),
						 size_t(plaintext.size())) <= 0)
	{
		return {};
	}
	QByteArray ciphertext(int(size), 0);
	if (EVP_PKEY_encrypt(context.get(), reinterpret_cast<unsigned char*>(ciphertext.data()), &size,
						 reinterpret_cast<const unsigned char*>(plaintext.constData()), size_t(plaintext.size())) <= 0)
	{
		return {};
	}
	return ciphertext;
}

// raw RSA private key operation (no padding) on an encoded message of the modulus size, for
// signatures with crafted encodings
QByteArray opensslRawPrivate(const char* keyName, const QByteArray& encodedMessage)
{
	const auto key = opensslPrivateKey(keyName);
	std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
		key ? EVP_PKEY_CTX_new(key.get(), nullptr) : nullptr, &EVP_PKEY_CTX_free);
	size_t size = size_t(encodedMessage.size());
	QByteArray result(encodedMessage.size(), 0);
	if (context == nullptr ||
		EVP_PKEY_sign_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_padding(context.get(), RSA_NO_PADDING) <= 0 ||
		EVP_PKEY_sign(context.get(), reinterpret_cast<unsigned char*>(result.data()), &size,
					  reinterpret_cast<const unsigned char*>(encodedMessage.constData()), size_t(encodedMessage.size())) <= 0)
	{
		return {};
	}
	return result;
}



// EMSA-PKCS1-v1_5 encoding 00 01 FF..FF 00 <digestInfo> <trailer> of the given size
QByteArray pkcs1EncodedMessage(int size, const QByteArray& digestInfo, const QByteArray& trailer = {})
{
	const auto paddingSize = size - 3 - digestInfo.size() - trailer.size();
	return QByteArray::fromHex("0001") + QByteArray(paddingSize, '\xff') + QByteArray(1, '\0') + digestInfo + trailer;
}



QByteArray opensslEncryptWithLabel(const char* keyName, const QByteArray& label, const QByteArray& plaintext)
{
	const auto key = opensslPrivateKey(keyName);
	std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
		key ? EVP_PKEY_CTX_new(key.get(), nullptr) : nullptr, &EVP_PKEY_CTX_free);
	// set0: the context takes ownership of the label buffer
	auto labelCopy = static_cast<unsigned char*>(OPENSSL_memdup(label.constData(), size_t(label.size())));
	size_t size = 512;
	QByteArray ciphertext(int(size), 0);
	if (context == nullptr || labelCopy == nullptr ||
		EVP_PKEY_encrypt_init(context.get()) <= 0 ||
		EVP_PKEY_CTX_set_rsa_padding(context.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
		EVP_PKEY_CTX_set0_rsa_oaep_label(context.get(), labelCopy, int(label.size())) <= 0 ||
		EVP_PKEY_encrypt(context.get(), reinterpret_cast<unsigned char*>(ciphertext.data()), &size,
						 reinterpret_cast<const unsigned char*>(plaintext.constData()), size_t(plaintext.size())) <= 0)
	{
		return {};
	}
	ciphertext.truncate(int(size));
	return ciphertext;
}

} // namespace



class VeyonCryptoLegacyRsaTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void keyFilesImportAndExport_data();
	void keyFilesImportAndExport();
	void pkcs1KeyFiles();
	void lenientPemFraming();

	void verifiesQcaSignatures_data();
	void verifiesQcaSignatures();
	void decryptsQcaCiphertexts();
	void decryptsQcaObfuscatedPasswords();

	void encryptionRoundTrip();
	void generatedKey();

	void rejectsInvalidSignatures();
	void rejectsInvalidCiphertexts();
	void rejectsInvalidObfuscatedPasswords();
	void rejectsKeys_data();
	void rejectsKeys();
	void rejectsPublicKeyDer();
	void rejectsInconsistentPrivateKey();
	void acceptsBoundaryKeys_data();
	void acceptsBoundaryKeys();
	void oddModulusSize();
	void rejectsCraftedSignatures();
	void rejectsOaepLabel();
	void successClearsLastError();
	void nullKeys();
	void copyAndMove();
};



void VeyonCryptoLegacyRsaTest::keyFilesImportAndExport_data()
{
	QTest::addColumn<QByteArray>("privateKeyFile");
	QTest::addColumn<QByteArray>("publicKeyFile");
	QTest::addColumn<int>("bits");

	QTest::newRow("4096") << QByteArray("key4096-private.pem") << QByteArray("key4096-public.pem") << 4096;
	QTest::newRow("2048") << QByteArray("key2048-private.pem") << QByteArray("key2048-public.pem") << 2048;
}



void VeyonCryptoLegacyRsaTest::keyFilesImportAndExport()
{
	QFETCH(QByteArray, privateKeyFile);
	QFETCH(QByteArray, publicKeyFile);
	QFETCH(int, bits);

	const auto privateKeyPem = fixture(privateKeyFile.constData());
	const auto publicKeyPem = fixture(publicKeyFile.constData());

	const auto loadedPrivateKey = LegacyRsaPrivateKey::fromPem(privateKeyPem);
	QVERIFY2(loadedPrivateKey.has_value(), qPrintable(lastError()));
	const auto loadedPublicKey = LegacyRsaPublicKey::fromPem(publicKeyPem);
	QVERIFY2(loadedPublicKey.has_value(), qPrintable(lastError()));

	QCOMPARE(loadedPrivateKey->bits(), bits);
	QCOMPARE(loadedPublicKey->bits(), bits);
	QVERIFY(loadedPrivateKey->publicKey() == *loadedPublicKey);

	// files written by VeyonCrypto are byte-identical to the 4.x key files
	QCOMPARE(toByteArray(loadedPrivateKey->toPem()), privateKeyPem);
	QCOMPARE(loadedPublicKey->toPem(), publicKeyPem);
	QCOMPARE(loadedPrivateKey->publicKey().toPem(), publicKeyPem);
	QCOMPARE(LegacyRsaPublicKey::fromDer(loadedPublicKey->toDer())->toPem(), publicKeyPem);

	if (bits == 4096)
	{
		// DER of the public key as in 4.x (e.g. AuthKeysManager::keyPairId())
		QCOMPARE(loadedPublicKey->toDer(), fixture("key4096-public.der"));
	}
}



void VeyonCryptoLegacyRsaTest::pkcs1KeyFiles()
{
	const auto pkcs1PrivateKey = privateKey("key4096-pkcs1-private.pem");
	const auto pkcs1PublicKey = publicKey("key4096-pkcs1-public.pem");

	QVERIFY(pkcs1PrivateKey.publicKey() == publicKey("key4096-public.pem"));
	QVERIFY(pkcs1PublicKey == publicKey("key4096-public.pem"));

	// written in the 4.x format (PKCS#8 / SubjectPublicKeyInfo) only
	QCOMPARE(toByteArray(pkcs1PrivateKey.toPem()), fixture("key4096-private.pem"));
	QCOMPARE(pkcs1PublicKey.toPem(), fixture("key4096-public.pem"));

	// a key with exponent 3 (openssl genrsa -3) is read by 4.x and accepted
	QVERIFY(LegacyRsaPrivateKey::fromPem(fixture("key2048-e3-private.pem")).has_value());
}



void VeyonCryptoLegacyRsaTest::lenientPemFraming()
{
	// as OpenSSL/4.x: text around the block, CRLF line endings and blocks with other labels
	const auto privateKeyPem = fixture("key4096-private.pem");
	const auto publicKeyPem = fixture("key4096-public.pem");

	QByteArray withText = QByteArrayLiteral("comment\n") + privateKeyPem + QByteArrayLiteral("trailing text\n");
	QVERIFY(LegacyRsaPrivateKey::fromPem(withText).has_value());

	QByteArray crlf = privateKeyPem;
	crlf.replace("\n", "\r\n");
	QVERIFY(LegacyRsaPrivateKey::fromPem(crlf).has_value());

	// a public key block next to the private key and vice versa
	QByteArray combined = publicKeyPem + privateKeyPem;
	QVERIFY(LegacyRsaPrivateKey::fromPem(combined).has_value());
	QVERIFY(LegacyRsaPublicKey::fromPem(combined).has_value());

	// a block without content and one with more DER than any accepted key
	QVERIFY(LegacyRsaPrivateKey::fromPem(QByteArrayLiteral("-----BEGIN PRIVATE KEY-----\n-----END PRIVATE KEY-----\n")).has_value() == false);
	QVERIFY(LegacyRsaPublicKey::fromPem(QByteArrayLiteral("-----BEGIN PUBLIC KEY-----\n-----END PUBLIC KEY-----\n")).has_value() == false);
	QVERIFY(LegacyRsaPrivateKey::fromPem(pem("PRIVATE KEY", QByteArray(LegacyRsaPrivateKey::MaximumEncodedSize + 1, 'x'))).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("invalid size")), qPrintable(lastError()));
}



void VeyonCryptoLegacyRsaTest::verifiesQcaSignatures_data()
{
	QTest::addColumn<QByteArray>("privateKeyFile");
	QTest::addColumn<QByteArray>("publicKeyFile");
	QTest::addColumn<QByteArray>("signatureFile");

	QTest::newRow("4096") << QByteArray("key4096-private.pem") << QByteArray("key4096-public.pem")
						  << QByteArray("signatures4096.txt");
	QTest::newRow("2048") << QByteArray("key2048-private.pem") << QByteArray("key2048-public.pem")
						  << QByteArray("signatures2048.txt");
}



void VeyonCryptoLegacyRsaTest::verifiesQcaSignatures()
{
	QFETCH(QByteArray, privateKeyFile);
	QFETCH(QByteArray, publicKeyFile);
	QFETCH(QByteArray, signatureFile);

	const auto signingKey = privateKey(privateKeyFile.constData());
	const auto verificationKey = publicKey(publicKeyFile.constData());

	const auto signatureVectors = vectors(signatureFile.constData());
	QCOMPARE(signatureVectors.size(), 4);
	for (const auto& vector : signatureVectors)
	{
		const auto qcaSignature = QByteArray::fromHex(vector.result);
		QVERIFY2(verificationKey.verifyPkcs1Sha512(vector.message, qcaSignature), qPrintable(lastError()));

		// PKCS#1 v1.5 signatures are deterministic: VeyonCrypto produces the identical bytes
		const auto signature = signingKey.signPkcs1Sha512(vector.message);
		QVERIFY(signature.has_value());
		QCOMPARE(signature->toHex(), vector.result);
	}
}



void VeyonCryptoLegacyRsaTest::decryptsQcaCiphertexts()
{
	const auto key = privateKey("key4096-private.pem");

	const auto ciphertextVectors = vectors("ciphertexts4096.txt");
	QCOMPARE(ciphertextVectors.size(), 4);
	for (const auto& vector : ciphertextVectors)
	{
		const auto plaintext = key.decryptOaepSha1(QByteArray::fromHex(vector.result));
		QVERIFY2(plaintext.has_value(), qPrintable(lastError()));
		QCOMPARE(plaintext->toUnprotectedByteArray(), vector.message);
	}
}



void VeyonCryptoLegacyRsaTest::decryptsQcaObfuscatedPasswords()
{
	// the built-in key of veyon-core (core/resources/default-pkey.pem), as used by 4.x for the
	// configuration passwords
	QFile defaultKeyFile(QStringLiteral(":/legacyrsa/default-pkey.pem"));
	QVERIFY(defaultKeyFile.open(QFile::ReadOnly));
	const auto defaultKey = LegacyRsaPrivateKey::fromPem(defaultKeyFile.readAll());
	QVERIFY2(defaultKey.has_value(), qPrintable(lastError()));
	QCOMPARE(defaultKey->bits(), 4096);

	const auto passwordVectors = vectors("obfuscated-passwords.txt");
	QCOMPARE(passwordVectors.size(), 4);
	for (const auto& vector : passwordVectors)
	{
		const auto encryptedPassword = QString::fromLatin1(vector.result);
		const auto password = LegacyRsa::decryptPasswordHex(*defaultKey, encryptedPassword);
		QVERIFY2(password.has_value(), qPrintable(lastError()));
		QCOMPARE(password->toUnprotectedByteArray(), vector.message);

		// upper-case hex digits are accepted as well
		QCOMPARE(toByteArray(LegacyRsa::decryptPasswordHex(*defaultKey, encryptedPassword.toUpper())), vector.message);

		// and the reverse direction has the 4.x format (lower-case hex of the modulus size)
		const auto reencrypted = LegacyRsa::encryptPasswordHex(defaultKey->publicKey(), vector.message);
		QVERIFY(reencrypted.has_value());
		QCOMPARE(reencrypted->size(), 1024);
		QCOMPARE(*reencrypted, reencrypted->toLower());
		QVERIFY(*reencrypted != encryptedPassword);
		QCOMPARE(toByteArray(LegacyRsa::decryptPasswordHex(*defaultKey, *reencrypted)), vector.message);
	}
}



void VeyonCryptoLegacyRsaTest::encryptionRoundTrip()
{
	const auto key = privateKey("key2048-private.pem");
	const auto encryptionKey = key.publicKey();
	QCOMPARE(encryptionKey.maximumOaepSha1PlaintextSize(), qsizetype(256 - 42));

	for (const auto size : {0, 1, 32, 256 - 42})
	{
		const QByteArray plaintext(size, 'p');
		const auto ciphertext = encryptionKey.encryptOaepSha1(plaintext);
		QVERIFY2(ciphertext.has_value(), qPrintable(lastError()));
		QCOMPARE(ciphertext->size(), 256);
		QCOMPARE(toByteArray(key.decryptOaepSha1(*ciphertext)), plaintext);
	}

	// no silent truncation as in 4.x
	QVERIFY(encryptionKey.encryptOaepSha1(QByteArray(256 - 41, 'p')).has_value() == false);
	QVERIFY(lastError().contains(QLatin1String("plaintext too long")));
}



void VeyonCryptoLegacyRsaTest::generatedKey()
{
	const auto key = LegacyRsaPrivateKey::generate();
	QVERIFY2(key.has_value(), qPrintable(lastError()));
	QCOMPARE(key->bits(), 4096);

	// public exponent 65537 at the end of the SubjectPublicKeyInfo, as in 4.x
	QVERIFY(key->publicKey().toDer().endsWith(QByteArray::fromHex("0203010001")));

	const auto pemData = key->toPem();
	QVERIFY(pemData.has_value());
	QVERIFY(pemData->toUnprotectedByteArray().startsWith("-----BEGIN PRIVATE KEY-----\n"));
	const auto reloaded = LegacyRsaPrivateKey::fromPem(*pemData);
	QVERIFY2(reloaded.has_value(), qPrintable(lastError()));
	QVERIFY(reloaded->publicKey() == key->publicKey());

	const QByteArray challenge(128, 'c');
	const auto signature = key->signPkcs1Sha512(challenge);
	QVERIFY(signature.has_value());
	QCOMPARE(signature->size(), 512);
	QVERIFY(reloaded->publicKey().verifyPkcs1Sha512(challenge, *signature));

	QVERIFY(LegacyRsaPrivateKey::generate()->publicKey() != key->publicKey());
}



void VeyonCryptoLegacyRsaTest::rejectsInvalidSignatures()
{
	const auto key = publicKey("key4096-public.pem");
	const auto otherKey = publicKey("key2048-public.pem");
	const auto vector = vectors("signatures4096.txt").first();
	const auto signature = QByteArray::fromHex(vector.result);

	QVERIFY(key.verifyPkcs1Sha512(vector.message, signature));

	// wrong key (same and other size)
	const auto otherKeyOfSameSize = LegacyRsaPrivateKey::fromPem(fixture("key2048-e3-private.pem"));
	QVERIFY(otherKey.verifyPkcs1Sha512(vector.message, signature) == false);
	QVERIFY(otherKeyOfSameSize->publicKey().verifyPkcs1Sha512(
				vector.message, *privateKey("key2048-private.pem").signPkcs1Sha512(vector.message)) == false);

	// other message
	QByteArray otherMessage = vector.message;
	otherMessage[0] = char(otherMessage[0] ^ 1);
	QVERIFY(key.verifyPkcs1Sha512(otherMessage, signature) == false);

	// tampered signature
	for (const auto position : {0, 100, 511})
	{
		QByteArray tampered = signature;
		tampered[position] = char(tampered[position] ^ 0x40);
		QVERIFY(key.verifyPkcs1Sha512(vector.message, tampered) == false);
	}

	// not exactly the modulus size
	QVERIFY(key.verifyPkcs1Sha512(vector.message, {}) == false);
	QVERIFY(key.verifyPkcs1Sha512(vector.message, signature.left(511)) == false);
	QVERIFY(key.verifyPkcs1Sha512(vector.message, QByteArray(signature + '\0')) == false);
	QVERIFY(key.verifyPkcs1Sha512(vector.message, QByteArray(QByteArray(1, '\0') + signature)) == false);

	// other schemes with the right key
	const QByteArray message("challenge");
	const auto pssSignature = opensslSign("key4096-private.pem", EVP_sha512(), RSA_PKCS1_PSS_PADDING, message);
	const auto sha256Signature = opensslSign("key4096-private.pem", EVP_sha256(), RSA_PKCS1_PADDING, message);
	const auto sha512Signature = opensslSign("key4096-private.pem", EVP_sha512(), RSA_PKCS1_PADDING, message);
	QCOMPARE(pssSignature.size(), 512);
	QCOMPARE(sha256Signature.size(), 512);
	QVERIFY(key.verifyPkcs1Sha512(message, pssSignature) == false);
	QVERIFY(key.verifyPkcs1Sha512(message, sha256Signature) == false);
	QVERIFY(key.verifyPkcs1Sha512(message, sha512Signature));
}



void VeyonCryptoLegacyRsaTest::rejectsInvalidCiphertexts()
{
	const auto key = privateKey("key4096-private.pem");
	const auto ciphertext = QByteArray::fromHex(vectors("ciphertexts4096.txt").first().result);
	QVERIFY(key.decryptOaepSha1(ciphertext).has_value());

	const auto failureText = QStringLiteral("LegacyRsaPrivateKey::decryptOaepSha1: decryption failed");

	QList<QByteArray> invalidCiphertexts;
	for (const auto position : {0, 1, 255, 511})
	{
		QByteArray tampered = ciphertext;
		tampered[position] = char(tampered[position] ^ 0x01);
		invalidCiphertexts.append(tampered);
	}
	invalidCiphertexts.append(QByteArray());
	invalidCiphertexts.append(ciphertext.left(511));
	invalidCiphertexts.append(ciphertext + '\0');
	invalidCiphertexts.append(QByteArray(512, '\0'));
	invalidCiphertexts.append(QByteArray(512, '\xff'));
	// for another key
	invalidCiphertexts.append(*privateKey("key2048-private.pem").publicKey().encryptOaepSha1(QByteArray("x")));
	invalidCiphertexts.append(*LegacyRsaPublicKey::fromPem(fixture("key2048-public.pem"))->encryptOaepSha1(QByteArray("x")));
	// other paddings with the right key
	const QByteArray plaintext("password");
	const auto pkcs1Ciphertext = opensslEncrypt("key4096-private.pem", RSA_PKCS1_PADDING, nullptr, plaintext);
	const auto oaepSha256Ciphertext = opensslEncrypt("key4096-private.pem", RSA_PKCS1_OAEP_PADDING, EVP_sha256(), plaintext);
	QCOMPARE(pkcs1Ciphertext.size(), 512);
	QCOMPARE(oaepSha256Ciphertext.size(), 512);
	invalidCiphertexts.append(pkcs1Ciphertext);
	invalidCiphertexts.append(oaepSha256Ciphertext);

	for (const auto& invalidCiphertext : std::as_const(invalidCiphertexts))
	{
		clearLastError();
		QVERIFY(key.decryptOaepSha1(invalidCiphertext).has_value() == false);
		// one error text for every failure (no padding oracle)
		QCOMPARE(lastError(), failureText);
	}

	QCOMPARE(toByteArray(key.decryptOaepSha1(opensslEncrypt("key4096-private.pem", RSA_PKCS1_OAEP_PADDING, EVP_sha1(), plaintext))),
			 plaintext);
}



void VeyonCryptoLegacyRsaTest::rejectsInvalidObfuscatedPasswords()
{
	const auto key = privateKey("key4096-private.pem");
	const auto valid = LegacyRsa::encryptPasswordHex(key.publicKey(), QByteArray("password"));
	QVERIFY(valid.has_value());
	QCOMPARE(toByteArray(LegacyRsa::decryptPasswordHex(key, *valid)), QByteArray("password"));

	const QStringList invalidValues{
		QString{},
		valid->left(1022),
		*valid + QStringLiteral("00"),
		*valid + QLatin1Char('0'),
		valid->left(1023) + QLatin1Char('g'),
		QLatin1Char(' ') + valid->left(1023),
		valid->left(1023) + QLatin1Char('\n'),
		valid->left(1022) + QStringLiteral("ää"),
	};
	for (const auto& invalidValue : invalidValues)
	{
		QVERIFY(LegacyRsa::decryptPasswordHex(key, invalidValue).has_value() == false);
	}

	QVERIFY(LegacyRsa::decryptPasswordHex(LegacyRsaPrivateKey{}, *valid).has_value() == false);
	QVERIFY(LegacyRsa::encryptPasswordHex(LegacyRsaPublicKey{}, QByteArray("password")).has_value() == false);
}



void VeyonCryptoLegacyRsaTest::rejectsKeys_data()
{
	QTest::addColumn<QByteArray>("data");
	QTest::addColumn<bool>("isPrivate");

	const auto privateKeyPem = fixture("key4096-private.pem");
	const auto publicKeyPem = fixture("key4096-public.pem");
	const auto privateKeyDer = pemContent(privateKeyPem);
	const auto publicKeyDer = pemContent(publicKeyPem);

	QTest::newRow("private 1024 bits") << fixture("key1024-private.pem") << true;
	QTest::newRow("public 1024 bits") << fixture("key1024-public.pem") << false;
	QTest::newRow("public exponent 1") << fixture("key2048-e1-public.pem") << false;
	QTest::newRow("public exponent 65536") << fixture("key2048-e65536-public.pem") << false;
	QTest::newRow("public exponent 2^32+1") << fixture("key2048-elarge-public.pem") << false;
	QTest::newRow("private RSASSA-PSS") << fixture("key2048-rsapss-private.pem") << true;
	QTest::newRow("public RSASSA-PSS") << fixture("key2048-rsapss-public.pem") << false;
	QTest::newRow("private EC") << fixture("ec-private.pem") << true;
	QTest::newRow("public EC") << fixture("ec-public.pem") << false;
	QTest::newRow("encrypted PKCS#8") << fixture("key2048-encrypted-pkcs8.pem") << true;
	QTest::newRow("encrypted PKCS#1") << fixture("key2048-encrypted-pkcs1.pem") << true;

	QTest::newRow("private: two keys") << (privateKeyPem + fixture("key2048-private.pem")) << true;
	QTest::newRow("private: PKCS#8 and PKCS#1") << (privateKeyPem + fixture("key4096-pkcs1-private.pem")) << true;
	QTest::newRow("public: two keys") << (publicKeyPem + fixture("key2048-public.pem")) << false;
	QTest::newRow("public: SPKI and PKCS#1") << (fixture("key4096-pkcs1-public.pem") + publicKeyPem) << false;

	QTest::newRow("private: public key file") << publicKeyPem << true;
	QTest::newRow("public: private key file") << privateKeyPem << false;
	QTest::newRow("private: empty") << QByteArray() << true;
	QTest::newRow("public: empty") << QByteArray() << false;
	QTest::newRow("private: DER") << privateKeyDer << true;
	QTest::newRow("public: DER") << publicKeyDer << false;
	QTest::newRow("private: oversized") << QByteArray(64 * 1024, 'A') << true;
	QTest::newRow("public: oversized") << QByteArray(64 * 1024, 'A') << false;

	QByteArray truncated = privateKeyPem;
	truncated.remove(privateKeyPem.size() / 2, 200);
	QTest::newRow("private: truncated") << truncated << true;
	truncated = publicKeyPem;
	truncated.remove(publicKeyPem.size() / 2, 100);
	QTest::newRow("public: truncated") << truncated << false;

	QByteArray brokenBase64 = privateKeyPem;
	brokenBase64[100] = '*';
	QTest::newRow("private: invalid base64") << brokenBase64 << true;
	brokenBase64 = publicKeyPem;
	brokenBase64[100] = '*';
	QTest::newRow("public: invalid base64") << brokenBase64 << false;

	QTest::newRow("private: trailing DER data") << pem("PRIVATE KEY", privateKeyDer + '\0') << true;
	QTest::newRow("public: trailing DER data") << pem("PUBLIC KEY", publicKeyDer + '\0') << false;
	QTest::newRow("private: PKCS#8 as PKCS#1") << pem("RSA PRIVATE KEY", privateKeyDer) << true;
	QTest::newRow("private: PKCS#1 as PKCS#8")
		<< pem("PRIVATE KEY", pemContent(fixture("key4096-pkcs1-private.pem"))) << true;
	QTest::newRow("public: PKCS#1 as SPKI") << pem("PUBLIC KEY", pemContent(fixture("key4096-pkcs1-public.pem"))) << false;
	QTest::newRow("public: SPKI as PKCS#1") << pem("RSA PUBLIC KEY", publicKeyDer) << false;

	// non-DER length encoding (BER long form for a short length) inside the SubjectPublicKeyInfo
	QByteArray berPublicKey = publicKeyDer;
	QVERIFY(berPublicKey.startsWith(QByteArray::fromHex("30820222300d06")));
	berPublicKey.replace(4, 2, QByteArray::fromHex("30810d"));
	berPublicKey[3] = char(berPublicKey[3] + 1);
	QTest::newRow("public: BER") << pem("PUBLIC KEY", berPublicKey) << false;

	QByteArray withHeaders = privateKeyPem;
	withHeaders.insert(int(qstrlen("-----BEGIN PRIVATE KEY-----\n")), "Comment: test\n\n");
	QTest::newRow("private: RFC 1421 header") << withHeaders << true;
	withHeaders = publicKeyPem;
	withHeaders.insert(int(qstrlen("-----BEGIN PUBLIC KEY-----\n")), "Comment: test\n\n");
	QTest::newRow("public: RFC 1421 header") << withHeaders << false;

	// crafted by hand (policy, strict encoding, consistency of the private components)
	const QList<QByteArray> privateFixtures{
		"crafted-k2047.pem",
		"crafted-bogus4096.pem",
		"crafted-bogus8192.pem",
		"crafted-bogus8194.pem",
		"crafted-zero-crt.pem",
		"crafted-zero-crt-pkcs8.pem",
		"crafted-bad-crt-good-d.pem",
		"crafted-good-crt-bad-d.pem",
		"crafted-swapped-pq.pem",
		"crafted-wrong-e3.pem",
		"crafted-wrong-n-pq-other.pem",
		"crafted-missing-crt-fields.pem",
		"crafted-version1-no-other.pem",
		"crafted-neg-d.pem",
		"crafted-mp3-pkcs1.pem",
		"crafted-mp3-pkcs8.pem",
		"crafted-p8-attrs.pem",
		"crafted-p8-v1-pub.pem",
		"crafted-p8-noparams.pem",
		"crafted-p8-ber-len.pem",
		"crafted-p8-indef.pem",
		"crafted-p8-inner-trailing.pem",
		"crafted-p8-inner-ber.pem",
		"crafted-p8-oid-rsassa-sha256.pem",
		"crafted-p8-oid-oaep.pem",
	};
	for (const auto& name : privateFixtures)
	{
		QTest::newRow(name.constData()) << fixture(name.constData()) << true;
	}
	const QList<QByteArray> publicFixtures{
		"crafted-pub-8193.pem",
		"crafted-pub-even-n.pem",
		"crafted-pub-neg-n.pem",
		"crafted-pub-neg-e.pem",
		"crafted-pub-e2p32p1.pem",
		"crafted-pub-e-leading0.pem",
		"crafted-pub-noparams.pem",
		"crafted-pub-bitstring-unused.pem",
		"crafted-pub-extra-field.pem",
		"crafted-pub-oid-sha256rsa.pem",
		"crafted-pub-oid-rsa-x509.pem",
		"crafted-pub-rsapub-ber.pem",
	};
	for (const auto& name : publicFixtures)
	{
		QTest::newRow(name.constData()) << fixture(name.constData()) << false;
	}
}



void VeyonCryptoLegacyRsaTest::rejectsKeys()
{
	QFETCH(QByteArray, data);
	QFETCH(bool, isPrivate);

	clearLastError();
	if (isPrivate)
	{
		QVERIFY(LegacyRsaPrivateKey::fromPem(data).has_value() == false);
	}
	else
	{
		QVERIFY(LegacyRsaPublicKey::fromPem(data).has_value() == false);
	}
	QVERIFY(lastError().isEmpty() == false);
}



void VeyonCryptoLegacyRsaTest::rejectsPublicKeyDer()
{
	const auto publicKeyDer = fixture("key4096-public.der");
	QVERIFY(LegacyRsaPublicKey::fromDer(publicKeyDer).has_value());

	QByteArray berPublicKey = publicKeyDer;
	berPublicKey.replace(4, 2, QByteArray::fromHex("30810d"));
	berPublicKey[3] = char(berPublicKey[3] + 1);

	const QList<QByteArray> invalidValues{
		QByteArray(),
		publicKeyDer.left(publicKeyDer.size() - 1),
		QByteArray(publicKeyDer + '\0'),
		berPublicKey,
		pemContent(fixture("key4096-pkcs1-public.pem")),
		pemContent(fixture("key1024-public.pem")),
		pemContent(fixture("key2048-e65536-public.pem")),
		pemContent(fixture("key2048-rsapss-public.pem")),
		pemContent(fixture("ec-public.pem")),
		fixture("key4096-public.pem"),
		QByteArray(8 * 1024, '0'),
	};
	for (const auto& invalidValue : invalidValues)
	{
		QVERIFY(LegacyRsaPublicKey::fromDer(invalidValue).has_value() == false);
	}
}



void VeyonCryptoLegacyRsaTest::rejectsInconsistentPrivateKey()
{
	// a damaged key file (one modulus byte changed) still parses, but must not be used for signing
	auto der = pemContent(fixture("key4096-pkcs1-private.pem"));
	QVERIFY(der.startsWith(QByteArray::fromHex("30820929020100028202")));
	der[100] = char(der[100] ^ 0x01);

	QVERIFY(LegacyRsaPrivateKey::fromPem(pem("RSA PRIVATE KEY", der)).has_value() == false);
	QVERIFY(lastError().contains(QLatin1String("inconsistent")));

	// bogus and multi-prime keys are rejected by the component check, before any private key
	// operation (bounded import cost for untrusted keys, e.g. received through the Web API)
	for (const auto name : {"crafted-bogus8192.pem", "crafted-mp3-pkcs1.pem", "crafted-good-crt-bad-d.pem"})
	{
		QVERIFY(LegacyRsaPrivateKey::fromPem(fixture(name)).has_value() == false);
		QVERIFY2(lastError().contains(QLatin1String("inconsistent or multi-prime key")), qPrintable(lastError()));
	}
	// larger keys are rejected by the size policy already
	QVERIFY(LegacyRsaPrivateKey::fromPem(fixture("crafted-bogus8194.pem")).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("unsupported key size")), qPrintable(lastError()));

	// consistent components, but p is the product of two primes (primality is not tested): only
	// the pairwise sign/verify check catches it
	QVERIFY(LegacyRsaPrivateKey::fromPem(fixture("crafted-composite-p.pem")).has_value() == false);
	QVERIFY2(lastError().endsWith(QLatin1String(": inconsistent key")), qPrintable(lastError()));

	// the same key with d + k * lcm(p - 1, q - 1) > n: consistent modulo p - 1 and q - 1, but a
	// private exponent of up to the maximum encoded size would make the pairwise check expensive
	QVERIFY(LegacyRsaPrivateKey::fromPem(fixture("crafted-composite-p-big-d.pem")).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("inconsistent or multi-prime key")), qPrintable(lastError()));
}



void VeyonCryptoLegacyRsaTest::acceptsBoundaryKeys_data()
{
	QTest::addColumn<QByteArray>("file");
	QTest::addColumn<bool>("isPrivate");
	QTest::addColumn<int>("bits");

	QTest::newRow("private 2048 (crafted base)") << QByteArray("crafted-base.pem") << true << 2048;
	QTest::newRow("private 2049") << QByteArray("crafted-k2049.pem") << true << 2049;
	QTest::newRow("private 8192") << QByteArray("crafted-k8192.pem") << true << 8192;
	QTest::newRow("public 8192") << QByteArray("crafted-pub-8192.pem") << false << 8192;
	QTest::newRow("public exponent 3") << QByteArray("crafted-pub-e3.pem") << false << 2048;
	QTest::newRow("public exponent 2^32-1") << QByteArray("crafted-pub-emax.pem") << false << 2048;
}



void VeyonCryptoLegacyRsaTest::acceptsBoundaryKeys()
{
	QFETCH(QByteArray, file);
	QFETCH(bool, isPrivate);
	QFETCH(int, bits);

	if (isPrivate)
	{
		const auto key = LegacyRsaPrivateKey::fromPem(fixture(file.constData()));
		QVERIFY2(key.has_value(), qPrintable(lastError()));
		QCOMPARE(key->bits(), bits);
		const auto signature = key->signPkcs1Sha512(QByteArray("message"));
		QVERIFY(signature.has_value());
		QVERIFY(key->publicKey().verifyPkcs1Sha512(QByteArray("message"), *signature));
	}
	else
	{
		const auto key = LegacyRsaPublicKey::fromPem(fixture(file.constData()));
		QVERIFY2(key.has_value(), qPrintable(lastError()));
		QCOMPARE(key->bits(), bits);
		QVERIFY(key->encryptOaepSha1(QByteArray("password")).has_value());
	}
}



void VeyonCryptoLegacyRsaTest::oddModulusSize()
{
	// 2049 bits: the modulus size (257 bytes) is not bits / 8
	const auto key = privateKey("crafted-k2049.pem");
	const auto publicKey = key.publicKey();
	QCOMPARE(publicKey.maximumOaepSha1PlaintextSize(), qsizetype(257 - 42));

	const auto signature = key.signPkcs1Sha512(QByteArray("message"));
	QVERIFY(signature.has_value());
	QCOMPARE(signature->size(), 257);
	QVERIFY(publicKey.verifyPkcs1Sha512(QByteArray("message"), *signature));
	QVERIFY(publicKey.verifyPkcs1Sha512(QByteArray("message"), signature->left(256)) == false);

	const QByteArray plaintext(257 - 42, 'p');
	const auto ciphertext = publicKey.encryptOaepSha1(plaintext);
	QVERIFY(ciphertext.has_value());
	QCOMPARE(ciphertext->size(), 257);
	QCOMPARE(toByteArray(key.decryptOaepSha1(*ciphertext)), plaintext);
	QVERIFY(publicKey.encryptOaepSha1(QByteArray(257 - 41, 'p')).has_value() == false);

	const auto hex = LegacyRsa::encryptPasswordHex(publicKey, QByteArray("password"));
	QVERIFY(hex.has_value());
	QCOMPARE(hex->size(), 514);
	QCOMPARE(toByteArray(LegacyRsa::decryptPasswordHex(key, *hex)), QByteArray("password"));
}



void VeyonCryptoLegacyRsaTest::rejectsCraftedSignatures()
{
	// raw RSA over crafted EMSA-PKCS1-v1_5 encodings (lenient verifiers accept some of them, e.g.
	// Bleichenbacher 2006 forgeries with garbage after the DigestInfo)
	const auto key = publicKey("key2048-public.pem");
	const QByteArray message("challenge");
	const auto hash = QCryptographicHash::hash(message, QCryptographicHash::Sha512);
	const QByteArray digestInfo = QByteArray::fromHex("3051300d060960864801650304020305000440") + hash;
	const QByteArray digestInfoWithoutNull = QByteArray::fromHex("304f300b06096086480165030402030440") + hash;
	const QByteArray berDigestInfo = QByteArray::fromHex("308151300d060960864801650304020305000440") + hash;
	const QByteArray sha256DigestInfo = QByteArray::fromHex("3031300d060960864801650304020105000420") +
		QCryptographicHash::hash(message, QCryptographicHash::Sha256);

	// the canonical encoding verifies (the crafting itself is correct)
	QVERIFY(key.verifyPkcs1Sha512(message, opensslRawPrivate("key2048-private.pem", pkcs1EncodedMessage(256, digestInfo))));

	QByteArray blockType2 = pkcs1EncodedMessage(256, digestInfo);
	blockType2[1] = 2;
	QByteArray zeroInPadding = pkcs1EncodedMessage(256, digestInfo);
	zeroInPadding[10] = 0;

	const QList<QByteArray> encodedMessages{
		pkcs1EncodedMessage(256, digestInfoWithoutNull),
		pkcs1EncodedMessage(256, berDigestInfo),
		pkcs1EncodedMessage(256, digestInfo, QByteArray(32, 'G')),
		pkcs1EncodedMessage(256, hash),
		pkcs1EncodedMessage(256, sha256DigestInfo),
		blockType2,
		zeroInPadding,
	};
	for (const auto& encodedMessage : encodedMessages)
	{
		const auto signature = opensslRawPrivate("key2048-private.pem", encodedMessage);
		QCOMPARE(signature.size(), 256);
		QVERIFY(key.verifyPkcs1Sha512(message, signature) == false);
	}

	QByteArray one(256, '\0');
	one[255] = 1;
	QVERIFY(key.verifyPkcs1Sha512(message, one) == false);
	QVERIFY(key.verifyPkcs1Sha512(message, QByteArray(256, '\xff')) == false);
}



void VeyonCryptoLegacyRsaTest::rejectsOaepLabel()
{
	const auto key = privateKey("key4096-private.pem");
	const auto ciphertext = opensslEncryptWithLabel("key4096-private.pem", QByteArray("label"), QByteArray("password"));
	QCOMPARE(ciphertext.size(), 512);
	QVERIFY(key.decryptOaepSha1(ciphertext).has_value() == false);
	QCOMPARE(toByteArray(key.decryptOaepSha1(opensslEncrypt("key4096-private.pem", RSA_PKCS1_OAEP_PADDING, nullptr,
															QByteArray("password")))),
			 QByteArray("password"));
}



void VeyonCryptoLegacyRsaTest::successClearsLastError()
{
	const auto key = privateKey("key2048-private.pem");

	QVERIFY(LegacyRsaPrivateKey::fromPem(QByteArray("invalid")).has_value() == false);
	QVERIFY(lastError().isEmpty() == false);
	const auto signature = key.signPkcs1Sha512(QByteArray("m"));
	QVERIFY(signature.has_value());
	QVERIFY(lastError().isEmpty());

	QVERIFY(key.publicKey().verifyPkcs1Sha512(QByteArray("m"), QByteArray()) == false);
	QVERIFY(lastError().isEmpty() == false);
	QVERIFY(key.publicKey().verifyPkcs1Sha512(QByteArray("m"), *signature));
	QVERIFY(lastError().isEmpty());

	QVERIFY(key.decryptOaepSha1(QByteArray()).has_value() == false);
	QVERIFY(LegacyRsaPrivateKey::fromPem(fixture("key2048-private.pem")).has_value());
	QVERIFY(lastError().isEmpty());
}



void VeyonCryptoLegacyRsaTest::nullKeys()
{
	const LegacyRsaPrivateKey privateKey;
	const LegacyRsaPublicKey publicKey;

	QVERIFY(privateKey.isNull());
	QVERIFY(publicKey.isNull());
	QCOMPARE(privateKey.bits(), 0);
	QCOMPARE(publicKey.bits(), 0);
	QVERIFY(privateKey.publicKey().isNull());
	QVERIFY(privateKey.toPem().has_value() == false);
	QVERIFY(privateKey.signPkcs1Sha512(QByteArray("m")).has_value() == false);
	QVERIFY(privateKey.decryptOaepSha1(QByteArray(512, 'c')).has_value() == false);
	QVERIFY(publicKey.toDer().isEmpty());
	QVERIFY(publicKey.toPem().isEmpty());
	QVERIFY(publicKey.verifyPkcs1Sha512(QByteArray("m"), QByteArray(512, 's')) == false);
	QVERIFY(publicKey.encryptOaepSha1(QByteArray("m")).has_value() == false);
	QCOMPARE(publicKey.maximumOaepSha1PlaintextSize(), qsizetype(0));
	QVERIFY((publicKey == publicKey) == false);
}



void VeyonCryptoLegacyRsaTest::copyAndMove()
{
	// public keys are implicitly shared, private keys move-only
	LegacyRsaPublicKey publicKey;
	const auto loadedPublicKey = ::publicKey("key2048-public.pem");
	publicKey = loadedPublicKey;
	QVERIFY(publicKey == loadedPublicKey);
	LegacyRsaPublicKey movedPublicKey;
	movedPublicKey = std::move(publicKey);
	QVERIFY(movedPublicKey == loadedPublicKey);
	const LegacyRsaPublicKey copiedPublicKey(movedPublicKey);
	QCOMPARE(copiedPublicKey.toDer(), loadedPublicKey.toDer());

	LegacyRsaPrivateKey key;
	key = ::privateKey("key2048-private.pem");
	QCOMPARE(key.bits(), 2048);
	LegacyRsaPrivateKey movedKey(std::move(key));
	QCOMPARE(movedKey.bits(), 2048);
	QVERIFY(movedKey.publicKey() == loadedPublicKey);
}



QTEST_GUILESS_MAIN(VeyonCryptoLegacyRsaTest)
#include "VeyonCryptoLegacyRsaTest.moc"
