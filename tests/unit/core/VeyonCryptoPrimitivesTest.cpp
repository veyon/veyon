// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "Aead.h"
#include "ConstantTime.h"
#include "CryptoError.h"
#include "Hash.h"
#include "Hkdf.h"
#include "Hmac.h"
#include "OpenSslCompat.h"
#include "PasswordKdf.h"
#include "PrivateKey.h"
#include "Random.h"
#include "SecureBuffer.h"

#include <vector>

using namespace Veyon::Crypto;

static QByteArray hex(const char* data)
{
	return QByteArray::fromHex(QByteArray(data));
}



static QByteArray toByteArray(const std::optional<SecureBuffer>& buffer)
{
	return buffer.has_value() ? buffer->toUnprotectedByteArray() : QByteArray{};
}



static QStringList capturedMessages;

static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	Q_UNUSED(type)
	Q_UNUSED(context)
	capturedMessages.append(message);
}



class VeyonCryptoPrimitivesTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();

	void secureBufferBasics();
	void secureBufferHex();
	void secureHeapRejectsInvalidSizes();
	void secureBufferWipe();

	void random();
	void constantTimeEquals();

	void hash_data();
	void hash();

	void hmacKnownAnswers_data();
	void hmacKnownAnswers();
	void hmacVerify();

	void hkdfKnownAnswers_data();
	void hkdfKnownAnswers();
	void hkdfInvalidInput();

	void aeadKnownAnswers_data();
	void aeadKnownAnswers();
	void aeadRoundTrip_data();
	void aeadRoundTrip();
	void aeadRejectsTampering_data();
	void aeadRejectsTampering();
	void aeadInvalidInput();

	void scryptKnownAnswers_data();
	void scryptKnownAnswers();
	void scryptRejectsInvalidParameters();
	void argon2idKnownAnswer();
	void argon2idParameters();

	void errorsContainNoSecrets();

	// initialises the secure heap, therefore last
	void operationsWithFullSecureHeap();
};



void VeyonCryptoPrimitivesTest::initTestCase()
{
	qInfo() << "OpenSSL runtime:" << openSslVersion()
			<< "Argon2id available:" << PasswordKdf::isArgon2idAvailable();
}



void VeyonCryptoPrimitivesTest::secureBufferBasics()
{
	SecureBuffer empty;
	QVERIFY(empty.isEmpty());
	QCOMPARE(empty.constData(), nullptr);

	SecureBuffer zeroed(16);
	QCOMPARE(zeroed.size(), 16);
	QCOMPARE(zeroed.toUnprotectedByteArray(), QByteArray(16, 0));

	SecureBuffer negative(-5);
	QVERIFY(negative.isEmpty());

	const QByteArray data("secret data");
	auto buffer = SecureBuffer::fromByteArray(data);
	QCOMPARE(buffer.toUnprotectedByteArray(), data);

	// clone is an independent copy
	auto copy = buffer.clone();
	QVERIFY(copy.constData() != buffer.constData());
	copy.data()[0] = 'S';
	QCOMPARE(buffer.toUnprotectedByteArray(), data);

	// move leaves the source empty
	SecureBuffer moved(std::move(buffer));
	QVERIFY(buffer.isEmpty()); // NOLINT(bugprone-use-after-move)
	QCOMPARE(moved.toUnprotectedByteArray(), data);
	moved = std::move(copy);
	QCOMPARE(moved.toUnprotectedByteArray(), QByteArray("Secret data"));

	// resize keeps the prefix and zero-fills
	QVERIFY(moved.resize(4));
	QCOMPARE(moved.toUnprotectedByteArray(), QByteArray("Secr"));
	QVERIFY(moved.resize(6));
	QCOMPARE(moved.toUnprotectedByteArray(), QByteArray("Secr\0\0", 6));

	moved.clear();
	QVERIFY(moved.isEmpty());

	// sizes are never clamped: out of range means empty, resize() fails and keeps the contents
	QVERIFY(SecureBuffer(SecureBuffer::MaximumSize + 1).isEmpty());
	QCOMPARE(SecureBuffer(SecureBuffer::MaximumSize + 1).data(), nullptr);
	QCOMPARE(SecureBuffer(SecureBuffer::MaximumSize).size(), qsizetype(SecureBuffer::MaximumSize));
	QVERIFY(SecureBuffer(nullptr, 5).isEmpty());
	auto unchanged = SecureBuffer::fromByteArray(QByteArray("keep"));
	QVERIFY(unchanged.resize(SecureBuffer::MaximumSize + 1) == false);
	QVERIFY(unchanged.resize(-1) == false);
	QCOMPARE(unchanged.toUnprotectedByteArray(), QByteArray("keep"));
}



void VeyonCryptoPrimitivesTest::secureHeapRejectsInvalidSizes()
{
	// invalid sizes must not reach CRYPTO_secure_malloc_init(), which aborts the process for sizes
	// that are not a power of two; none of these calls initialises the heap
	for (const auto size : {qsizetype(0), qsizetype(-1), qsizetype(1000), qsizetype(48 * 1024),
							SecureBuffer::MinimumSecureHeapSize / 2, SecureBuffer::MaximumSecureHeapSize * 2})
	{
		QCOMPARE(SecureBuffer::initializeSecureHeap(size), SecureBuffer::SecureHeapState::Unavailable);
	}
	QVERIFY(SecureBuffer::isSecureHeapInitialized() == false);
}



void VeyonCryptoPrimitivesTest::secureBufferHex()
{
	const auto bytes = SecureBuffer::fromByteArray(hex("00ff7f80a5"));
	QCOMPARE(bytes.toHex()->toUnprotectedByteArray(), QByteArray("00ff7f80a5"));
	QVERIFY(SecureBuffer().toHex()->isEmpty());
	QVERIFY(SecureBuffer(SecureBuffer::MaximumSize / 2 + 1).toHex().has_value() == false);

	const QByteArray mixedCase("00FF7f80A5");
	const auto decoded = SecureBuffer::fromHex(mixedCase.constData(), mixedCase.size());
	QVERIFY(decoded.has_value());
	QCOMPARE(decoded->toUnprotectedByteArray(), hex("00ff7f80a5"));

	for (const auto& invalid : {QByteArray("0"), QByteArray("0g"), QByteArray("00 1"), QByteArray("0x00"), QByteArray("00\n")})
	{
		QVERIFY2(SecureBuffer::fromHex(invalid.constData(), invalid.size()).has_value() == false, invalid.constData());
	}
	QVERIFY(SecureBuffer::fromHex(nullptr, 2).has_value() == false);
	QVERIFY(SecureBuffer::fromHex("", 0).has_value());
}



void VeyonCryptoPrimitivesTest::secureBufferWipe()
{
	auto buffer = SecureBuffer::fromByteArray(QByteArray(64, char(0xa5)));
	const auto data = buffer.constData();
	buffer.wipe();
	QCOMPARE(buffer.constData(), data);
	QCOMPARE(buffer.toUnprotectedByteArray(), QByteArray(64, 0));
}



void VeyonCryptoPrimitivesTest::random()
{
	const auto first = Random::bytes(32);
	const auto second = Random::bytes(32);
	QVERIFY(first.has_value() && second.has_value());
	QCOMPARE(first->size(), 32);
	QVERIFY(Veyon::Crypto::constantTimeEquals(*first, *second) == false);

	QVERIFY(Random::bytes(0).has_value() == false);
	QVERIFY(Random::bytes(-1).has_value() == false);
	QVERIFY(Random::bytes(Random::MaximumSize + 1).has_value() == false);

	const auto hexString = Random::hex(4);
	QVERIFY(hexString.has_value());
	QCOMPARE(hexString->size(), 8);
	QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-f]{8}$")).match(*hexString).hasMatch());
}



void VeyonCryptoPrimitivesTest::constantTimeEquals()
{
	QVERIFY(Veyon::Crypto::constantTimeEquals(QByteArray("abc"), QByteArray("abc")));
	QVERIFY(Veyon::Crypto::constantTimeEquals(QByteArray("abc"), QByteArray("abd")) == false);
	QVERIFY(Veyon::Crypto::constantTimeEquals(QByteArray("abc"), QByteArray("abcd")) == false);
	QVERIFY(Veyon::Crypto::constantTimeEquals(QByteArray(), QByteArray()) == false);
	QVERIFY(Veyon::Crypto::constantTimeEquals(SecureBuffer::fromByteArray("abc"), QByteArray("abc")));
}



void VeyonCryptoPrimitivesTest::hash_data()
{
	QTest::addColumn<int>("algorithm");
	QTest::addColumn<QByteArray>("expected");

	// FIPS 180-2 "abc" examples
	QTest::newRow("sha256") << int(Hash::Algorithm::Sha256)
							<< hex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	QTest::newRow("sha384") << int(Hash::Algorithm::Sha384)
							<< hex("cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed"
								   "8086072ba1e7cc2358baeca134c825a7");
	QTest::newRow("sha512") << int(Hash::Algorithm::Sha512)
							<< hex("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
								   "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
}



void VeyonCryptoPrimitivesTest::hash()
{
	QFETCH(int, algorithm);
	QFETCH(QByteArray, expected);

	const auto digest = Hash::digest(Hash::Algorithm(algorithm), QByteArray("abc"));
	QVERIFY(digest.has_value());
	QCOMPARE(digest->toHex(), expected.toHex());
	QCOMPARE(digest->size(), Hash::size(Hash::Algorithm(algorithm)));
}



void VeyonCryptoPrimitivesTest::hmacKnownAnswers_data()
{
	QTest::addColumn<QByteArray>("key");
	QTest::addColumn<QByteArray>("message");
	QTest::addColumn<QByteArray>("expected");

	// RFC 4231 test cases 1, 2, 6, 7 (HMAC-SHA-256)
	QTest::newRow("rfc4231-1") << QByteArray(20, 0x0b) << QByteArray("Hi There")
							   << hex("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
	QTest::newRow("rfc4231-2") << QByteArray("Jefe") << QByteArray("what do ya want for nothing?")
							   << hex("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
	QTest::newRow("rfc4231-6") << QByteArray(131, char(0xaa))
							   << QByteArray("Test Using Larger Than Block-Size Key - Hash Key First")
							   << hex("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
	QTest::newRow("rfc4231-7") << QByteArray(131, char(0xaa))
							   << QByteArray("This is a test using a larger than block-size key and a larger than "
											 "block-size data. The key needs to be hashed before being used by the "
											 "HMAC algorithm.")
							   << hex("9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");
}



void VeyonCryptoPrimitivesTest::hmacKnownAnswers()
{
	QFETCH(QByteArray, key);
	QFETCH(QByteArray, message);
	QFETCH(QByteArray, expected);

	const auto mac = Hmac::sha256(SecureBuffer::fromByteArray(key), message);
	QVERIFY(mac.has_value());
	QCOMPARE(mac->toHex(), expected.toHex());
	QVERIFY(Hmac::verifySha256(key, message, expected));
}



void VeyonCryptoPrimitivesTest::hmacVerify()
{
	const QByteArray key("key");
	const QByteArray message("message");
	const auto mac = Hmac::sha256(key, message);
	QVERIFY(mac.has_value());

	auto tampered = *mac;
	tampered[5] = char(tampered[5] ^ 0x01);
	QVERIFY(Hmac::verifySha256(key, message, tampered) == false);
	QVERIFY(Hmac::verifySha256(key, message, mac->left(16)) == false);
	QVERIFY(Hmac::verifySha256(key, QByteArray("messagE"), *mac) == false);
	QVERIFY(Hmac::verifySha256(QByteArray("kez"), message, *mac) == false);
	QVERIFY(Hmac::verifySha256(key, message, {}) == false);

	// empty keys are rejected (an empty key means no authentication)
	QVERIFY(Hmac::sha256({}, message).has_value() == false);
}



void VeyonCryptoPrimitivesTest::hkdfKnownAnswers_data()
{
	QTest::addColumn<QByteArray>("inputKeyMaterial");
	QTest::addColumn<QByteArray>("salt");
	QTest::addColumn<QByteArray>("info");
	QTest::addColumn<int>("outputSize");
	QTest::addColumn<QByteArray>("pseudoRandomKey");
	QTest::addColumn<QByteArray>("outputKeyMaterial");

	QByteArray ikm2, salt2, info2;
	for (int i = 0; i < 80; ++i)
	{
		ikm2.append(char(i));
		salt2.append(char(0x60 + i));
		info2.append(char(0xb0 + i));
	}

	// RFC 5869 appendix A.1-A.3 (SHA-256)
	QTest::newRow("rfc5869-1") << QByteArray(22, 0x0b) << hex("000102030405060708090a0b0c") << hex("f0f1f2f3f4f5f6f7f8f9") << 42
							   << hex("077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5")
							   << hex("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865");
	QTest::newRow("rfc5869-2") << ikm2 << salt2 << info2 << 82
							   << hex("06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244")
							   << hex("b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c"
									  "59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71"
									  "cc30c58179ec3e87c14c01d5c1f3434f1d87");
	QTest::newRow("rfc5869-3") << QByteArray(22, 0x0b) << QByteArray() << QByteArray() << 42
							   << hex("19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04")
							   << hex("8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8");
}



void VeyonCryptoPrimitivesTest::hkdfKnownAnswers()
{
	QFETCH(QByteArray, inputKeyMaterial);
	QFETCH(QByteArray, salt);
	QFETCH(QByteArray, info);
	QFETCH(int, outputSize);
	QFETCH(QByteArray, pseudoRandomKey);
	QFETCH(QByteArray, outputKeyMaterial);

	const auto ikm = SecureBuffer::fromByteArray(inputKeyMaterial);

	QCOMPARE(toByteArray(Hkdf::sha256(ikm, salt, info, outputSize)).toHex(), outputKeyMaterial.toHex());

	const auto prk = Hkdf::extractSha256(ikm, salt);
	QCOMPARE(toByteArray(prk).toHex(), pseudoRandomKey.toHex());
	QVERIFY(prk.has_value());
	QCOMPARE(toByteArray(Hkdf::expandSha256(*prk, info, outputSize)).toHex(), outputKeyMaterial.toHex());
}



void VeyonCryptoPrimitivesTest::hkdfInvalidInput()
{
	const QByteArray ikm(32, 0x01);
	QVERIFY(Hkdf::sha256({}, {}, {}, 32).has_value() == false);
	QVERIFY(Hkdf::sha256(ikm, {}, {}, 0).has_value() == false);
	QVERIFY(Hkdf::sha256(ikm, {}, {}, Hkdf::MaximumOutputSize + 1).has_value() == false);
	QVERIFY(Hkdf::sha256(ikm, {}, {}, Hkdf::MaximumOutputSize).has_value());
	QVERIFY(Hkdf::expandSha256(QByteArray(31, 0x01), {}, 32).has_value() == false);

	// input limits are the same on every OpenSSL version
	QVERIFY(Hkdf::sha256(ikm, {}, QByteArray(Hkdf::MaximumInfoSize, 'i'), 32).has_value());
	QVERIFY(Hkdf::sha256(ikm, {}, QByteArray(Hkdf::MaximumInfoSize + 1, 'i'), 32).has_value() == false);
	QVERIFY(Hkdf::sha256(ikm, QByteArray(Hkdf::MaximumSaltSize + 1, 's'), {}, 32).has_value() == false);
	QVERIFY(Hkdf::sha256(QByteArray(Hkdf::MaximumInputKeyMaterialSize + 1, 'k'), {}, {}, 32).has_value() == false);
	QVERIFY(Hkdf::expandSha256(QByteArray(32, 0x01), QByteArray(Hkdf::MaximumInfoSize + 1, 'i'), 32).has_value() == false);

	// different info strings give independent keys (02-ipc.md §4 c2s/s2c)
	const auto c2s = Hkdf::sha256(ikm, QByteArray("salt"), QByteArray("veyon-ipc-v1 keyc2s"), 32);
	const auto s2c = Hkdf::sha256(ikm, QByteArray("salt"), QByteArray("veyon-ipc-v1 keys2c"), 32);
	QVERIFY(c2s.has_value() && s2c.has_value());
	QVERIFY(Veyon::Crypto::constantTimeEquals(*c2s, *s2c) == false);
}



void VeyonCryptoPrimitivesTest::aeadKnownAnswers_data()
{
	QTest::addColumn<int>("algorithm");
	QTest::addColumn<QByteArray>("key");
	QTest::addColumn<QByteArray>("nonce");
	QTest::addColumn<QByteArray>("plaintext");
	QTest::addColumn<QByteArray>("associatedData");
	QTest::addColumn<QByteArray>("ciphertextWithTag");

	QByteArray chachaKey;
	for (int i = 0x80; i < 0xa0; ++i)
	{
		chachaKey.append(char(i));
	}

	// RFC 8439 section 2.8.2
	QTest::newRow("rfc8439-2.8.2")
		<< int(Aead::Algorithm::ChaCha20Poly1305) << chachaKey << hex("070000004041424344454647")
		<< QByteArray("Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, "
					  "sunscreen would be it.")
		<< hex("50515253c0c1c2c3c4c5c6c7")
		<< hex("d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8ca9671282fafb69da92728b"
			   "1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc"
			   "3ff4def08e4b7a9de576d26586cec64b6116"
			   "1ae10b594f09e26a7e902ecbd0600691");

	// GCM specification test cases 13, 14, 16 (AES-256)
	QTest::newRow("gcm-13") << int(Aead::Algorithm::Aes256Gcm) << QByteArray(32, 0) << QByteArray(12, 0)
							<< QByteArray() << QByteArray() << hex("530f8afbc74536b9a963b4f1c4cb738b");
	QTest::newRow("gcm-14") << int(Aead::Algorithm::Aes256Gcm) << QByteArray(32, 0) << QByteArray(12, 0)
							<< QByteArray(16, 0) << QByteArray()
							<< hex("cea7403d4d606b6e074ec5d3baf39d18d0d1c8a799996bf0265b98b5d48ab919");
	QTest::newRow("gcm-16") << int(Aead::Algorithm::Aes256Gcm)
							<< hex("feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308")
							<< hex("cafebabefacedbaddecaf888")
							<< hex("d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a72"
								   "1c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39")
							<< hex("feedfacedeadbeeffeedfacedeadbeefabaddad2")
							<< hex("522dc1f099567d07f47f37a32a84427d643a8cdcbfe5c0c97598a2bd2555d1aa"
								   "8cb08e48590dbb3da7b08b1056828838c5f61e6393ba7a0abcc9f662"
								   "76fc6ece0f4e1768cddf8853bb2d551b");
}



void VeyonCryptoPrimitivesTest::aeadKnownAnswers()
{
	QFETCH(int, algorithm);
	QFETCH(QByteArray, key);
	QFETCH(QByteArray, nonce);
	QFETCH(QByteArray, plaintext);
	QFETCH(QByteArray, associatedData);
	QFETCH(QByteArray, ciphertextWithTag);

	const auto keyBuffer = SecureBuffer::fromByteArray(key);
	const auto sealed = Aead::seal(Aead::Algorithm(algorithm), keyBuffer, nonce, plaintext, associatedData);
	QVERIFY(sealed.has_value());
	QCOMPARE(sealed->toHex(), ciphertextWithTag.toHex());

	const auto opened = Aead::open(Aead::Algorithm(algorithm), keyBuffer, nonce, ciphertextWithTag, associatedData);
	QVERIFY(opened.has_value());
	QCOMPARE(opened->toUnprotectedByteArray(), plaintext);
}



void VeyonCryptoPrimitivesTest::aeadRoundTrip_data()
{
	QTest::addColumn<int>("algorithm");
	QTest::newRow("chacha20-poly1305") << int(Aead::Algorithm::ChaCha20Poly1305);
	QTest::newRow("aes-256-gcm") << int(Aead::Algorithm::Aes256Gcm);
}



void VeyonCryptoPrimitivesTest::aeadRoundTrip()
{
	QFETCH(int, algorithm);
	const auto aead = Aead::Algorithm(algorithm);

	const auto key = Random::bytes(Aead::KeySize);
	QVERIFY(key.has_value());

	for (const auto size : {0, 1, 15, 16, 17, 4096})
	{
		const QByteArray plaintext(size, 'x');
		const QByteArray nonce = QByteArray(4, 0) + QByteArray::fromHex("0000000000000007");
		const auto sealed = Aead::seal(aead, *key, nonce, plaintext, QByteArray("aad"));
		QVERIFY(sealed.has_value());
		QCOMPARE(sealed->size(), size + Aead::TagSize);
		const auto opened = Aead::open(aead, *key, nonce, *sealed, QByteArray("aad"));
		QVERIFY(opened.has_value());
		QCOMPARE(opened->toUnprotectedByteArray(), plaintext);
	}
}



void VeyonCryptoPrimitivesTest::aeadRejectsTampering_data()
{
	aeadRoundTrip_data();
}



void VeyonCryptoPrimitivesTest::aeadRejectsTampering()
{
	QFETCH(int, algorithm);
	const auto aead = Aead::Algorithm(algorithm);

	const QByteArray key(32, 0x11);
	const QByteArray nonce(12, 0x22);
	const QByteArray plaintext("attack at dawn");
	const QByteArray associatedData("u32be length");
	const auto sealed = Aead::seal(aead, key, nonce, plaintext, associatedData);
	QVERIFY(sealed.has_value());

	auto tamperedTag = *sealed;
	tamperedTag[tamperedTag.size() - 1] = char(tamperedTag.back() ^ 0x80);
	auto tamperedCiphertext = *sealed;
	tamperedCiphertext[0] = char(tamperedCiphertext[0] ^ 0x01);

	QVERIFY(Aead::open(aead, key, nonce, tamperedTag, associatedData).has_value() == false);
	QCOMPARE(Veyon::Crypto::lastError(), QStringLiteral("Aead::open: authentication failed"));
	QVERIFY(Aead::open(aead, key, nonce, tamperedCiphertext, associatedData).has_value() == false);
	QVERIFY(Aead::open(aead, key, nonce, *sealed, QByteArray("other aad")).has_value() == false);
	QVERIFY(Aead::open(aead, key, nonce, *sealed, {}).has_value() == false);
	QVERIFY(Aead::open(aead, QByteArray(32, 0x12), nonce, *sealed, associatedData).has_value() == false);
	QVERIFY(Aead::open(aead, key, QByteArray(12, 0x23), *sealed, associatedData).has_value() == false);
	QVERIFY(Aead::open(aead, key, nonce, sealed->left(sealed->size() - 1), associatedData).has_value() == false);
	QVERIFY(Aead::open(aead, key, nonce, sealed->left(Aead::TagSize - 1), associatedData).has_value() == false);
	QVERIFY(Aead::open(aead, key, nonce, {}, associatedData).has_value() == false);
	QVERIFY(Aead::open(aead, key, nonce, QByteArray(*sealed + QByteArray(1, 0)), associatedData).has_value() == false);

	// ciphertext of the other algorithm
	const auto other = aead == Aead::Algorithm::Aes256Gcm ? Aead::Algorithm::ChaCha20Poly1305 : Aead::Algorithm::Aes256Gcm;
	QVERIFY(Aead::open(other, key, nonce, *sealed, associatedData).has_value() == false);
}



void VeyonCryptoPrimitivesTest::aeadInvalidInput()
{
	const auto algorithm = Aead::Algorithm::ChaCha20Poly1305;
	const QByteArray key(32, 0x11);
	const QByteArray nonce(12, 0x22);

	QVERIFY(Aead::seal(algorithm, key.left(31), nonce, QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::seal(algorithm, QByteArray(key + "x"), nonce, QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::seal(algorithm, {}, nonce, QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::seal(algorithm, key, nonce.left(8), QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::seal(algorithm, key, QByteArray(nonce + "x"), QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::seal(algorithm, key, {}, QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::seal(Aead::Algorithm(42), key, nonce, QByteArray("x"), {}).has_value() == false);
	QVERIFY(Aead::open(algorithm, key.left(16), nonce, QByteArray(32, 0), {}).has_value() == false);
}



void VeyonCryptoPrimitivesTest::scryptKnownAnswers_data()
{
	QTest::addColumn<QByteArray>("password");
	QTest::addColumn<QByteArray>("salt");
	QTest::addColumn<quint64>("cost");
	QTest::addColumn<quint32>("blockSize");
	QTest::addColumn<quint32>("parallelism");
	QTest::addColumn<QByteArray>("expected");

	// RFC 7914 section 12, vectors 2 and 3 (vector 1 has an empty password and salt, which are
	// rejected by design; vector 4 needs 1 GiB of memory)
	QTest::newRow("rfc7914-2") << QByteArray("password") << QByteArray("NaCl") << quint64(1024) << quint32(8) << quint32(16)
							   << hex("fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b373162"
									  "2eaf30d92e22a3886ff109279d9830dac727afb94a83ee6d8360cbdfa2cc0640");
	QTest::newRow("rfc7914-3") << QByteArray("pleaseletmein") << QByteArray("SodiumChloride") << quint64(16384) << quint32(8) << quint32(1)
							   << hex("7023bdcb3afd7348461c06cd81fd38ebfda8fbba904f8e3ea9b543f6545da1f2"
									  "d5432955613f0fcf62d49705242a9af9e61e85dc0d651e40dfcf017b45575887");
}



void VeyonCryptoPrimitivesTest::scryptKnownAnswers()
{
	QFETCH(QByteArray, password);
	QFETCH(QByteArray, salt);
	QFETCH(quint64, cost);
	QFETCH(quint32, blockSize);
	QFETCH(quint32, parallelism);
	QFETCH(QByteArray, expected);

	PasswordKdf::ScryptParameters parameters;
	parameters.cost = cost;
	parameters.blockSize = blockSize;
	parameters.parallelism = parallelism;

	const auto key = PasswordKdf::scrypt(SecureBuffer::fromByteArray(password), salt, parameters, expected.size());
	QCOMPARE(toByteArray(key).toHex(), expected.toHex());
}



void VeyonCryptoPrimitivesTest::scryptRejectsInvalidParameters()
{
	const QByteArray password("correct horse battery staple");
	const QByteArray salt(PasswordKdf::SaltSize, 0x01);

	// spec default (03 §2 item 6): N=2^15, r=8, p=1
	const PasswordKdf::ScryptParameters defaults;
	QCOMPARE(defaults.cost, quint64(1) << 15);
	QCOMPARE(defaults.blockSize, quint32(8));
	QCOMPARE(defaults.parallelism, quint32(1));
	const auto first = PasswordKdf::scrypt(password, salt, defaults, 32);
	const auto second = PasswordKdf::scrypt(password, salt, defaults, 32);
	QVERIFY(first.has_value() && second.has_value());
	QVERIFY(Veyon::Crypto::constantTimeEquals(*first, *second));
	QVERIFY(Veyon::Crypto::constantTimeEquals(*first, toByteArray(PasswordKdf::scrypt(password, QByteArray(16, 0x02), defaults, 32))) == false);

	auto parameters = defaults;
	parameters.cost = 1000;
	QVERIFY(PasswordKdf::scrypt(password, salt, parameters, 32).has_value() == false);
	parameters.cost = PasswordKdf::MaximumScryptCost * 2;
	QVERIFY(PasswordKdf::scrypt(password, salt, parameters, 32).has_value() == false);
	parameters = defaults;
	parameters.blockSize = 0;
	QVERIFY(PasswordKdf::scrypt(password, salt, parameters, 32).has_value() == false);
	parameters.blockSize = PasswordKdf::MaximumScryptBlockSize + 1;
	QVERIFY(PasswordKdf::scrypt(password, salt, parameters, 32).has_value() == false);
	parameters = defaults;
	parameters.parallelism = 0;
	QVERIFY(PasswordKdf::scrypt(password, salt, parameters, 32).has_value() == false);
	// memory limit (128 * r * N > 1 GiB) without allocating it
	parameters = defaults;
	parameters.cost = PasswordKdf::MaximumScryptCost;
	parameters.blockSize = PasswordKdf::MaximumScryptBlockSize;
	QVERIFY(PasswordKdf::scrypt(password, salt, parameters, 32).has_value() == false);

	QVERIFY(PasswordKdf::scrypt({}, salt, defaults, 32).has_value() == false);
	QVERIFY(PasswordKdf::scrypt(password, {}, defaults, 32).has_value() == false);
	QVERIFY(PasswordKdf::scrypt(password, salt, defaults, PasswordKdf::MinimumOutputSize - 1).has_value() == false);
	QVERIFY(PasswordKdf::scrypt(password, salt, defaults, PasswordKdf::MaximumOutputSize + 1).has_value() == false);
}



void VeyonCryptoPrimitivesTest::argon2idKnownAnswer()
{
	if (PasswordKdf::isArgon2idAvailable() == false)
	{
		// built and running with OpenSSL >= 3.2: a missing Argon2id is a failure, not a skip
		QVERIFY(OpenSslCompat::isArgon2idExpected() == false);
		QCOMPARE(PasswordKdf::preferredAlgorithm(), PasswordKdf::Algorithm::Scrypt);
		QVERIFY(PasswordKdf::argon2id(QByteArray("password"), QByteArray(16, 0x01), {}, 32).has_value() == false);
		QSKIP("Argon2id requires OpenSSL >= 3.2");
	}

	QCOMPARE(PasswordKdf::preferredAlgorithm(), PasswordKdf::Algorithm::Argon2id);

	// RFC 9106 section 5.3 (Argon2id, version 0x13; with secret and associated data, which only
	// the internal compat function exposes)
	const QByteArray password(32, 0x01);
	const QByteArray salt(16, 0x02);
	const QByteArray secret(8, 0x03);
	const QByteArray associatedData(12, 0x04);
	QByteArray tag(32, 0);
	QVERIFY(OpenSslCompat::argon2id(reinterpret_cast<const unsigned char*>(password.constData()), size_t(password.size()),
									reinterpret_cast<const unsigned char*>(salt.constData()), size_t(salt.size()),
									reinterpret_cast<const unsigned char*>(secret.constData()), size_t(secret.size()),
									reinterpret_cast<const unsigned char*>(associatedData.constData()), size_t(associatedData.size()),
									3, 32, 4, reinterpret_cast<unsigned char*>(tag.data()), size_t(tag.size())));
	QCOMPARE(tag.toHex(), QByteArray("0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb52520e96b01e659"));

	// public API: reference implementation test vector (phc-winner-argon2, Argon2id v19, t=2,
	// m=64 MiB, p=1, "password"/"somesalt")
	PasswordKdf::Argon2idParameters parameters;
	parameters.iterations = 2;
	const auto key = PasswordKdf::argon2id(QByteArray("password"), QByteArray("somesalt"), parameters, 32);
	QCOMPARE(toByteArray(key).toHex(), QByteArray("09316115d5cf24ed5a15a31a3ba326e5cf32edc24702987c02b6566f61913cf7"));
}



void VeyonCryptoPrimitivesTest::argon2idParameters()
{
	// spec default (03 §2 item 6): t=3, m=64 MiB, p=1
	const PasswordKdf::Argon2idParameters defaults;
	QCOMPARE(defaults.iterations, quint32(3));
	QCOMPARE(defaults.memoryKiB, quint32(64 * 1024));
	QCOMPARE(defaults.parallelism, quint32(1));

	const QByteArray password("correct horse battery staple");
	const QByteArray salt(PasswordKdf::SaltSize, 0x01);

	auto parameters = defaults;
	parameters.memoryKiB = PasswordKdf::MaximumArgon2idMemoryKiB + 1;
	QVERIFY(PasswordKdf::argon2id(password, salt, parameters, 32).has_value() == false);
	parameters = defaults;
	parameters.iterations = 0;
	QVERIFY(PasswordKdf::argon2id(password, salt, parameters, 32).has_value() == false);
	parameters = defaults;
	parameters.parallelism = PasswordKdf::MaximumParallelism + 1;
	QVERIFY(PasswordKdf::argon2id(password, salt, parameters, 32).has_value() == false);
	QVERIFY(PasswordKdf::argon2id(password, salt.left(7), defaults, 32).has_value() == false);

	if (PasswordKdf::isArgon2idAvailable() == false)
	{
		QSKIP("Argon2id requires OpenSSL >= 3.2");
	}

	const auto first = PasswordKdf::argon2id(password, salt, defaults, 32);
	const auto second = PasswordKdf::argon2id(password, salt, defaults, 32);
	QVERIFY(first.has_value() && second.has_value());
	QVERIFY(Veyon::Crypto::constantTimeEquals(*first, *second));
}



void VeyonCryptoPrimitivesTest::errorsContainNoSecrets()
{
	// DoD 3: failures never put key material into error strings or log output
	capturedMessages.clear();
	const auto previousHandler = qInstallMessageHandler(captureMessage);

	const auto key = hex("5ec7e75ec7e75ec7e75ec7e75ec7e75ec7e75ec7e75ec7e75ec7e75ec7e75ec7");
	const auto password = QByteArray("Pa55phrase-Never-Logged");
	QStringList errors;

	QVERIFY(Aead::open(Aead::Algorithm::Aes256Gcm, key, QByteArray(12, 0), QByteArray(40, 0x33), {}).has_value() == false);
	errors.append(lastError());
	QVERIFY(Aead::seal(Aead::Algorithm::Aes256Gcm, QByteArray(key + key), QByteArray(12, 0), password, {}).has_value() == false);
	errors.append(lastError());
	QVERIFY(Hkdf::sha256(key, {}, {}, 0).has_value() == false);
	errors.append(lastError());
	PasswordKdf::ScryptParameters invalidParameters;
	invalidParameters.cost = 3;
	QVERIFY(PasswordKdf::scrypt(password, QByteArray(16, 0), invalidParameters, 32).has_value() == false);
	errors.append(lastError());

	qInstallMessageHandler(previousHandler);

	for (const auto& error : std::as_const(errors))
	{
		QVERIFY(error.isEmpty() == false);
		QVERIFY(error.contains(QString::fromLatin1(key.toHex()), Qt::CaseInsensitive) == false);
		QVERIFY(error.contains(QString::fromLatin1(password)) == false);
	}
	for (const auto& message : std::as_const(capturedMessages))
	{
		QVERIFY(message.contains(QString::fromLatin1(key.toHex()), Qt::CaseInsensitive) == false);
		QVERIFY(message.contains(QString::fromLatin1(password)) == false);
	}
}



void VeyonCryptoPrimitivesTest::operationsWithFullSecureHeap()
{
	// once the secure heap is initialised, OpenSSL takes private key internals from it and fails
	// instead of falling back to the normal heap, so SecureBuffers must leave room for it
	if (SecureBuffer::initializeSecureHeap(64 * 1024) == SecureBuffer::SecureHeapState::Unavailable)
	{
		QSKIP("OpenSSL secure heap not available on this platform");
	}

	const auto previousHandler = qInstallMessageHandler(captureMessage);
	std::vector<SecureBuffer> buffers;
	// bulk buffers that would exhaust the heap on their own ...
	buffers.emplace_back(30000);
	buffers.emplace_back(30000);
	// ... and more small ones than fit into it
	for (int i = 0; i < 32; ++i)
	{
		buffers.emplace_back(qsizetype(SecureBuffer::MaximumSecureHeapAllocationSize));
	}
	qInstallMessageHandler(previousHandler);
	QVERIFY(Hkdf::sha256(QByteArray(32, 'k'), {}, QByteArray(16, 'i'), 32).has_value());

	for (const auto algorithm : {KeyAlgorithm::EcP256, KeyAlgorithm::Ed25519, KeyAlgorithm::Rsa})
	{
		const auto key = PrivateKey::generate(algorithm);
		QVERIFY2(key.has_value(), qPrintable(lastError()));
		QVERIFY(key->sign(QByteArray("message")).has_value());

		const auto pem = key->toPkcs8Pem();
		QVERIFY(pem.has_value());
		const auto loadedKey = PrivateKey::fromPkcs8Pem(*pem);
		QVERIFY2(loadedKey.has_value(), qPrintable(lastError()));
		const auto signature = loadedKey->sign(QByteArray("message"));
		QVERIFY(signature.has_value());
		QVERIFY(key->publicKey().verify(QByteArray("message"), *signature));
	}

	QVERIFY(buffers[0].isInSecureHeap() == false);
	QVERIFY(buffers[2].isInSecureHeap());
	QVERIFY(buffers.back().isInSecureHeap() == false);
}



QTEST_GUILESS_MAIN(VeyonCryptoPrimitivesTest)
#include "VeyonCryptoPrimitivesTest.moc"
