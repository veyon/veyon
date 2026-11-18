// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSslKey>
#include <QtTest>

#include "Certificate.h"
#include "CertificateAuthority.h"
#include "CertificateRequest.h"
#include "CryptoPrivate.h"
#include "Hash.h"
#include "PrivateKey.h"

#include <algorithm>
#include <ctime>
#include <functional>
#include <map>
#include <openssl/pem.h>
#include <openssl/rsa.h>

using namespace Veyon::Crypto;

Q_DECLARE_METATYPE(Veyon::Crypto::KeyAlgorithm)

static QByteArray hex(const char* data)
{
	return QByteArray::fromHex(QByteArray(data));
}



// foreign keys/requests created with the openssl command line tool (throw-away test material)
static const QByteArray P384PrivateKeyPem(R"(-----BEGIN PRIVATE KEY-----
MIG2AgEAMBAGByqGSM49AgEGBSuBBAAiBIGeMIGbAgEBBDCtJimqJ1dRjNd/9EzJ
BEBbSSCi45kvSgksy8/OyuppXbvFC5ZAatwlvs++SPtaIiKhZANiAARxR4xLLEvG
e4itqOKwmMMaVHl6MLbQmNHd7Lz5rbslblo/L+qpe9BktiEiDi1U8CcyXX0DU1Hn
3Akfd6qS1ExPx2oDuDX8Y4O9rkiSCGciPglbxB7sWJPMCGg+yuoMlSc=
-----END PRIVATE KEY-----
)");

static const QByteArray Secp256k1PrivateKeyPem(R"(-----BEGIN PRIVATE KEY-----
MIGEAgEAMBAGByqGSM49AgEGBSuBBAAKBG0wawIBAQQgTEXaqNPwMgI1M9AzekST
lX3bSX1bLF5gNCccuaFyNnShRANCAATLjeEGXzMFxPGRBlxV6a1wlw4NBsZ6NdN0
CL3M8GVc01/Jh7mQqRoaUD+bgwPzjNPDtlSz2nB4CgLS91JaXMDu
-----END PRIVATE KEY-----
)");

static const QByteArray P256ExplicitParametersPrivateKeyPem(R"(-----BEGIN PRIVATE KEY-----
MIIBeQIBADCCAQMGByqGSM49AgEwgfcCAQEwLAYHKoZIzj0BAQIhAP////8AAAAB
AAAAAAAAAAAAAAAA////////////////MFsEIP////8AAAABAAAAAAAAAAAAAAAA
///////////////8BCBaxjXYqjqT57PrvVV2mIa8ZR0GsMxTsPY7zjw+J9JgSwMV
AMSdNgiG5wSTamZ44ROdJreBn36QBEEEaxfR8uEsQkf4vOblY6RA8ncDfYEt6zOg
9KE5RdiYwpZP40Li/hp/m47n60p8D54WK84zV2sxXs7LtkBoN79R9QIhAP////8A
AAAA//////////+85vqtpxeehPO5ysL8YyVRAgEBBG0wawIBAQQgI3hkg+eA3CUm
wfhCmAJOR3+jFBz2pZmvXg6MWL063syhRANCAASwrCcCkOB+bvA5Rfqi85wkjTkv
Re8v6MYkM5zC+ACbhkbA75ZJLnyDNFTsexEoKEOvXVm1nkayBjjBYwLcKQae
-----END PRIVATE KEY-----
)");

static const QByteArray P256Sec1PrivateKeyPem(R"(-----BEGIN EC PRIVATE KEY-----
MHcCAQEEID1jvHZJCbEzyjD5UUMGMZlPtP+BjPJugwXlDkabEyxyoAoGCCqGSM49
AwEHoUQDQgAE1pg0c3mgqVdXsJWFixYXUAvtlgKneBFZcrBHCbVpbQ25l1JZIqEp
UeN+52okD5VARSm8L3iBKVfoux0c2nMjdQ==
-----END EC PRIVATE KEY-----
)");

static const QByteArray P256EncryptedPrivateKeyPem(R"(-----BEGIN ENCRYPTED PRIVATE KEY-----
MIHsMFcGCSqGSIb3DQEFDTBKMCkGCSqGSIb3DQEFDDAcBAgoeyiszY+8XgICCAAw
DAYIKoZIhvcNAgkFADAdBglghkgBZQMEASoEEIQws73hpIaO5Y9HcHtXcqMEgZDS
FBOJ97jjPk3LIFDsomNjBq6gjMJpiHKYRohWLw/i0Acxin63h++Z/x9LIUqau32M
z5CZvyppOwpiqEWTlYhT0feGyv82XuQIQ3aHuK4DlmbFcxUSTiBK9RDcqaql1i/8
h0Xg7BCjXKHym96RrPSqF82jhbSdq1QjxqPI36xIndyKNUThHYQ2coYdaLlP1O4=
-----END ENCRYPTED PRIVATE KEY-----
)");

static const QByteArray Rsa2048PublicKeyPem(R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA02h68276YDbPTFCPyKJV
pgVN4SUuRzg3ddf8/dy4O2xRSa6RPTPHdGJtGIN5br1niS4j5p+gBu+95pyAzG2B
WYSNHpAn072JNpk++bvt3hH7Q7NcWouKAkSWoMASKJHjUTjPRurxcDonE6TMgFAB
zJKpx8xAmm3V/UnNKo82jk4drvhIj7AkYAUH8T0U2FKRzpdrX5cy//nfDPnl0KBx
CfhVchaWuZNY9rg7rIm57QnLEh4UvqxqjIqrMeDPUdjd1fnBXJ61SZ8fXeGCanp4
GRFD7a/leQxY4y5zFxKwCO15PGckvcCwGVIT8dU74ydR69qer4FZGBnb+rjRCYv5
ywIDAQAB
-----END PUBLIC KEY-----
)");

// valid self-signature, but ecdsa-with-SHA1
static const QByteArray Sha1SignedRequestPem(R"(-----BEGIN CERTIFICATE REQUEST-----
MIHNMHYCAQAwFDESMBAGA1UEAwwJc2hhMS1ob3N0MFkwEwYHKoZIzj0CAQYIKoZI
zj0DAQcDQgAE1pg0c3mgqVdXsJWFixYXUAvtlgKneBFZcrBHCbVpbQ25l1JZIqEp
UeN+52okD5VARSm8L3iBKVfoux0c2nMjdaAAMAkGByqGSM49BAEDSAAwRQIgO7el
bl2Y8hblCaf++qpQPBNSYZ4Sdemcc19un+gFASUCIQDu89SdNFKGVgBEFzuj/VIL
kywkCdjIbshZgVzK6gUzVg==
-----END CERTIFICATE REQUEST-----
)");


// EC P-256 key with compressed public point (PKCS#8) and a self-signed CA certificate for it whose
// SubjectPublicKeyInfo carries the compressed point (59 bytes)
static const QByteArray CompressedP256PrivateKeyPem(R"(-----BEGIN PRIVATE KEY-----
MGcCAQAwEwYHKoZIzj0CAQYIKoZIzj0DAQcETTBLAgEBBCA9Y7x2SQmxM8ow+VFD
BjGZT7T/gYzyboMF5Q5GmxMscqEkAyIAA9aYNHN5oKlXV7CVhYsWF1AL7ZYCp3gR
WXKwRwm1aW0N
-----END PRIVATE KEY-----
)");

static const QByteArray CompressedP256CaCertificatePem(R"(-----BEGIN CERTIFICATE-----
MIIBeDCCAR6gAwIBAgIUFsFcOj0yG4Vhs0tqNu+0OAi54+kwCgYIKoZIzj0EAwIw
GDEWMBQGA1UEAwwNQ29tcHJlc3NlZCBDQTAeFw0yNjExMTYxNjIwMjJaFw0zNjEx
MTMxNjIwMjJaMBgxFjAUBgNVBAMMDUNvbXByZXNzZWQgQ0EwOTATBgcqhkjOPQIB
BggqhkjOPQMBBwMiAAPWmDRzeaCpV1ewlYWLFhdQC+2WAqd4EVlysEcJtWltDaNm
MGQwHQYDVR0OBBYEFC9IdoLRpuq+XcljTnoyhfcOxeCVMB8GA1UdIwQYMBaAFC9I
doLRpuq+XcljTnoyhfcOxeCVMBIGA1UdEwEB/wQIMAYBAf8CAQAwDgYDVR0PAQH/
BAQDAgEGMAoGCCqGSM49BAMCA0gAMEUCIFP9h3gLqfQqZ2n/nl4cPXJM4R7oMy6t
/wCBMomOSBMLAiEAvrxgRba5XM/7B3+w23KEz51rygj8HSxHVUiZV3iqA38=
-----END CERTIFICATE-----
)");

// SHA-256 over bytes 131..189 of the DER certificate (its SubjectPublicKeyInfo), computed
// independently of OpenSSL
static const QByteArray CompressedP256CaPin("484e73009359817ab494ccc701e1a30b5e68b3a743c1485ce82ba81cd15c4908");


class VeyonCryptoPkiTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();

	void ed25519KnownAnswers_data();
	void ed25519KnownAnswers();

	void keyAlgorithms_data();
	void keyRoundTrip_data();
	void keyRoundTrip();
	void signatureRejectsTampering_data();
	void signatureRejectsTampering();
	void keyPolicy();
	void privateKeyImportStrictness();

	void certificateRequestRoundTrip_data();
	void certificateRequestRoundTrip();
	void certificateRequestRejectsInvalid();

	void caCertificate();
	void issueEndEntity_data();
	void issueEndEntity();
	void issueRejectsInvalidInput();

	void certificatePemChain();
	void certificateDerStrictness();
	void qsslCertificateInterop_data();
	void qsslCertificateInterop();
	void malformedCertificates();
	void compressedPointKeys();
	void strictDer();
	void pemLabelsAndBlockCount();
	void certificateTimeFormat_data();
	void certificateTimeFormat();
	void displaySafeNames_data();
	void displaySafeNames();
	void criticalExtensionsAndSanTypes();
	void usageExtensions();
	void nullAndCopiedObjects();
	void invalidInputSizes();
	void unsupportedKeyTypes();
	void rsaPssParameters_data();
	void rsaPssParameters();
	void canonicalPublicKeyEncoding();
	void canonicalSignatureAlgorithms();
	void canonicalCriticalFlag();

private:
	const PrivateKey& key(KeyAlgorithm algorithm) const
	{
		return m_keys.at(algorithm);
	}

	CertificateAuthority::IssuanceProfile profile() const;

	std::map<KeyAlgorithm, PrivateKey> m_keys;
	PrivateKey m_caKey;
	Certificate m_caCertificate;
	QDateTime m_now;
};



void VeyonCryptoPkiTest::initTestCase()
{
	for (const auto algorithm : {KeyAlgorithm::EcP256, KeyAlgorithm::Ed25519, KeyAlgorithm::Rsa})
	{
		auto generatedKey = PrivateKey::generate(algorithm);
		QVERIFY(generatedKey.has_value());
		m_keys.emplace(algorithm, std::move(*generatedKey));
	}

	auto caKey = PrivateKey::generate(KeyAlgorithm::EcP256);
	QVERIFY(caKey.has_value());
	m_caKey = std::move(*caKey);

	// whole seconds: X.509 times have no sub-second precision
	m_now = QDateTime::fromSecsSinceEpoch(QDateTime::currentSecsSinceEpoch()).toUTC();

	CertificateAuthority::CaParameters parameters;
	parameters.commonName = QStringLiteral("Veyon CA Test School");
	parameters.organization = QStringLiteral("Test School");
	parameters.currentTime = m_now;
	const auto caCertificate = CertificateAuthority::createCaCertificate(m_caKey, parameters);
	QVERIFY2(caCertificate.has_value(), qPrintable(lastError()));
	m_caCertificate = *caCertificate;
}



CertificateAuthority::IssuanceProfile VeyonCryptoPkiTest::profile() const
{
	CertificateAuthority::IssuanceProfile issuanceProfile;
	issuanceProfile.commonName = QStringLiteral("pc01.school.example");
	issuanceProfile.dnsNames = QStringList{QStringLiteral("pc01.school.example"), QStringLiteral("pc01")};
	issuanceProfile.ipAddresses = {QHostAddress(QStringLiteral("192.0.2.10")), QHostAddress(QStringLiteral("2001:db8::10"))};
	issuanceProfile.currentTime = m_now;
	return issuanceProfile;
}



void VeyonCryptoPkiTest::ed25519KnownAnswers_data()
{
	QTest::addColumn<QByteArray>("seed");
	QTest::addColumn<QByteArray>("publicKey");
	QTest::addColumn<QByteArray>("message");
	QTest::addColumn<QByteArray>("signature");

	// RFC 8032 section 7.1, TEST 1 and TEST 2
	QTest::newRow("rfc8032-1") << hex("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60")
							   << hex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a")
							   << QByteArray()
							   << hex("e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46b"
									  "d25bf5f0595bbe24655141438e7a100b");
	QTest::newRow("rfc8032-2") << hex("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb")
							   << hex("3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c")
							   << hex("72")
							   << hex("92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c"
									  "387b2eaeb4302aeeb00d291612bb0c00");
}



void VeyonCryptoPkiTest::ed25519KnownAnswers()
{
	QFETCH(QByteArray, seed);
	QFETCH(QByteArray, publicKey);
	QFETCH(QByteArray, message);
	QFETCH(QByteArray, signature);

	const auto privateKey = PrivateKey::fromEd25519Seed(SecureBuffer::fromByteArray(seed));
	QVERIFY(privateKey.has_value());
	QCOMPARE(privateKey->algorithm(), KeyAlgorithm::Ed25519);
	QCOMPARE(privateKey->publicKey().toEd25519RawKey().toHex(), publicKey.toHex());

	const auto computedSignature = privateKey->sign(message);
	QVERIFY(computedSignature.has_value());
	QCOMPARE(computedSignature->toHex(), signature.toHex());

	const auto verificationKey = PublicKey::fromEd25519RawKey(publicKey);
	QVERIFY(verificationKey.has_value());
	QVERIFY(verificationKey->verify(message, signature));
	QVERIFY(verificationKey->verify(QByteArray(message + "x"), signature) == false);
	QVERIFY(PublicKey::fromEd25519RawKey(publicKey.left(31)).has_value() == false);
	QVERIFY(PrivateKey::fromEd25519Seed(seed.left(31)).has_value() == false);
}



void VeyonCryptoPkiTest::keyAlgorithms_data()
{
	QTest::addColumn<KeyAlgorithm>("algorithm");
	QTest::newRow("ec-p256") << KeyAlgorithm::EcP256;
	QTest::newRow("ed25519") << KeyAlgorithm::Ed25519;
	QTest::newRow("rsa-3072") << KeyAlgorithm::Rsa;
}



void VeyonCryptoPkiTest::keyRoundTrip_data()
{
	keyAlgorithms_data();
}



void VeyonCryptoPkiTest::keyRoundTrip()
{
	QFETCH(KeyAlgorithm, algorithm);
	const auto& privateKey = key(algorithm);

	QCOMPARE(privateKey.algorithm(), algorithm);
	QCOMPARE(privateKey.bits(), algorithm == KeyAlgorithm::Rsa ? 3072 : 256);
	QCOMPARE(privateKey.publicKey().bits(), privateKey.bits());

	const auto publicKey = privateKey.publicKey();
	QCOMPARE(publicKey.algorithm(), algorithm);

	// PKCS#8 DER and PEM
	const auto der = privateKey.toPkcs8Der();
	const auto pem = privateKey.toPkcs8Pem();
	QVERIFY(der.has_value() && pem.has_value());
	QVERIFY(pem->toUnprotectedByteArray().startsWith("-----BEGIN PRIVATE KEY-----\n"));
	QVERIFY(pem->toUnprotectedByteArray().endsWith("-----END PRIVATE KEY-----\n"));
	const auto fromDer = PrivateKey::fromPkcs8Der(*der);
	const auto fromPem = PrivateKey::fromPkcs8Pem(*pem);
	QVERIFY(fromDer.has_value() && fromPem.has_value());
	QCOMPARE(fromDer->publicKey(), publicKey);
	QCOMPARE(fromPem->publicKey(), publicKey);

	// the PEM encoding must be readable by Qt (05 §1.3 QSslKey import)
	if (algorithm != KeyAlgorithm::Ed25519)
	{
		const QSslKey sslKey(pem->toUnprotectedByteArray(), algorithm == KeyAlgorithm::Rsa ? QSsl::Rsa : QSsl::Ec, QSsl::Pem);
		QVERIFY(sslKey.isNull() == false);
	}

	// SubjectPublicKeyInfo DER and PEM
	const auto publicFromDer = PublicKey::fromDer(publicKey.toDer());
	const auto publicFromPem = PublicKey::fromPem(publicKey.toPem());
	QVERIFY(publicFromDer.has_value() && publicFromPem.has_value());
	QCOMPARE(*publicFromDer, publicKey);
	QCOMPARE(*publicFromPem, publicKey);
	QCOMPARE(publicKey.spkiSha256(), Hash::sha256(publicKey.toDer()).value());
	QCOMPARE(publicKey.spkiFingerprint(), QStringLiteral("sha256:") + QString::fromLatin1(publicKey.spkiSha256().toHex()));

	// sign/verify with the bound scheme; signatures from other keys fail
	const QByteArray message("veyon-renewal-proof");
	const auto signature = privateKey.sign(message);
	QVERIFY(signature.has_value());
	QVERIFY(publicKey.verify(message, *signature));
	QVERIFY(fromPem->publicKey().verify(message, *signature));
	for (const auto& [otherAlgorithm, otherKey] : m_keys)
	{
		if (otherAlgorithm != algorithm)
		{
			QVERIFY(otherKey.publicKey().verify(message, *signature) == false);
		}
	}

	const auto otherKey = PrivateKey::generate(algorithm == KeyAlgorithm::Rsa ? KeyAlgorithm::EcP256 : algorithm);
	QVERIFY(otherKey.has_value());
	QVERIFY(otherKey->publicKey() != publicKey);
	QVERIFY(otherKey->publicKey().verify(message, *signature) == false);
}



void VeyonCryptoPkiTest::signatureRejectsTampering_data()
{
	keyAlgorithms_data();
}



void VeyonCryptoPkiTest::signatureRejectsTampering()
{
	QFETCH(KeyAlgorithm, algorithm);
	const auto& privateKey = key(algorithm);
	const auto publicKey = privateKey.publicKey();

	const QByteArray message("message");
	const auto signature = privateKey.sign(message);
	QVERIFY(signature.has_value());

	auto tampered = *signature;
	tampered[tampered.size() / 2] = char(tampered[tampered.size() / 2] ^ 0x04);
	QVERIFY(publicKey.verify(message, tampered) == false);
	QVERIFY(publicKey.verify(QByteArray("messagf"), *signature) == false);
	QVERIFY(publicKey.verify(message, signature->left(signature->size() - 1)) == false);
	QVERIFY(publicKey.verify(message, QByteArray(*signature + QByteArray(1, 0))) == false);
	QVERIFY(publicKey.verify(message, {}) == false);
	QVERIFY(PublicKey().verify(message, *signature) == false);
	QVERIFY(PrivateKey().sign(message).has_value() == false);

	if (algorithm == KeyAlgorithm::Rsa)
	{
		// only RSASSA-PSS with SHA-256, MGF1-SHA256 and a 32-byte salt verifies (no PKCS#1 v1.5,
		// no other salt lengths)
		const auto foreignSignature = [&](int padding, int saltLength) {
			Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
			EVP_PKEY_CTX* keyContext = nullptr;
			size_t size = 1024;
			QByteArray result(int(size), 0);
			if (EVP_DigestSignInit(context.get(), &keyContext, EVP_sha256(), nullptr, Internal::Access::nativeHandle(privateKey)) != 1 ||
				EVP_PKEY_CTX_set_rsa_padding(keyContext, padding) <= 0 ||
				(padding == RSA_PKCS1_PSS_PADDING && EVP_PKEY_CTX_set_rsa_pss_saltlen(keyContext, saltLength) <= 0) ||
				EVP_DigestSign(context.get(), reinterpret_cast<unsigned char*>(result.data()), &size,
							   reinterpret_cast<const unsigned char*>(message.constData()), size_t(message.size())) != 1)
			{
				return QByteArray{};
			}
			result.truncate(int(size));
			return result;
		};
		const auto pss32 = foreignSignature(RSA_PKCS1_PSS_PADDING, 32);
		const auto pss20 = foreignSignature(RSA_PKCS1_PSS_PADDING, 20);
		const auto pkcs1 = foreignSignature(RSA_PKCS1_PADDING, 0);
		QVERIFY(pss32.isEmpty() == false && pss20.isEmpty() == false && pkcs1.isEmpty() == false);
		QVERIFY(publicKey.verify(message, pss32));
		QVERIFY(publicKey.verify(message, pss20) == false);
		QVERIFY(publicKey.verify(message, pkcs1) == false);
	}
}



void VeyonCryptoPkiTest::keyPolicy()
{
	QVERIFY(PrivateKey::generate(KeyAlgorithm::Rsa, 2048).has_value() == false);
	QVERIFY(PrivateKey::generate(KeyAlgorithm::Rsa, PrivateKey::MaximumRsaBits + 1).has_value() == false);
	QVERIFY(PrivateKey::generate(KeyAlgorithm::Invalid).has_value() == false);

	// other curves, explicit curve parameters, too short RSA keys
	QVERIFY(PrivateKey::fromPkcs8Pem(P384PrivateKeyPem).has_value() == false);
	QVERIFY(lastError().contains(QStringLiteral("unsupported key")));
	QVERIFY(PrivateKey::fromPkcs8Pem(Secp256k1PrivateKeyPem).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Pem(P256ExplicitParametersPrivateKeyPem).has_value() == false);
	QVERIFY(PublicKey::fromPem(Rsa2048PublicKeyPem).has_value() == false);

	QVERIFY(PublicKey().isNull());
	QVERIFY(PublicKey() != PublicKey());
	QCOMPARE(PublicKey().algorithm(), KeyAlgorithm::Invalid);
	QVERIFY(PublicKey().toDer().isEmpty());
	QVERIFY(PrivateKey().toPkcs8Der().has_value() == false);
}



void VeyonCryptoPkiTest::privateKeyImportStrictness()
{
	// traditional (SEC1) and encrypted formats are not accepted, and no password is ever prompted for
	QVERIFY(PrivateKey::fromPkcs8Pem(P256Sec1PrivateKeyPem).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Pem(P256EncryptedPrivateKeyPem).has_value() == false);

	const auto validPem = key(KeyAlgorithm::EcP256).toPkcs8Pem()->toUnprotectedByteArray();
	auto legacyEncryptedPem = validPem;
	legacyEncryptedPem.insert(int(qstrlen("-----BEGIN PRIVATE KEY-----\n")),
							  "Proc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,00112233445566778899AABBCCDDEEFF\n\n");
	QVERIFY(PrivateKey::fromPkcs8Pem(legacyEncryptedPem).has_value() == false);

	const auto der = key(KeyAlgorithm::EcP256).toPkcs8Der()->toUnprotectedByteArray();
	QVERIFY(PrivateKey::fromPkcs8Der(QByteArray(der + QByteArray(1, 0))).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Der(der.left(der.size() - 1)).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Der({}).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Der(QByteArray(PrivateKey::MaximumEncodedSize + 1, 0x30)).has_value() == false);

	const auto publicDer = key(KeyAlgorithm::EcP256).publicKey().toDer();
	QVERIFY(PublicKey::fromDer(QByteArray(publicDer + QByteArray(1, 0))).has_value() == false);
	QVERIFY(PublicKey::fromDer(publicDer.left(publicDer.size() - 1)).has_value() == false);
	// a private key is not a public key and vice versa
	QVERIFY(PublicKey::fromDer(der).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Der(publicDer).has_value() == false);
}



void VeyonCryptoPkiTest::certificateRequestRoundTrip_data()
{
	keyAlgorithms_data();
}



void VeyonCryptoPkiTest::certificateRequestRoundTrip()
{
	QFETCH(KeyAlgorithm, algorithm);
	const auto& privateKey = key(algorithm);

	const auto request = CertificateRequest::create(privateKey, QStringLiteral("pc01.school.example"));
	QVERIFY2(request.has_value(), qPrintable(lastError()));
	QCOMPARE(request->subjectCommonName(), QStringLiteral("pc01.school.example"));
	QCOMPARE(request->subject(), QStringLiteral("CN=pc01.school.example"));
	QCOMPARE(request->publicKey(), privateKey.publicKey());

	const auto fromDer = CertificateRequest::fromDer(request->toDer());
	const auto fromPem = CertificateRequest::fromPem(request->toPem());
	QVERIFY(fromDer.has_value() && fromPem.has_value());
	QCOMPARE(fromDer->toDer(), request->toDer());
	QCOMPARE(fromPem->toDer(), request->toDer());
	QCOMPARE(fromPem->publicKey(), privateKey.publicKey());

	// broken self-signature (proof of possession)
	auto tampered = request->toDer();
	tampered[tampered.size() - 3] = char(tampered[tampered.size() - 3] ^ 0x01);
	QVERIFY(CertificateRequest::fromDer(tampered).has_value() == false);

	// changed content (subject) with the original signature
	auto changedSubject = request->toDer();
	const auto position = changedSubject.indexOf("pc01");
	QVERIFY(position > 0);
	changedSubject[position] = 'x';
	QVERIFY(CertificateRequest::fromDer(changedSubject).has_value() == false);

	QVERIFY(CertificateRequest::fromDer(QByteArray(request->toDer() + QByteArray(1, 0))).has_value() == false);
	QVERIFY(CertificateRequest::fromDer(request->toDer().left(request->toDer().size() - 1)).has_value() == false);
}



void VeyonCryptoPkiTest::certificateRequestRejectsInvalid()
{
	QVERIFY(CertificateRequest::create(key(KeyAlgorithm::EcP256), {}).has_value() == false);
	QVERIFY(CertificateRequest::create(key(KeyAlgorithm::EcP256), QString(65, QLatin1Char('a'))).has_value() == false);
	QVERIFY(CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("a\nb")).has_value() == false);
	QVERIFY(CertificateRequest::create(PrivateKey(), QStringLiteral("host")).has_value() == false);

	// valid proof of possession, but SHA-1 based signature
	QVERIFY(CertificateRequest::fromPem(Sha1SignedRequestPem).has_value() == false);
	QVERIFY(lastError().contains(QStringLiteral("signature algorithm")));

	QVERIFY(CertificateRequest::fromPem(QByteArray("garbage")).has_value() == false);
	QVERIFY(CertificateRequest::fromDer(QByteArray(100, 0x30)).has_value() == false);
	QVERIFY(CertificateRequest::fromDer(QByteArray(CertificateRequest::MaximumEncodedSize + 1, 0x30)).has_value() == false);
	QVERIFY(CertificateRequest().isNull());
	QVERIFY(CertificateRequest().publicKey().isNull());
}



void VeyonCryptoPkiTest::caCertificate()
{
	const auto& ca = m_caCertificate;

	QVERIFY(ca.isCa());
	QCOMPARE(ca.pathLengthConstraint(), 0);
	QVERIFY(ca.hasKeyUsageExtension());
	QVERIFY(ca.hasKeyUsage(Certificate::KeyUsage::KeyCertSign));
	QVERIFY(ca.hasKeyUsage(Certificate::KeyUsage::CrlSign));
	QVERIFY(ca.hasKeyUsage(Certificate::KeyUsage::DigitalSignature) == false);
	QVERIFY(ca.hasExtendedKeyUsageExtension() == false);
	QCOMPARE(ca.subjectCommonName(), QStringLiteral("Veyon CA Test School"));
	QCOMPARE(ca.subject(), QStringLiteral("O=Test School,CN=Veyon CA Test School"));
	QCOMPARE(ca.issuer(), ca.subject());
	QCOMPARE(ca.notBefore(), m_now.addSecs(-CertificateAuthority::NotBeforeBackdateSeconds));
	QCOMPARE(ca.notAfter(), m_now.addDays(CertificateAuthority::DefaultCaValidityDays));
	QCOMPARE(ca.publicKey(), m_caKey.publicKey());
	QVERIFY(ca.isSignedBy(m_caKey.publicKey()));
	QVERIFY(ca.isSignedBy(key(KeyAlgorithm::EcP256).publicKey()) == false);
	QVERIFY(ca.dnsNames().isEmpty());

	// CA key must be EC P-256, validity bounded, CN required
	CertificateAuthority::CaParameters parameters;
	parameters.commonName = QStringLiteral("Veyon CA");
	QVERIFY(CertificateAuthority::createCaCertificate(key(KeyAlgorithm::Rsa), parameters).has_value() == false);
	QVERIFY(CertificateAuthority::createCaCertificate(key(KeyAlgorithm::Ed25519), parameters).has_value() == false);
	QVERIFY(CertificateAuthority::createCaCertificate(PrivateKey(), parameters).has_value() == false);
	parameters.validityDays = CertificateAuthority::MaximumCaValidityDays + 1;
	QVERIFY(CertificateAuthority::createCaCertificate(m_caKey, parameters).has_value() == false);
	parameters.validityDays = CertificateAuthority::DefaultCaValidityDays;
	parameters.commonName.clear();
	QVERIFY(CertificateAuthority::createCaCertificate(m_caKey, parameters).has_value() == false);
}



void VeyonCryptoPkiTest::issueEndEntity_data()
{
	QTest::addColumn<KeyAlgorithm>("algorithm");
	QTest::newRow("ec-p256") << KeyAlgorithm::EcP256;
	QTest::newRow("rsa-3072") << KeyAlgorithm::Rsa;
}



void VeyonCryptoPkiTest::issueEndEntity()
{
	QFETCH(KeyAlgorithm, algorithm);

	const auto request = CertificateRequest::create(key(algorithm), QStringLiteral("ignored-request-name"));
	QVERIFY(request.has_value());

	const auto certificate = CertificateAuthority::issue(m_caCertificate, m_caKey, *request, profile());
	QVERIFY2(certificate.has_value(), qPrintable(lastError()));

	// names come from the profile only
	QCOMPARE(certificate->subjectCommonName(), QStringLiteral("pc01.school.example"));
	QCOMPARE(certificate->dnsNames(), QStringList({QStringLiteral("pc01.school.example"), QStringLiteral("pc01")}));
	QCOMPARE(certificate->ipAddresses(), QList<QHostAddress>({QHostAddress(QStringLiteral("192.0.2.10")), QHostAddress(QStringLiteral("2001:db8::10"))}));
	QCOMPARE(certificate->issuer(), m_caCertificate.subject());
	QCOMPARE(certificate->publicKey(), key(algorithm).publicKey());

	// issuance profile 06 §11.2
	QVERIFY(certificate->isCa() == false);
	QVERIFY(certificate->hasExtendedKeyUsageExtension());
	QVERIFY(certificate->hasExtendedKeyUsage(Certificate::ExtendedKeyUsage::ServerAuth));
	QVERIFY(certificate->hasExtendedKeyUsage(Certificate::ExtendedKeyUsage::ClientAuth) == false);
	QVERIFY(certificate->hasKeyUsage(Certificate::KeyUsage::DigitalSignature));
	QCOMPARE(certificate->hasKeyUsage(Certificate::KeyUsage::KeyEncipherment), algorithm == KeyAlgorithm::Rsa);
	QVERIFY(certificate->hasKeyUsage(Certificate::KeyUsage::KeyCertSign) == false);
	QCOMPARE(certificate->notBefore(), m_now.addSecs(-5 * 60));
	QCOMPARE(certificate->notAfter(), m_now.addDays(730));
	QVERIFY(certificate->isValidAt(m_now));
	QVERIFY(certificate->isValidAt(m_now.addDays(731)) == false);
	QVERIFY(certificate->isValidAt(m_now.addSecs(-301)) == false);

	// serial: random, positive, 128 bit
	QVERIFY(certificate->serialNumber().size() >= 8 && certificate->serialNumber().size() <= CertificateAuthority::SerialNumberSize);
	QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-f]+$")).match(certificate->serialNumberHex()).hasMatch());
	const auto secondCertificate = CertificateAuthority::issue(m_caCertificate, m_caKey, *request, profile());
	QVERIFY(secondCertificate.has_value());
	QVERIFY(secondCertificate->serialNumber() != certificate->serialNumber());

	QVERIFY(certificate->isSignedBy(m_caKey.publicKey()));
	QVERIFY(certificate->isSignedBy(key(algorithm).publicKey()) == false);
}



void VeyonCryptoPkiTest::issueRejectsInvalidInput()
{
	const auto request = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("host"));
	QVERIFY(request.has_value());
	QVERIFY(CertificateAuthority::issue(m_caCertificate, m_caKey, *request, profile()).has_value());

	// Ed25519 keys are not issued (TLS interoperability), null requests neither
	const auto ed25519Request = CertificateRequest::create(key(KeyAlgorithm::Ed25519), QStringLiteral("host"));
	QVERIFY(ed25519Request.has_value());
	QVERIFY(CertificateAuthority::issue(m_caCertificate, m_caKey, *ed25519Request, profile()).has_value() == false);
	QVERIFY(CertificateAuthority::issue(m_caCertificate, m_caKey, CertificateRequest(), profile()).has_value() == false);

	// CA key not matching the CA certificate, CA certificate that is no CA
	QVERIFY(CertificateAuthority::issue(m_caCertificate, key(KeyAlgorithm::EcP256), *request, profile()).has_value() == false);
	const auto endEntity = CertificateAuthority::issue(m_caCertificate, m_caKey, *request, profile());
	QVERIFY(CertificateAuthority::issue(*endEntity, key(KeyAlgorithm::EcP256), *request, profile()).has_value() == false);
	QVERIFY(CertificateAuthority::issue(Certificate(), m_caKey, *request, profile()).has_value() == false);

	const auto rejects = [&](const std::function<void(CertificateAuthority::IssuanceProfile&)>& modify) {
		auto modifiedProfile = profile();
		modify(modifiedProfile);
		return CertificateAuthority::issue(m_caCertificate, m_caKey, *request, modifiedProfile).has_value() == false;
	};

	QVERIFY(rejects([](auto& p) { p.dnsNames.clear(); p.ipAddresses.clear(); }));
	for (const auto& invalidName : {"PC01.example", "*.example", "a..example", "example.", "-a.example", "a-.example",
									"a_b.example", "192.0.2.1", "a b", "", "host\n", "a\n.example", "\nhost"})
	{
		QVERIFY2(rejects([&](auto& p) { p.dnsNames = QStringList{QString::fromLatin1(invalidName)}; }), invalidName);
		// rejected by the name validation, not only later by the self-check of the issued certificate
		QVERIFY2(lastError().contains(QStringLiteral("invalid subject alternative names")), invalidName);
	}
	QVERIFY(rejects([](auto& p) { p.dnsNames = QStringList{QString(64, QLatin1Char('a')) + QStringLiteral(".example")}; }));
	QVERIFY(rejects([](auto& p) { p.ipAddresses = {QHostAddress()}; }));
	QVERIFY(rejects([](auto& p) { p.ipAddresses = {QHostAddress(QStringLiteral("fe80::1%eth0"))}; }));
	QVERIFY(rejects([](auto& p) {
		p.dnsNames.clear();
		for (int i = 0; i <= CertificateAuthority::MaximumSubjectAlternativeNames; ++i)
		{
			p.dnsNames.append(QStringLiteral("host%1").arg(i));
		}
	}));
	QVERIFY(rejects([](auto& p) { p.commonName.clear(); }));
	QVERIFY(rejects([](auto& p) { p.validityDays = 0; }));
	QVERIFY(rejects([](auto& p) { p.validityDays = CertificateAuthority::MaximumEndEntityValidityDays + 1; }));
	// must not outlive the CA certificate; CA must be valid at issuance time
	QVERIFY(rejects([this](auto& p) { p.currentTime = m_now.addDays(CertificateAuthority::DefaultCaValidityDays - 100); }));
	QVERIFY(rejects([this](auto& p) { p.currentTime = m_now.addDays(-1); }));

	auto maximumProfile = profile();
	maximumProfile.validityDays = CertificateAuthority::MaximumEndEntityValidityDays;
	maximumProfile.dnsNames = QStringList{QString(63, QLatin1Char('a')) + QStringLiteral(".example")};
	QVERIFY(CertificateAuthority::issue(m_caCertificate, m_caKey, *request, maximumProfile).has_value());
}



void VeyonCryptoPkiTest::certificatePemChain()
{
	const auto request = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("host"));
	const auto leaf = CertificateAuthority::issue(m_caCertificate, m_caKey, *request, profile());
	QVERIFY(leaf.has_value());

	const QByteArray chainPem = leaf->toPem() + m_caCertificate.toPem();
	const auto chain = Certificate::chainFromPem(chainPem);
	QVERIFY(chain.has_value());
	QCOMPARE(chain->size(), 2);
	QCOMPARE(chain->at(0), *leaf);
	QCOMPARE(chain->at(1), m_caCertificate);

	// text around and between the blocks is ignored, other block types are skipped
	const QByteArray annotated = QByteArray("leaf:\n") + leaf->toPem() + request->toPem() + "ca:\n" + m_caCertificate.toPem() + "end\n";
	QCOMPARE(Certificate::chainFromPem(annotated)->size(), 2);

	const auto single = Certificate::fromPem(leaf->toPem());
	QVERIFY(single.has_value());
	QCOMPARE(*single, *leaf);
	QVERIFY(Certificate::fromPem(chainPem).has_value() == false);

	QVERIFY(Certificate::chainFromPem({}).has_value() == false);
	QVERIFY(Certificate::chainFromPem(QByteArray("no certificate")).has_value() == false);
	QVERIFY(Certificate::chainFromPem(request->toPem()).has_value() == false);

	// a corrupted block anywhere fails the whole chain
	auto corrupted = chainPem;
	corrupted[leaf->toPem().size() + 40] = '!';
	QVERIFY(Certificate::chainFromPem(corrupted).has_value() == false);
	QVERIFY(Certificate::chainFromPem(chainPem.left(chainPem.size() - 30)).has_value() == false);

	QByteArray tooLong;
	for (int i = 0; i <= Certificate::MaximumChainLength; ++i)
	{
		tooLong += m_caCertificate.toPem();
	}
	QVERIFY(Certificate::chainFromPem(tooLong).has_value() == false);

	// legacy PEM encryption headers must not lead to a password prompt
	auto encryptedHeader = leaf->toPem();
	encryptedHeader.insert(int(qstrlen("-----BEGIN CERTIFICATE-----\n")),
						   "Proc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,00112233445566778899AABBCCDDEEFF\n\n");
	QVERIFY(Certificate::chainFromPem(encryptedHeader).has_value() == false);
}



void VeyonCryptoPkiTest::certificateDerStrictness()
{
	const auto der = m_caCertificate.toDer();
	QVERIFY(Certificate::fromDer(der).has_value());
	QVERIFY(Certificate::fromDer(QByteArray(der + QByteArray(1, 0))).has_value() == false);
	QVERIFY(Certificate::fromDer(der.left(der.size() - 1)).has_value() == false);
	QVERIFY(Certificate::fromDer({}).has_value() == false);
	QVERIFY(Certificate::fromDer(QByteArray(Certificate::MaximumEncodedSize + 1, 0x30)).has_value() == false);

	auto tamperedSignature = der;
	tamperedSignature[tamperedSignature.size() - 2] = char(tamperedSignature[tamperedSignature.size() - 2] ^ 0x01);
	const auto parsedTampered = Certificate::fromDer(tamperedSignature);
	// parsing does not verify signatures, isSignedBy() does
	QVERIFY(parsedTampered.has_value());
	QVERIFY(parsedTampered->isSignedBy(m_caKey.publicKey()) == false);
	QVERIFY(Certificate().isSignedBy(m_caKey.publicKey()) == false);
	QVERIFY(m_caCertificate.isSignedBy(PublicKey()) == false);
	QVERIFY(Certificate() != Certificate());
}



void VeyonCryptoPkiTest::qsslCertificateInterop_data()
{
	issueEndEntity_data();
}



void VeyonCryptoPkiTest::qsslCertificateInterop()
{
	QFETCH(KeyAlgorithm, algorithm);

	const auto request = CertificateRequest::create(key(algorithm), QStringLiteral("host"));
	const auto certificate = CertificateAuthority::issue(m_caCertificate, m_caKey, *request, profile());
	QVERIFY(certificate.has_value());

	const auto sslCertificate = certificate->toQSslCertificate();
	QVERIFY(sslCertificate.isNull() == false);
	QCOMPARE(sslCertificate.toDer(), certificate->toDer());
	QCOMPARE(sslCertificate.subjectInfo(QSslCertificate::CommonName), QStringList{QStringLiteral("pc01.school.example")});
	QCOMPARE(sslCertificate.expiryDate(), certificate->notAfter());
	auto qtDnsNames = sslCertificate.subjectAlternativeNames().values(QSsl::DnsEntry);
	std::sort(qtDnsNames.begin(), qtDnsNames.end());
	QCOMPARE(qtDnsNames, QList<QString>({QStringLiteral("pc01"), QStringLiteral("pc01.school.example")}));

	// for uncompressed keys Qt's re-encoding happens to give the same pin; compressed points do not
	// (see compressedPointKeys()), which is why only Certificate::spkiSha256() is normative
	const auto qtPin = Hash::sha256(sslCertificate.publicKey().toDer());
	QVERIFY(qtPin.has_value());
	QCOMPARE(certificate->spkiSha256().toHex(), qtPin->toHex());
	QCOMPARE(certificate->spkiSha256(), key(algorithm).publicKey().spkiSha256());
	QCOMPARE(certificate->spkiFingerprint(), QStringLiteral("sha256:") + QString::fromLatin1(qtPin->toHex()));
}



// crafts certificates that VeyonCrypto itself never produces (test-only use of OpenSSL)
using SignFunction = std::function<bool(X509*, EVP_PKEY*)>;

static bool signWithSha256(X509* certificate, EVP_PKEY* key)
{
	return X509_sign(certificate, key, EVP_sha256()) > 0;
}



static QByteArray craftCertificate(EVP_PKEY* nativeKey, const std::function<bool(X509*)>& addContent,
								   const SignFunction& sign = signWithSha256)
{
	using namespace Internal;

	const auto name = createName(QStringLiteral("crafted"));
	X509Ptr certificate(X509_new());
	Asn1TimePtr notBefore(ASN1_TIME_adj(nullptr, time(nullptr), 0, -60));
	Asn1TimePtr notAfter(ASN1_TIME_adj(nullptr, time(nullptr), 1, 0));
	Asn1IntegerPtr serial(ASN1_INTEGER_new());
	if (nativeKey == nullptr || name == nullptr || certificate == nullptr || notBefore == nullptr || notAfter == nullptr || serial == nullptr ||
		ASN1_INTEGER_set(serial.get(), 1) != 1 ||
		X509_set_version(certificate.get(), 2) != 1 ||
		X509_set_serialNumber(certificate.get(), serial.get()) != 1 ||
		X509_set1_notBefore(certificate.get(), notBefore.get()) != 1 ||
		X509_set1_notAfter(certificate.get(), notAfter.get()) != 1 ||
		X509_set_subject_name(certificate.get(), name.get()) != 1 ||
		X509_set_issuer_name(certificate.get(), name.get()) != 1 ||
		X509_set_pubkey(certificate.get(), nativeKey) != 1 ||
		addContent(certificate.get()) == false ||
		sign(certificate.get(), nativeKey) == false)
	{
		return {};
	}

	unsigned char* der = nullptr;
	const auto size = i2d_X509(certificate.get(), &der);
	const QByteArray result(reinterpret_cast<const char*>(der), size);
	OPENSSL_free(der);
	return result;
}



static QByteArray craftCertificate(const PrivateKey& key, const std::function<bool(X509*)>& addContent,
								   const SignFunction& sign = signWithSha256)
{
	return craftCertificate(Internal::Access::nativeHandle(key), addContent, sign);
}



void VeyonCryptoPkiTest::compressedPointKeys()
{
	const auto caKey = PrivateKey::fromPkcs8Pem(CompressedP256PrivateKeyPem);
	const auto caCertificate = Certificate::fromPem(CompressedP256CaCertificatePem);
	QVERIFY(caKey.has_value() && caCertificate.has_value());
	QCOMPARE(caKey->algorithm(), KeyAlgorithm::EcP256);

	// one pin definition: the SubjectPublicKeyInfo as contained in the certificate
	QCOMPARE(caCertificate->spkiSha256().toHex(), CompressedP256CaPin);
	QCOMPARE(caCertificate->publicKey().toDer().size(), 59);
	QCOMPARE(caCertificate->publicKey().spkiSha256(), caCertificate->spkiSha256());
	QCOMPARE(caCertificate->publicKey().spkiFingerprint(), caCertificate->spkiFingerprint());
	const auto reparsed = PublicKey::fromDer(caCertificate->publicKey().toDer());
	QVERIFY(reparsed.has_value());
	QCOMPARE(reparsed->spkiSha256(), caCertificate->spkiSha256());

	// semantic key comparison regardless of the point encoding
	QCOMPARE(caCertificate->publicKey(), caKey->publicKey());
	QVERIFY(caCertificate->publicKey() != key(KeyAlgorithm::EcP256).publicKey());
	QVERIFY(caCertificate->isSignedBy(caKey->publicKey()));

	// hence a matching CA key/certificate pair is accepted for issuance
	const auto request = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("host"));
	auto issuanceProfile = profile();
	issuanceProfile.currentTime = caCertificate->notBefore().addDays(1);
	const auto issued = CertificateAuthority::issue(*caCertificate, *caKey, *request, issuanceProfile);
	QVERIFY2(issued.has_value(), qPrintable(lastError()));
	QVERIFY(issued->isSignedBy(caCertificate->publicKey()));
}



// DER manipulation helpers for strictness tests (SEQUENCE with short or 1-2 byte long form length)
static QByteArray encodeLength(qsizetype length)
{
	if (length < 0x80)
	{
		return QByteArray(1, char(length));
	}
	if (length <= 0xff)
	{
		return QByteArray("\x81", 1) + char(length);
	}
	return QByteArray("\x82", 1) + char(length >> 8) + char(length & 0xff);
}



static qsizetype headerSize(const QByteArray& der, qsizetype offset = 0)
{
	const auto first = uchar(der.at(offset + 1));
	return first < 0x80 ? 2 : 2 + (first & 0x7f);
}



// re-encodes the length of the SEQUENCE at offset with one superfluous leading zero byte (BER)
static QByteArray withNonMinimalLength(const QByteArray& der, qsizetype offset = 0)
{
	const auto header = headerSize(der, offset);
	const auto contentSize = uchar(der.at(offset + 1)) < 0x80 ? qsizetype(uchar(der.at(offset + 1))) : header == 3 ? qsizetype(uchar(der.at(offset + 2))) :
																													 (qsizetype(uchar(der.at(offset + 2))) << 8) | uchar(der.at(offset + 3));
	const auto minimalLength = encodeLength(contentSize);
	QByteArray nonMinimal(1, char(0x81 + (minimalLength.size() > 1 ? minimalLength.size() - 1 : 0)));
	nonMinimal += QByteArray(1, 0) + (minimalLength.size() > 1 ? minimalLength.mid(1) : minimalLength);
	return der.left(offset + 1) + nonMinimal + der.mid(offset + header);
}



// same for the first element inside the outer SEQUENCE (tbsCertificate/CertificationRequestInfo)
static QByteArray withNonMinimalInnerLength(const QByteArray& der)
{
	const auto outerHeader = headerSize(der);
	const auto content = withNonMinimalLength(der.mid(outerHeader));
	return QByteArray(1, 0x30) + encodeLength(content.size()) + content;
}



static QByteArray toPemBlock(const QByteArray& der, const char* label)
{
	QByteArray pem = QByteArray("-----BEGIN ") + label + "-----\n";
	const auto base64 = der.toBase64();
	for (int i = 0; i < base64.size(); i += 64)
	{
		pem += base64.mid(i, 64) + "\n";
	}
	return pem + "-----END " + label + "-----\n";
}



// length of the content of the element at offset (short or long form length)
static qsizetype contentSize(const QByteArray& der, qsizetype offset = 0)
{
	const auto first = uchar(der.at(offset + 1));
	if (first < 0x80)
	{
		return first;
	}
	qsizetype size = 0;
	for (int i = 0; i < (first & 0x7f); ++i)
	{
		size = (size << 8) | uchar(der.at(offset + 2 + i));
	}
	return size;
}



static qsizetype elementSize(const QByteArray& der, qsizetype offset = 0)
{
	return headerSize(der, offset) + contentSize(der, offset);
}



static QByteArray derElement(char tag, const QByteArray& content)
{
	return QByteArray(1, tag) + encodeLength(content.size()) + content;
}



// signed DER structure (certificate, CSR): to-be-signed part, signature AlgorithmIdentifier and
// signature value (without the unused-bits octet)
struct SignedDer
{
	QByteArray toBeSigned;
	QByteArray signatureAlgorithm;
	QByteArray signature;

	static SignedDer split(const QByteArray& der)
	{
		const auto content = der.mid(headerSize(der));
		const auto toBeSignedSize = elementSize(content);
		const auto algorithmSize = elementSize(content, toBeSignedSize);
		const auto bitString = content.mid(toBeSignedSize + algorithmSize);
		return {content.left(toBeSignedSize), content.mid(toBeSignedSize, algorithmSize),
				bitString.mid(headerSize(bitString) + 1)};
	}

	// replaces bytes in the to-be-signed part and signs it again with key
	void replaceAndSign(const QByteArray& before, const QByteArray& after, const PrivateKey& key)
	{
		toBeSigned = derElement(0x30, toBeSigned.mid(headerSize(toBeSigned)).replace(before, after));
		signature = key.sign(toBeSigned).value_or(QByteArray{});
	}

	QByteArray toDer() const
	{
		return derElement(0x30, toBeSigned + signatureAlgorithm + derElement(0x03, QByteArray(1, 0) + signature));
	}
};



// RSASSA-PSS signing context with explicit parameters (including the OpenSSL defaults
// SHA-1/MGF1-SHA1/salt 20 that VeyonCrypto must reject)
static Internal::EvpMdContextPtr pssContext(EVP_PKEY* key, const EVP_MD* digest, const EVP_MD* maskDigest, int saltLength)
{
	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	EVP_PKEY_CTX* keyContext = nullptr;
	if (EVP_DigestSignInit(context.get(), &keyContext, digest, nullptr, key) != 1 ||
		EVP_PKEY_CTX_set_rsa_padding(keyContext, RSA_PKCS1_PSS_PADDING) <= 0 ||
		EVP_PKEY_CTX_set_rsa_mgf1_md(keyContext, maskDigest) <= 0 ||
		EVP_PKEY_CTX_set_rsa_pss_saltlen(keyContext, saltLength) <= 0)
	{
		return {};
	}
	return context;
}



void VeyonCryptoPkiTest::rsaPssParameters_data()
{
	QTest::addColumn<QString>("digest");
	QTest::addColumn<QString>("maskDigest");
	QTest::addColumn<int>("saltLength");
	QTest::addColumn<bool>("accepted");

	QTest::newRow("sha256-mgf1sha256-salt32") << QStringLiteral("SHA256") << QStringLiteral("SHA256") << 32 << true;
	// review reproducer: RSASSA-PSS default parameters
	QTest::newRow("sha1-mgf1sha1-salt20") << QStringLiteral("SHA1") << QStringLiteral("SHA1") << 20 << false;
	QTest::newRow("sha256-mgf1sha1-salt32") << QStringLiteral("SHA256") << QStringLiteral("SHA1") << 32 << false;
	QTest::newRow("sha256-mgf1sha256-salt20") << QStringLiteral("SHA256") << QStringLiteral("SHA256") << 20 << false;
	QTest::newRow("sha384-mgf1sha384-salt32") << QStringLiteral("SHA384") << QStringLiteral("SHA384") << 32 << false;
}



void VeyonCryptoPkiTest::rsaPssParameters()
{
	QFETCH(QString, digest);
	QFETCH(QString, maskDigest);
	QFETCH(int, saltLength);
	QFETCH(bool, accepted);

	const auto& rsaKey = key(KeyAlgorithm::Rsa);
	const auto nativeKey = Internal::Access::nativeHandle(rsaKey);
	const auto md = EVP_get_digestbyname(digest.toLatin1().constData());
	const auto maskMd = EVP_get_digestbyname(maskDigest.toLatin1().constData());
	QVERIFY(md && maskMd);

	// certificate request
	const auto subject = Internal::createName(QStringLiteral("pss"));
	Internal::X509RequestPtr request(X509_REQ_new());
	auto context = pssContext(nativeKey, md, maskMd, saltLength);
	QVERIFY(context != nullptr);
	QVERIFY(X509_REQ_set_version(request.get(), 0) == 1 &&
			X509_REQ_set_subject_name(request.get(), subject.get()) == 1 &&
			X509_REQ_set_pubkey(request.get(), nativeKey) == 1 &&
			X509_REQ_sign_ctx(request.get(), context.get()) > 0);
	unsigned char* der = nullptr;
	const auto derSize = i2d_X509_REQ(request.get(), &der);
	const QByteArray requestDer(reinterpret_cast<const char*>(der), derSize);
	OPENSSL_free(der);
	QCOMPARE(CertificateRequest::fromDer(requestDer).has_value(), accepted);

	// certificate signature
	const auto certificate = Certificate::fromDer(craftCertificate(rsaKey, [](X509*) { return true; },
																   [&](X509* x509, EVP_PKEY* signingKey) {
		auto signingContext = pssContext(signingKey, md, maskMd, saltLength);
		return signingContext && X509_sign_ctx(x509, signingContext.get()) > 0;
	}));
	QVERIFY(certificate.has_value());
	QCOMPARE(certificate->isSignedBy(rsaKey.publicKey()), accepted);
}



void VeyonCryptoPkiTest::canonicalPublicKeyEncoding()
{
	// RSA SubjectPublicKeyInfo whose RSAPublicKey (inside the BIT STRING) has a non-minimal length:
	// OpenSSL decodes it, but re-encodes it differently, so the pin of a CSR would differ from the
	// one of the certificate issued for it while the keys compare equal
	const auto& rsaKey = key(KeyAlgorithm::Rsa);
	const auto spki = rsaKey.publicKey().toDer();
	const auto spkiContent = spki.mid(headerSize(spki));
	const auto algorithmIdentifier = spkiContent.left(elementSize(spkiContent));
	const auto bitString = spkiContent.mid(algorithmIdentifier.size());
	const auto rsaPublicKey = bitString.mid(headerSize(bitString) + 1);
	const auto berSpki = derElement(0x30, algorithmIdentifier + derElement(0x03, QByteArray(1, 0) + withNonMinimalLength(rsaPublicKey)));
	QCOMPARE(berSpki.size(), spki.size() + 1);

	auto input = reinterpret_cast<const unsigned char*>(berSpki.constData());
	Internal::EvpPkeyPtr decoded(d2i_PUBKEY(nullptr, &input, berSpki.size()));
	QVERIFY(decoded != nullptr);
	QVERIFY(PublicKey::fromDer(spki).has_value());
	QVERIFY(PublicKey::fromDer(berSpki).has_value() == false);
	QVERIFY(PublicKey::fromPem(toPemBlock(berSpki, "PUBLIC KEY")).has_value() == false);

	// the same key in a (correctly self-signed) CSR and in a certificate
	const auto rsaRequest = CertificateRequest::create(rsaKey, QStringLiteral("host"));
	QVERIFY(rsaRequest.has_value());
	auto berRequest = SignedDer::split(rsaRequest->toDer());
	berRequest.replaceAndSign(spki, berSpki, rsaKey);
	QVERIFY(CertificateRequest::fromDer(berRequest.toDer()).has_value() == false);
	auto unchangedRequest = SignedDer::split(rsaRequest->toDer());
	unchangedRequest.replaceAndSign(spki, spki, rsaKey);
	QVERIFY(CertificateRequest::fromDer(unchangedRequest.toDer()).has_value());

	const auto issued = CertificateAuthority::issue(m_caCertificate, m_caKey, *rsaRequest, profile());
	QVERIFY(issued.has_value());
	QCOMPARE(issued->spkiSha256(), rsaRequest->publicKey().spkiSha256());
	auto berCertificate = SignedDer::split(issued->toDer());
	berCertificate.replaceAndSign(spki, berSpki, m_caKey);
	QVERIFY(Certificate::fromDer(berCertificate.toDer()).has_value() == false);
	auto unchangedCertificate = SignedDer::split(issued->toDer());
	unchangedCertificate.replaceAndSign(spki, spki, m_caKey);
	QVERIFY(Certificate::fromDer(unchangedCertificate.toDer()).has_value());

	// EC keys keep their point form (compressed, hybrid) and the pins stay equal
	const auto compressedKey = PrivateKey::fromPkcs8Pem(CompressedP256PrivateKeyPem);
	QVERIFY(compressedKey.has_value());
	const auto compressedRequest = CertificateRequest::create(*compressedKey, QStringLiteral("host"));
	QVERIFY(compressedRequest.has_value());
	QCOMPARE(compressedRequest->publicKey().toDer().size(), 59);

	const auto& ecKey = key(KeyAlgorithm::EcP256);
	const auto ecSpki = ecKey.publicKey().toDer();
	QCOMPARE(ecSpki.at(ecSpki.size() - 65), char(0x04));
	auto hybridSpki = ecSpki;
	hybridSpki[hybridSpki.size() - 65] = char(0x06 | (hybridSpki.back() & 0x01));
	const auto ecRequest = CertificateRequest::create(ecKey, QStringLiteral("host"));
	QVERIFY(ecRequest.has_value());
	auto hybrid = SignedDer::split(ecRequest->toDer());
	hybrid.replaceAndSign(ecSpki, hybridSpki, ecKey);
	const auto hybridRequest = CertificateRequest::fromDer(hybrid.toDer());
	QVERIFY2(hybridRequest.has_value(), qPrintable(lastError()));

	for (const auto& request : {*compressedRequest, *hybridRequest})
	{
		const auto certificate = CertificateAuthority::issue(m_caCertificate, m_caKey, request, profile());
		QVERIFY2(certificate.has_value(), qPrintable(lastError()));
		QCOMPARE(certificate->spkiSha256(), request.publicKey().spkiSha256());
		QCOMPARE(certificate->publicKey(), request.publicKey());
		QVERIFY(Certificate::fromDer(certificate->toDer()).has_value());
	}
}



void VeyonCryptoPkiTest::canonicalSignatureAlgorithms()
{
	// CSRs: the AlgorithmIdentifier is not covered by the signature, so a variant of it with
	// parameters added or removed would verify with the original signature
	const auto withSignatureAlgorithm = [](const QByteArray& der, const QByteArray& signatureAlgorithm) {
		auto request = SignedDer::split(der);
		request.signatureAlgorithm = signatureAlgorithm;
		return request.toDer();
	};

	const auto ecRequest = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("host"));
	const auto ed25519Request = CertificateRequest::create(key(KeyAlgorithm::Ed25519), QStringLiteral("host"));
	QVERIFY(ecRequest.has_value() && ed25519Request.has_value());
	const auto ecdsaSha256 = hex("300a06082a8648ce3d040302");
	const auto ed25519 = hex("300506032b6570");
	QCOMPARE(SignedDer::split(ecRequest->toDer()).signatureAlgorithm, ecdsaSha256);
	QCOMPARE(SignedDer::split(ed25519Request->toDer()).signatureAlgorithm, ed25519);
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(ecRequest->toDer(), hex("300c06082a8648ce3d0403020500"))).has_value() == false);
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(ed25519Request->toDer(), hex("300706032b65700500"))).has_value() == false);

	// sha256WithRSAEncryption requires NULL parameters
	const auto& rsaKey = key(KeyAlgorithm::Rsa);
	const auto subject = Internal::createName(QStringLiteral("pkcs1"));
	Internal::X509RequestPtr pkcs1Request(X509_REQ_new());
	QVERIFY(X509_REQ_set_version(pkcs1Request.get(), 0) == 1 &&
			X509_REQ_set_subject_name(pkcs1Request.get(), subject.get()) == 1 &&
			X509_REQ_set_pubkey(pkcs1Request.get(), Internal::Access::nativeHandle(rsaKey)) == 1 &&
			X509_REQ_sign(pkcs1Request.get(), Internal::Access::nativeHandle(rsaKey), EVP_sha256()) > 0);
	unsigned char* der = nullptr;
	const auto derSize = i2d_X509_REQ(pkcs1Request.get(), &der);
	const QByteArray pkcs1RequestDer(reinterpret_cast<const char*>(der), derSize);
	OPENSSL_free(der);
	QCOMPARE(SignedDer::split(pkcs1RequestDer).signatureAlgorithm, hex("300d06092a864886f70d01010b0500"));
	QVERIFY(CertificateRequest::fromDer(pkcs1RequestDer).has_value());
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(pkcs1RequestDer, hex("300b06092a864886f70d01010b"))).has_value() == false);

	// RSASSA-PSS parameters in DER only: no explicitly encoded trailer field (DEFAULT 1), no BER
	const auto pssRequest = CertificateRequest::create(rsaKey, QStringLiteral("host"));
	QVERIFY(pssRequest.has_value());
	const auto pssAlgorithm = SignedDer::split(pssRequest->toDer()).signatureAlgorithm;
	const auto pssAlgorithmContent = pssAlgorithm.mid(headerSize(pssAlgorithm));
	const auto pssObject = pssAlgorithmContent.left(elementSize(pssAlgorithmContent));
	const auto pssParameters = pssAlgorithmContent.mid(pssObject.size());
	const auto pssParametersContent = pssParameters.mid(headerSize(pssParameters));
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(pssRequest->toDer(), derElement(0x30, pssObject + pssParameters))).has_value());
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(
											pssRequest->toDer(), derElement(0x30, pssObject + derElement(0x30, pssParametersContent + hex("a303020101")))))
				.has_value() == false);
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(
											pssRequest->toDer(), derElement(0x30, pssObject + withNonMinimalLength(pssParameters))))
				.has_value() == false);
	// RSASSA-PSS without parameters (i.e. the SHA-1 defaults) and with NULL parameters
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(pssRequest->toDer(), derElement(0x30, pssObject))).has_value() == false);
	QVERIFY(CertificateRequest::fromDer(withSignatureAlgorithm(pssRequest->toDer(), derElement(0x30, pssObject + hex("0500")))).has_value() == false);

	// certificates: the AlgorithmIdentifier is signed (inside the tbsCertificate as well), but still
	// only its one encoding is accepted
	auto certificate = SignedDer::split(m_caCertificate.toDer());
	QCOMPARE(certificate.signatureAlgorithm, ecdsaSha256);
	certificate.replaceAndSign(ecdsaSha256, ecdsaSha256, m_caKey);
	QVERIFY(Certificate::fromDer(certificate.toDer())->isSignedBy(m_caKey.publicKey()));
	certificate.replaceAndSign(ecdsaSha256, hex("300c06082a8648ce3d0403020500"), m_caKey);
	certificate.signatureAlgorithm = hex("300c06082a8648ce3d0403020500");
	const auto withNullParameter = Certificate::fromDer(certificate.toDer());
	QVERIFY(withNullParameter.has_value());
	QVERIFY(withNullParameter->isSignedBy(m_caKey.publicKey()) == false);
}



void VeyonCryptoPkiTest::canonicalCriticalFlag()
{
	// OpenSSL re-encodes a critical BOOLEAN TRUE with the value it has read, DER requires 0xff
	const auto basicConstraintsCritical = hex("0603551d130101ff");
	QVERIFY(m_caCertificate.toDer().contains(basicConstraintsCritical));

	auto certificate = SignedDer::split(m_caCertificate.toDer());
	certificate.replaceAndSign(basicConstraintsCritical, basicConstraintsCritical, m_caKey);
	QVERIFY(Certificate::fromDer(certificate.toDer()).has_value());
	certificate.replaceAndSign(basicConstraintsCritical, hex("0603551d13010101"), m_caKey);
	QVERIFY(Certificate::fromDer(certificate.toDer()).has_value() == false);
}



static bool addSubjectAlternativeName(X509* certificate, int type, const QByteArray& value, unsigned long flags)
{
	Internal::GeneralNamesPtr names(GENERAL_NAMES_new());
	auto name = GENERAL_NAME_new();
	auto string = ASN1_IA5STRING_new();
	ASN1_STRING_set(string, value.constData(), value.size());
	GENERAL_NAME_set0_value(name, type, string);
	sk_GENERAL_NAME_push(names.get(), name);
	return X509_add1_ext_i2d(certificate, NID_subject_alt_name, names.get(), 0, flags) == 1;
}



void VeyonCryptoPkiTest::malformedCertificates()
{
	const auto& signingKey = key(KeyAlgorithm::EcP256);

	// sanity check of the crafting helper: plain certificate without extensions
	const auto plain = Certificate::fromDer(craftCertificate(signingKey, [](X509*) { return true; }));
	QVERIFY(plain.has_value());
	QVERIFY(plain->isCa() == false);
	QCOMPARE(plain->pathLengthConstraint(), -1);
	QVERIFY(plain->hasExtendedKeyUsageExtension() == false);
	QVERIFY(plain->hasExtendedKeyUsage(Certificate::ExtendedKeyUsage::ServerAuth) == false);
	QVERIFY(plain->hasKeyUsageExtension() == false);
	QVERIFY(plain->dnsNames().isEmpty());
	QCOMPARE(plain->serialNumberHex(), QStringLiteral("01"));
	QVERIFY(plain->isSignedBy(signingKey.publicKey()));

	const auto valid = Certificate::fromDer(craftCertificate(signingKey, [](X509* certificate) {
		return addSubjectAlternativeName(certificate, GEN_DNS, "host.example", X509V3_ADD_DEFAULT);
	}));
	QVERIFY(valid.has_value());
	QCOMPARE(valid->dnsNames(), QStringList{QStringLiteral("host.example")});

	// duplicate subjectAltName extension
	const auto duplicate = craftCertificate(signingKey, [](X509* certificate) {
		return addSubjectAlternativeName(certificate, GEN_DNS, "a.example", X509V3_ADD_DEFAULT) &&
			addSubjectAlternativeName(certificate, GEN_DNS, "b.example", X509V3_ADD_APPEND);
	});
	QVERIFY(duplicate.isEmpty() == false);
	QVERIFY(Certificate::fromDer(duplicate).has_value() == false);

	// NUL byte and non-ASCII characters in a DNS name
	const auto embeddedNul = craftCertificate(signingKey, [](X509* certificate) {
		return addSubjectAlternativeName(certificate, GEN_DNS, QByteArray("good.example\0.evil", 18), X509V3_ADD_DEFAULT);
	});
	QVERIFY(embeddedNul.isEmpty() == false);
	QVERIFY(Certificate::fromDer(embeddedNul).has_value() == false);
	const auto nonAscii = craftCertificate(signingKey, [](X509* certificate) {
		return addSubjectAlternativeName(certificate, GEN_DNS, QByteArray("h\xc3\xb6st.example"), X509V3_ADD_DEFAULT);
	});
	QVERIFY(Certificate::fromDer(nonAscii).has_value() == false);

	// IP address entry with invalid length
	const auto invalidIp = craftCertificate(signingKey, [](X509* certificate) {
		return addSubjectAlternativeName(certificate, GEN_IPADD, QByteArray(5, 1), X509V3_ADD_DEFAULT);
	});
	QVERIFY(invalidIp.isEmpty() == false);
	QVERIFY(Certificate::fromDer(invalidIp).has_value() == false);

	// serial numbers: positive, non-zero, at most 20 content octets (RFC 5280 §4.1.2.2)
	const auto withSerial = [&](const QByteArray& magnitude, bool negative) {
		return Certificate::fromDer(craftCertificate(signingKey, [&](X509* certificate) {
			Internal::BigNumPtr number(BN_bin2bn(reinterpret_cast<const unsigned char*>(magnitude.constData()),
												 magnitude.size(), nullptr));
			BN_set_negative(number.get(), negative ? 1 : 0);
			Internal::Asn1IntegerPtr serial(BN_to_ASN1_INTEGER(number.get(), nullptr));
			return serial && X509_set_serialNumber(certificate, serial.get()) == 1;
		}));
	};
	QVERIFY(withSerial(QByteArray::fromHex("1234"), true).has_value() == false);
	QVERIFY(withSerial(QByteArray(1, 0), false).has_value() == false);
	QVERIFY(withSerial(QByteArray(20, 0x7f), false).has_value());
	QCOMPARE(withSerial(QByteArray(20, 0x7f), false)->serialNumberHex(), QString::fromLatin1(QByteArray(20, 0x7f).toHex()));
	QVERIFY(withSerial(QByteArray(20, char(0x80)), false).has_value() == false); // 21 content octets
	QVERIFY(withSerial(QByteArray(21, 0x01), false).has_value() == false);

	// keys outside the key policy: the certificate parses (e.g. as part of a foreign chain), but
	// has no usable PublicKey; the SPKI pin is still available
	const auto bio = Internal::readOnlyBio(P384PrivateKeyPem);
	Internal::EvpPkeyPtr p384Key(PEM_read_bio_PrivateKey(bio.get(), nullptr, Internal::noPasswordCallback, nullptr));
	QVERIFY(p384Key != nullptr);
	const auto p384Certificate = Certificate::fromDer(craftCertificate(p384Key.get(), [](X509*) { return true; }));
	QVERIFY(p384Certificate.has_value());
	QVERIFY(p384Certificate->publicKey().isNull());
	QCOMPARE(p384Certificate->spkiSha256().size(), 32);
	QCOMPARE(p384Certificate->toQSslCertificate().isNull(), false);
	QCOMPARE(p384Certificate->spkiSha256(), Hash::sha256(p384Certificate->toQSslCertificate().publicKey().toDer()).value());
	QVERIFY(p384Certificate->isSignedBy(signingKey.publicKey()) == false);
}



void VeyonCryptoPkiTest::strictDer()
{
	// certificates: only the one DER encoding is accepted, via DER and PEM alike
	const auto der = m_caCertificate.toDer();
	QVERIFY(Certificate::fromDer(withNonMinimalLength(der)).has_value() == false);
	QVERIFY(Certificate::fromDer(withNonMinimalInnerLength(der)).has_value() == false);
	QVERIFY(Certificate::chainFromPem(toPemBlock(withNonMinimalInnerLength(der), "CERTIFICATE")).has_value() == false);
	QVERIFY(Certificate::fromPem(toPemBlock(QByteArray(der + QByteArray(2, 0)), "CERTIFICATE")).has_value() == false);
	QVERIFY(Certificate::fromPem(toPemBlock(der, "CERTIFICATE")).has_value());
	QVERIFY(Certificate::fromPem(toPemBlock(der, "TRUSTED CERTIFICATE")).has_value() == false);

	// the per-certificate size limit also applies to PEM input
	const auto oversized = craftCertificate(key(KeyAlgorithm::EcP256), [](X509* certificate) {
		Internal::X509ExtensionPtr extension(X509_EXTENSION_new());
		const std::unique_ptr<ASN1_OBJECT, Internal::OpenSslDeleter<ASN1_OBJECT_free>> object(OBJ_txt2obj("1.3.6.1.4.1.99999.1", 1));
		const std::unique_ptr<ASN1_OCTET_STRING, Internal::OpenSslDeleter<ASN1_OCTET_STRING_free>> value(ASN1_OCTET_STRING_new());
		const QByteArray padding(int(Certificate::MaximumEncodedSize), 'x');
		return extension && object && value &&
			ASN1_OCTET_STRING_set(value.get(), reinterpret_cast<const unsigned char*>(padding.constData()), padding.size()) == 1 &&
			X509_EXTENSION_set_object(extension.get(), object.get()) == 1 &&
			X509_EXTENSION_set_data(extension.get(), value.get()) == 1 &&
			X509_add_ext(certificate, extension.get(), -1) == 1;
	});
	QVERIFY(oversized.size() > Certificate::MaximumEncodedSize);
	QVERIFY(Certificate::fromDer(oversized).has_value() == false);
	QVERIFY(Certificate::chainFromPem(toPemBlock(oversized, "CERTIFICATE")).has_value() == false);

	// certificate requests
	const auto request = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("host"));
	QVERIFY(request.has_value());
	QVERIFY(CertificateRequest::fromDer(withNonMinimalLength(request->toDer())).has_value() == false);
	QVERIFY(CertificateRequest::fromDer(withNonMinimalInnerLength(request->toDer())).has_value() == false);
	QVERIFY(CertificateRequest::fromPem(toPemBlock(QByteArray(request->toDer() + QByteArray(1, 0)), "CERTIFICATE REQUEST")).has_value() == false);
	QVERIFY(CertificateRequest::fromPem(toPemBlock(request->toDer(), "CERTIFICATE REQUEST")).has_value());

	// public keys
	const auto publicDer = key(KeyAlgorithm::EcP256).publicKey().toDer();
	QVERIFY(PublicKey::fromDer(withNonMinimalLength(publicDer)).has_value() == false);
	QVERIFY(PublicKey::fromPem(toPemBlock(QByteArray(publicDer + QByteArray(1, 0)), "PUBLIC KEY")).has_value() == false);
	QVERIFY(PublicKey::fromPem(toPemBlock(publicDer, "PUBLIC KEY")).has_value());
}



void VeyonCryptoPkiTest::pemLabelsAndBlockCount()
{
	// legacy labels (RFC 7468 §5.1, §7)
	const auto certificateDer = m_caCertificate.toDer();
	QVERIFY(Certificate::fromPem(toPemBlock(certificateDer, "X509 CERTIFICATE")).has_value());
	const auto chain = Certificate::chainFromPem(QByteArray(toPemBlock(certificateDer, "CERTIFICATE") + toPemBlock(certificateDer, "X509 CERTIFICATE")));
	QVERIFY(chain.has_value());
	QCOMPARE(chain->size(), 2);

	const auto request = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("host"));
	const auto otherRequest = CertificateRequest::create(key(KeyAlgorithm::Ed25519), QStringLiteral("other"));
	QVERIFY(request.has_value() && otherRequest.has_value());
	const auto requestPem = toPemBlock(request->toDer(), "CERTIFICATE REQUEST");
	const auto legacyRequest = CertificateRequest::fromPem(toPemBlock(request->toDer(), "NEW CERTIFICATE REQUEST"));
	QVERIFY(legacyRequest.has_value());
	QCOMPARE(legacyRequest->toDer(), request->toDer());

	// exactly one block with the label, blocks with other labels are skipped
	QVERIFY(Certificate::fromPem(QByteArray(toPemBlock(certificateDer, "CERTIFICATE") + toPemBlock(certificateDer, "X509 CERTIFICATE"))).has_value() == false);
	QVERIFY(CertificateRequest::fromPem(QByteArray(requestPem + toPemBlock(otherRequest->toDer(), "CERTIFICATE REQUEST"))).has_value() == false);
	QVERIFY(CertificateRequest::fromPem(QByteArray(requestPem + toPemBlock(otherRequest->toDer(), "NEW CERTIFICATE REQUEST"))).has_value() == false);
	QVERIFY(CertificateRequest::fromPem(QByteArray(requestPem + toPemBlock(certificateDer, "CERTIFICATE"))).has_value());

	const auto publicKeyPem = key(KeyAlgorithm::EcP256).publicKey().toPem();
	QVERIFY(PublicKey::fromPem(publicKeyPem).has_value());
	QVERIFY(PublicKey::fromPem(QByteArray(publicKeyPem + key(KeyAlgorithm::Ed25519).publicKey().toPem())).has_value() == false);
	QVERIFY(PublicKey::fromPem(QByteArray(publicKeyPem + requestPem)).has_value());
}



void VeyonCryptoPkiTest::certificateTimeFormat_data()
{
	QTest::addColumn<int>("type");
	QTest::addColumn<QByteArray>("time");
	QTest::addColumn<bool>("accepted");

	QTest::newRow("utc") << V_ASN1_UTCTIME << QByteArray("261116160040Z") << true;
	QTest::newRow("generalized") << V_ASN1_GENERALIZEDTIME << QByteArray("20261116160040Z") << true;
	// review reproducers: accepted by ASN1_TIME_diff(), but not RFC 5280
	QTest::newRow("utc-without-seconds") << V_ASN1_UTCTIME << QByteArray("2611161600Z") << false;
	QTest::newRow("utc-with-offset") << V_ASN1_UTCTIME << QByteArray("261116160040+0100") << false;
	QTest::newRow("generalized-fraction") << V_ASN1_GENERALIZEDTIME << QByteArray("20261116160040.5Z") << false;
	QTest::newRow("generalized-local") << V_ASN1_GENERALIZEDTIME << QByteArray("20261116160040") << false;
	QTest::newRow("utc-non-digit") << V_ASN1_UTCTIME << QByteArray("2611161600x0Z") << false;
	QTest::newRow("utc-invalid-month") << V_ASN1_UTCTIME << QByteArray("261316160040Z") << false;
}



void VeyonCryptoPkiTest::certificateTimeFormat()
{
	QFETCH(int, type);
	QFETCH(QByteArray, time);
	QFETCH(bool, accepted);

	const auto der = craftCertificate(key(KeyAlgorithm::EcP256), [&](X509* certificate) {
		Internal::Asn1TimePtr notBefore(ASN1_STRING_type_new(type));
		return notBefore && ASN1_STRING_set(notBefore.get(), time.constData(), time.size()) == 1 &&
			X509_set1_notBefore(certificate, notBefore.get()) == 1;
	});
	QVERIFY(der.isEmpty() == false);
	QCOMPARE(Certificate::fromDer(der).has_value(), accepted);
}



static Internal::X509NamePtr rawCommonName(const QByteArray& utf8)
{
	Internal::X509NamePtr name(X509_NAME_new());
	if (name == nullptr ||
		X509_NAME_add_entry_by_NID(name.get(), NID_commonName, MBSTRING_UTF8,
								   reinterpret_cast<const unsigned char*>(utf8.constData()), utf8.size(), -1, 0) != 1)
	{
		return {};
	}
	return name;
}



void VeyonCryptoPkiTest::displaySafeNames_data()
{
	QTest::addColumn<QByteArray>("commonName");
	QTest::addColumn<QString>("escaped");
	QTest::addColumn<bool>("isDisplaySafe");

	QTest::newRow("newline") << QByteArray("pc01\ninjected: approved") << QStringLiteral("\\0A") << false; // escaped by OpenSSL
	QTest::newRow("bidi-override") << QByteArray("pc01\xe2\x80\xae"
												 "exe.txt")
								   << QStringLiteral("\\u202e") << false;
	QTest::newRow("line-separator") << QByteArray("pc01\xe2\x80\xa8x") << QStringLiteral("\\u2028") << false;
	// U+E0041 TAG LATIN CAPITAL LETTER A (invisible, outside the Basic Multilingual Plane)
	QTest::newRow("tag-character") << QByteArray("pc01\xf3\xa0\x81\x81x") << QStringLiteral("\\U000e0041") << false;
	// a literal escape sequence must not look like an escaped character
	QTest::newRow("literal-backslash") << QByteArray("pc01\\u202e") << QStringLiteral("pc01\\\\u202e") << true;
}



void VeyonCryptoPkiTest::displaySafeNames()
{
	QFETCH(QByteArray, commonName);
	QFETCH(QString, escaped);
	QFETCH(bool, isDisplaySafe);

	const auto hasUnsafeCharacters = [](const QString& text) {
		const auto codePoints = text.toUcs4();
		return std::any_of(codePoints.cbegin(), codePoints.cend(), [](uint codePoint) {
			const auto category = QChar::category(codePoint);
			return category == QChar::Other_Control || category == QChar::Other_Format ||
				category == QChar::Separator_Line || category == QChar::Separator_Paragraph;
		});
	};

	// certificate request (unauthenticated input of the enrollment server)
	const auto nativeKey = Internal::Access::nativeHandle(key(KeyAlgorithm::EcP256));
	const auto name = rawCommonName(commonName);
	Internal::X509RequestPtr x509Request(X509_REQ_new());
	QVERIFY(name && x509Request);
	QVERIFY(X509_REQ_set_version(x509Request.get(), 0) == 1 &&
			X509_REQ_set_subject_name(x509Request.get(), name.get()) == 1 &&
			X509_REQ_set_pubkey(x509Request.get(), nativeKey) == 1 &&
			X509_REQ_sign(x509Request.get(), nativeKey, EVP_sha256()) > 0);
	unsigned char* der = nullptr;
	const auto derSize = i2d_X509_REQ(x509Request.get(), &der);
	const QByteArray requestDer(reinterpret_cast<const char*>(der), derSize);
	OPENSSL_free(der);

	const auto request = CertificateRequest::fromDer(requestDer);
	QVERIFY(request.has_value());
	QCOMPARE(request->subjectCommonName(), isDisplaySafe ? QString::fromUtf8(commonName) : QString{});
	QVERIFY(request->subject().contains(escaped));
	QVERIFY(hasUnsafeCharacters(request->subject()) == false);

	// certificate
	const auto certificate = Certificate::fromDer(craftCertificate(key(KeyAlgorithm::EcP256), [&](X509* x509) {
		return X509_set_subject_name(x509, name.get()) == 1;
	}));
	QVERIFY(certificate.has_value());
	QCOMPARE(certificate->subjectCommonName(), isDisplaySafe ? QString::fromUtf8(commonName) : QString{});
	QVERIFY(certificate->subject().contains(escaped));
	QVERIFY(hasUnsafeCharacters(certificate->subject()) == false);

	// too long common names are not reported either
	const auto longName = rawCommonName(QByteArray(65, 'a'));
	QVERIFY(longName == nullptr ||
			Certificate::fromDer(craftCertificate(key(KeyAlgorithm::EcP256), [&](X509* x509) {
		return X509_set_subject_name(x509, longName.get()) == 1;
	}))->subjectCommonName()
				.isEmpty());
}



static bool addConfiguredExtension(X509* certificate, int nid, const char* value)
{
	X509V3_CTX context;
	X509V3_set_ctx_nodb(&context);
	X509V3_set_ctx(&context, certificate, certificate, nullptr, nullptr, 0);
	Internal::X509ExtensionPtr extension(X509V3_EXT_conf_nid(nullptr, &context, nid, value));
	return extension && X509_add_ext(certificate, extension.get(), -1) == 1;
}



void VeyonCryptoPkiTest::criticalExtensionsAndSanTypes()
{
	const auto& signingKey = key(KeyAlgorithm::EcP256);

	// own certificates only use modelled critical extensions
	QCOMPARE(m_caCertificate.criticalExtensions(), QStringList({QStringLiteral("2.5.29.19"), QStringLiteral("2.5.29.15")}));
	QVERIFY(m_caCertificate.hasUnsupportedCriticalExtension() == false);
	QVERIFY(Certificate().hasUnsupportedCriticalExtension());

	const auto unknownCritical = Certificate::fromDer(craftCertificate(signingKey, [](X509* certificate) {
		Internal::X509ExtensionPtr extension(X509_EXTENSION_new());
		const std::unique_ptr<ASN1_OBJECT, Internal::OpenSslDeleter<ASN1_OBJECT_free>> object(OBJ_txt2obj("1.3.6.1.4.1.99999.2", 1));
		const std::unique_ptr<ASN1_OCTET_STRING, Internal::OpenSslDeleter<ASN1_OCTET_STRING_free>> value(ASN1_OCTET_STRING_new());
		return extension && object && value &&
			ASN1_OCTET_STRING_set(value.get(), reinterpret_cast<const unsigned char*>("\x05\x00"), 2) == 1 &&
			X509_EXTENSION_set_object(extension.get(), object.get()) == 1 &&
			X509_EXTENSION_set_critical(extension.get(), 1) == 1 &&
			X509_EXTENSION_set_data(extension.get(), value.get()) == 1 &&
			X509_add_ext(certificate, extension.get(), -1) == 1;
	}));
	QVERIFY(unknownCritical.has_value());
	QVERIFY(unknownCritical->criticalExtensions().contains(QStringLiteral("1.3.6.1.4.1.99999.2")));
	QVERIFY(unknownCritical->hasUnsupportedCriticalExtension());

	const auto nameConstraints = Certificate::fromDer(craftCertificate(signingKey, [](X509* certificate) {
		return addConfiguredExtension(certificate, NID_basic_constraints, "critical,CA:TRUE") &&
			addConfiguredExtension(certificate, NID_name_constraints, "critical,permitted;DNS:example");
	}));
	QVERIFY(nameConstraints.has_value());
	QVERIFY(nameConstraints->criticalExtensions().contains(QStringLiteral("2.5.29.30")));
	QVERIFY(nameConstraints->hasUnsupportedCriticalExtension());

	// SAN types other than DNS/IP are ignored without being interpreted as strings
	const auto mixedNames = Certificate::fromDer(craftCertificate(signingKey, [](X509* certificate) {
		return addConfiguredExtension(certificate, NID_subject_alt_name,
									  "email:a@example,URI:https://example/,RID:1.2.3.4,otherName:1.2.3.4;UTF8:x,"
									  "DNS:host.example,IP:192.0.2.1");
	}));
	QVERIFY(mixedNames.has_value());
	QCOMPARE(mixedNames->dnsNames(), QStringList{QStringLiteral("host.example")});
	QCOMPARE(mixedNames->ipAddresses(), QList<QHostAddress>{QHostAddress(QStringLiteral("192.0.2.1"))});

	// DNS names longer than 253 characters
	const QByteArray longName = QByteArray(250, 'a') + QByteArrayLiteral(".exa");
	QVERIFY(Certificate::fromDer(craftCertificate(signingKey, [&](X509* certificate) {
		return addSubjectAlternativeName(certificate, GEN_DNS, longName, X509V3_ADD_DEFAULT);
	})).has_value() == false);
}



void VeyonCryptoPkiTest::usageExtensions()
{
	const auto& signingKey = key(KeyAlgorithm::EcP256);

	// usages other than serverAuth/clientAuth are ignored
	const auto clientCertificate = Certificate::fromDer(craftCertificate(signingKey, [](X509* certificate) {
		return addConfiguredExtension(certificate, NID_ext_key_usage, "clientAuth,codeSigning");
	}));
	QVERIFY(clientCertificate.has_value());
	QVERIFY(clientCertificate->hasExtendedKeyUsageExtension());
	QVERIFY(clientCertificate->hasExtendedKeyUsage(Certificate::ExtendedKeyUsage::ClientAuth));
	QVERIFY(clientCertificate->hasExtendedKeyUsage(Certificate::ExtendedKeyUsage::ServerAuth) == false);

	const auto pathLength = [&](const char* basicConstraints) {
		return Certificate::fromDer(craftCertificate(signingKey, [&](X509* certificate) {
			return addConfiguredExtension(certificate, NID_basic_constraints, basicConstraints);
		}));
	};
	const auto maximumPathLength = pathLength("critical,CA:TRUE,pathlen:255");
	QVERIFY(maximumPathLength.has_value());
	QCOMPARE(maximumPathLength->pathLengthConstraint(), 255);
	QVERIFY(pathLength("critical,CA:TRUE,pathlen:256").has_value() == false);
}



void VeyonCryptoPkiTest::nullAndCopiedObjects()
{
	const Certificate nullCertificate;
	QVERIFY(nullCertificate.isNull());
	QVERIFY(nullCertificate.toDer().isEmpty());
	QVERIFY(nullCertificate.toPem().isEmpty());
	QVERIFY(nullCertificate.serialNumberHex().isEmpty());
	QVERIFY(m_caCertificate.isNull() == false);

	const Certificate copiedCertificate(m_caCertificate);
	QCOMPARE(copiedCertificate.toDer(), m_caCertificate.toDer());
	Certificate movedCertificate;
	movedCertificate = Certificate(m_caCertificate);
	QCOMPARE(movedCertificate.toDer(), m_caCertificate.toDer());

	const CertificateRequest nullRequest;
	QVERIFY(nullRequest.toDer().isEmpty());
	QVERIFY(nullRequest.toPem().isEmpty());
	const auto request = CertificateRequest::create(key(KeyAlgorithm::EcP256), QStringLiteral("copied"));
	QVERIFY(request.has_value());
	CertificateRequest copiedRequest;
	copiedRequest = *request;
	QCOMPARE(copiedRequest.toDer(), request->toDer());
	CertificateRequest movedRequest;
	movedRequest = std::move(copiedRequest);
	QCOMPARE(movedRequest.toDer(), request->toDer());

	const PrivateKey nullKey;
	QVERIFY(nullKey.isNull());
	QVERIFY(key(KeyAlgorithm::EcP256).isNull() == false);
	QCOMPARE(nullKey.bits(), 0);
	QVERIFY(nullKey.toPkcs8Der().has_value() == false);
	QVERIFY(nullKey.toPkcs8Pem().has_value() == false);
	QVERIFY(nullKey.publicKey().isNull());

	const PublicKey nullPublicKey;
	QCOMPARE(nullPublicKey.bits(), 0);
	QVERIFY(nullPublicKey.toPem().isEmpty());
	QVERIFY(nullPublicKey.toEd25519RawKey().isEmpty());
	QVERIFY(key(KeyAlgorithm::EcP256).publicKey().toEd25519RawKey().isEmpty());
	const auto ed25519PublicKey = key(KeyAlgorithm::Ed25519).publicKey();
	PublicKey copiedPublicKey;
	copiedPublicKey = ed25519PublicKey;
	QCOMPARE(copiedPublicKey.toEd25519RawKey().size(), qsizetype(PublicKey::Ed25519KeySize));
}



void VeyonCryptoPkiTest::invalidInputSizes()
{
	const QByteArray oversized(64 * 1024 + 1, 'A');

	QVERIFY(CertificateRequest::fromPem({}).has_value() == false);
	QVERIFY(CertificateRequest::fromPem(oversized).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("invalid size")), qPrintable(lastError()));
	QVERIFY(PrivateKey::fromPkcs8Pem({}).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Pem(oversized).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("invalid size")), qPrintable(lastError()));
	QVERIFY(PublicKey::fromDer({}).has_value() == false);
	QVERIFY(PublicKey::fromDer(oversized).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("invalid size")), qPrintable(lastError()));
	QVERIFY(PublicKey::fromPem({}).has_value() == false);
	QVERIFY(PublicKey::fromPem(oversized).has_value() == false);
	QVERIFY2(lastError().contains(QLatin1String("invalid size")), qPrintable(lastError()));

	// names longer than 64 characters
	CertificateAuthority::CaParameters parameters;
	parameters.commonName = QString(65, u'c');
	QVERIFY(CertificateAuthority::createCaCertificate(m_caKey, parameters).has_value() == false);
	QVERIFY(lastError().contains(QLatin1String("invalid name attribute")));
}



void VeyonCryptoPkiTest::unsupportedKeyTypes()
{
	// X25519 (key agreement only) is no signature key type
	Internal::EvpPkeyPtr x25519Key;
	{
		const Internal::EvpPkeyContextPtr context(EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr));
		EVP_PKEY* generatedKey = nullptr;
		QVERIFY(context && EVP_PKEY_keygen_init(context.get()) == 1 && EVP_PKEY_keygen(context.get(), &generatedKey) == 1);
		x25519Key.reset(generatedKey);
	}
	const auto subjectPublicKeyInfo = Internal::subjectPublicKeyInfo(x25519Key.get());
	QVERIFY(subjectPublicKeyInfo.isEmpty() == false);
	QCOMPARE(Internal::classifyKey(x25519Key.get()), KeyAlgorithm::Invalid);
	QCOMPARE(Internal::classifyKey(nullptr), KeyAlgorithm::Invalid);
	QVERIFY(PublicKey::fromDer(subjectPublicKeyInfo).has_value() == false);
	QVERIFY(lastError().contains(QLatin1String("unsupported key type")));

	// well-formed structures with an algorithm unknown to OpenSSL (OID 1.3.6.1.4.1.99999.1)
	const auto unknownAlgorithm = derElement(0x30, hex("06092b06010401868d1f01"));
	QVERIFY(PublicKey::fromDer(derElement(0x30, unknownAlgorithm + derElement(0x03, QByteArray(33, 0x00)))).has_value() == false);
	QVERIFY(PrivateKey::fromPkcs8Der(derElement(0x30, hex("020100") + unknownAlgorithm + derElement(0x04, derElement(0x04, QByteArray(32, 0x01)))))
				.has_value() == false);
}


QTEST_GUILESS_MAIN(VeyonCryptoPkiTest)
#include "VeyonCryptoPkiTest.moc"
