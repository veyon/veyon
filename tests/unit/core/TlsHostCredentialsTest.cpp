// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QTemporaryDir>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "PlatformSecretFunctions.h"
#include "SecretStore.h"
#include "TlsTestHostCredentials.h"
#include "VeyonConfiguration.h"

#include <map>

using Veyon::Crypto::KeyAlgorithm;
using Veyon::Crypto::SecureBuffer;

enum class HostKey {
	EcP256,
	OtherEcP256,
	None,
	Garbage,
	Ed25519,
	Rsa,
	EcP256Pem, // PKCS#8 PEM instead of DER
};

enum class HostCertificate {
	SelfSigned,
	None,
	Whitespace,
	Garbage,
	Der, // instead of PEM
	Expired,
	NotYetValid,				// beyond the tolerance
	NotYetValidWithinTolerance, // notBefore 4 min ahead
	ExpiresSoon,
	CaSignedChain, // leaf, CA
	CaFirst,
	Rsa,
};

Q_DECLARE_METATYPE(TlsHostCredentials::Error)
Q_DECLARE_METATYPE(HostKey)
Q_DECLARE_METATYPE(HostCertificate)

// in-memory SecretStore backend
class HostKeySecretFunctions : public PlatformSecretFunctions
{
public:
	std::optional<SecureBuffer> readSecret(const QString& key) override
	{
		const auto it = m_secrets.find(key);
		if (it == m_secrets.end())
		{
			return {};
		}
		return it->second.clone();
	}

	bool writeSecret(const QString& key, const SecureBuffer& value) override
	{
		m_secrets[key] = value.clone();
		return true;
	}

	bool removeSecret(const QString& key) override
	{
		m_secrets.erase(key);
		return true;
	}

	QStringList secretKeys() override
	{
		QStringList keys;
		for (const auto& entry : m_secrets)
		{
			keys.append(entry.first);
		}
		return keys;
	}

	bool isAvailable() override
	{
		return true;
	}

	bool prepareStorage() override
	{
		return true;
	}

private:
	std::map<QString, SecureBuffer> m_secrets;
};


// Host key and certificate of the TLS server (05-tls.md §1.3, §2, 06-pki.md §3) and the server
// policy from the configuration
class TlsHostCredentialsTest : public QObject
{
	Q_OBJECT
private:
	QDateTime m_now;
	Veyon::Crypto::PrivateKey m_key;
	Veyon::Crypto::PrivateKey m_otherKey;
	Veyon::Crypto::PrivateKey m_rsaKey;
	Veyon::Crypto::PrivateKey m_caKey;
	Veyon::Crypto::Certificate m_certificate;
	Veyon::Crypto::Certificate m_caCertificate;
	HostKeySecretFunctions m_secrets;
	QTemporaryDir m_directory;
	QStringList m_logMessages;

	static QStringList* s_logMessages;

	static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_logMessages->append(message);
	}

	QString writeFile(const QString& name, const QByteArray& data) const
	{
		const auto path = m_directory.filePath(name);
		QFile file(path);
		if (file.open(QFile::WriteOnly | QFile::Truncate))
		{
			file.write(data);
		}
		return path;
	}

	static SecureBuffer der(const Veyon::Crypto::PrivateKey& key)
	{
		return key.toPkcs8Der().value_or(SecureBuffer{});
	}

	void verifyNoKeyMaterialLogged(const Veyon::Crypto::PrivateKey& key) const
	{
		const auto keyDer = der(key).toUnprotectedByteArray();
		const auto keyPem = key.toPkcs8Pem()->toUnprotectedByteArray();
		for (const auto& message : m_logMessages)
		{
			QVERIFY(message.contains(QLatin1String("PRIVATE KEY")) == false);
			QVERIFY(message.contains(QString::fromLatin1(keyDer.toBase64())) == false);
			QVERIFY(message.contains(QString::fromLatin1(keyDer.toHex())) == false);
			QVERIFY(message.contains(QString::fromLatin1(keyPem.mid(28, 40))) == false);
		}
	}

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_directory.isValid());
		m_now = QDateTime::currentDateTimeUtc();
		m_key = TlsTestHostCredentials::generateKey();
		m_otherKey = TlsTestHostCredentials::generateKey();
		m_rsaKey = TlsTestHostCredentials::generateKey(KeyAlgorithm::Rsa);
		m_caKey = TlsTestHostCredentials::generateKey();
		QVERIFY(m_key.isNull() == false && m_otherKey.isNull() == false && m_rsaKey.isNull() == false);
		m_certificate = TlsTestHostCredentials::createCertificate(m_key, m_now);
		m_caCertificate = PkiTestCertificates::createCa(m_caKey, QStringLiteral("Veyon CA"), m_now);
		QVERIFY(m_certificate.isNull() == false && m_caCertificate.isNull() == false);
	}

	void init()
	{
		m_logMessages.clear();
		s_logMessages = &m_logMessages;
		qInstallMessageHandler(captureMessage);
		SecretStore::setTestBackend(&m_secrets, VeyonCore::InitializationFlag::SecretStore);
		(void)m_secrets.removeSecret(TlsHostCredentials::PrivateKeySecret);
	}

	void cleanup()
	{
		SecretStore::setTestBackend(nullptr, VeyonCore::InitializationFlags());
		qInstallMessageHandler(nullptr);
	}

	void check_data()
	{
		QTest::addColumn<HostKey>("hostKey");
		QTest::addColumn<HostCertificate>("hostCertificate");
		QTest::addColumn<TlsHostCredentials::Error>("expected");

		using Error = TlsHostCredentials::Error;
		QTest::newRow("valid") << HostKey::EcP256 << HostCertificate::SelfSigned << Error::None;
		QTest::newRow("key mismatch") << HostKey::OtherEcP256 << HostCertificate::SelfSigned << Error::KeyMismatch;
		QTest::newRow("no key") << HostKey::None << HostCertificate::SelfSigned << Error::MissingKey;
		QTest::newRow("garbage key") << HostKey::Garbage << HostCertificate::SelfSigned << Error::InvalidKey;
		QTest::newRow("Ed25519 key") << HostKey::Ed25519 << HostCertificate::SelfSigned << Error::UnsupportedKeyAlgorithm;
		QTest::newRow("PEM key") << HostKey::EcP256Pem << HostCertificate::SelfSigned << Error::InvalidKey;
		QTest::newRow("no certificate") << HostKey::EcP256 << HostCertificate::None << Error::MissingCertificate;
		QTest::newRow("whitespace certificate") << HostKey::EcP256 << HostCertificate::Whitespace << Error::MissingCertificate;
		QTest::newRow("garbage certificate") << HostKey::EcP256 << HostCertificate::Garbage << Error::InvalidCertificate;
		QTest::newRow("DER certificate") << HostKey::EcP256 << HostCertificate::Der << Error::InvalidCertificate;
		QTest::newRow("expired") << HostKey::EcP256 << HostCertificate::Expired << Error::Expired;
		QTest::newRow("not yet valid") << HostKey::EcP256 << HostCertificate::NotYetValid << Error::NotYetValid;
		QTest::newRow("not yet valid within tolerance") << HostKey::EcP256 << HostCertificate::NotYetValidWithinTolerance << Error::None;
		QTest::newRow("expires in a minute") << HostKey::EcP256 << HostCertificate::ExpiresSoon << Error::None;
		QTest::newRow("chain") << HostKey::EcP256 << HostCertificate::CaSignedChain << Error::None;
		QTest::newRow("CA first in chain") << HostKey::EcP256 << HostCertificate::CaFirst << Error::KeyMismatch;
		QTest::newRow("RSA") << HostKey::Rsa << HostCertificate::Rsa << Error::None;
		QTest::newRow("RSA key, EC certificate") << HostKey::Rsa << HostCertificate::SelfSigned << Error::KeyMismatch;
		QTest::newRow("EC key, RSA certificate") << HostKey::EcP256 << HostCertificate::Rsa << Error::KeyMismatch;
	}

	void check()
	{
		QFETCH(HostKey, hostKey);
		QFETCH(HostCertificate, hostCertificate);
		QFETCH(TlsHostCredentials::Error, expected);

		SecureBuffer key;
		switch (hostKey)
		{
		case HostKey::EcP256: key = der(m_key); break;
		case HostKey::OtherEcP256: key = der(m_otherKey); break;
		case HostKey::None: break;
		case HostKey::Garbage: key = SecureBuffer::fromByteArray(QByteArrayLiteral("not a key")); break;
		case HostKey::Ed25519: key = der(TlsTestHostCredentials::generateKey(KeyAlgorithm::Ed25519)); break;
		case HostKey::Rsa: key = der(m_rsaKey); break;
		case HostKey::EcP256Pem: key = std::move(*m_key.toPkcs8Pem()); break;
		}

		using TlsTestHostCredentials::createCertificate;
		const auto caSignedLeaf = createCertificate(m_key, m_now, -60, 86400, &m_caKey, m_caCertificate.subjectCommonName());
		QByteArray chain;
		switch (hostCertificate)
		{
		case HostCertificate::SelfSigned: chain = m_certificate.toPem(); break;
		case HostCertificate::None: break;
		case HostCertificate::Whitespace: chain = QByteArrayLiteral(" \n\t\n"); break;
		case HostCertificate::Garbage: chain = QByteArrayLiteral("-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n"); break;
		case HostCertificate::Der: chain = m_certificate.toDer(); break;
		case HostCertificate::Expired: chain = createCertificate(m_key, m_now, -10 * 86400, -60).toPem(); break;
		case HostCertificate::NotYetValid: chain = createCertificate(m_key, m_now, 10 * 60, 86400).toPem(); break;
		case HostCertificate::NotYetValidWithinTolerance: chain = createCertificate(m_key, m_now, 4 * 60, 86400).toPem(); break;
		case HostCertificate::ExpiresSoon: chain = createCertificate(m_key, m_now, -86400, 60).toPem(); break;
		case HostCertificate::CaSignedChain: chain = caSignedLeaf.toPem() + m_caCertificate.toPem(); break;
		case HostCertificate::CaFirst: chain = m_caCertificate.toPem() + caSignedLeaf.toPem(); break;
		case HostCertificate::Rsa: chain = createCertificate(m_rsaKey, m_now).toPem(); break;
		}

		QCOMPARE(TlsHostCredentials::check(key, chain, m_now), expected);

		// certificates outside their validity period are still used, the verifying peer decides
		const auto usable = expected == TlsHostCredentials::Error::None ||
			expected == TlsHostCredentials::Error::NotYetValid || expected == TlsHostCredentials::Error::Expired;
		QCOMPARE(TlsHostCredentials::isUsable(expected), usable);

		if (VeyonCore::isTlsAvailable())
		{
			const auto credentials = TlsHostCredentials::fromEncoded({std::move(key), chain}, m_now);
			QCOMPARE(credentials.has_value(), usable);
			if (expected != TlsHostCredentials::Error::None)
			{
				QCOMPARE(m_logMessages.size(), 1);
				QVERIFY(m_logMessages.first().contains(TlsHostCredentials::errorString(expected)));
				QCOMPARE(m_logMessages.first().contains(QLatin1String("still offered")), usable);
			}
		}

		verifyNoKeyMaterialLogged(m_key);
		verifyNoKeyMaterialLogged(m_rsaKey);
	}

	void errorStrings()
	{
		using Error = TlsHostCredentials::Error;
		QVERIFY(TlsHostCredentials::errorString(Error::None).isEmpty());
		QStringList strings;
		for (const auto error : {Error::MissingKey, Error::InvalidKey, Error::UnsupportedKeyAlgorithm,
								 Error::MissingCertificate, Error::InvalidCertificate, Error::KeyMismatch,
								 Error::NotYetValid, Error::Expired})
		{
			const auto string = TlsHostCredentials::errorString(error);
			QVERIFY(string.isEmpty() == false);
			QVERIFY(strings.contains(string) == false);
			strings.append(string);
		}
	}

	// the QSslKey algorithm comes from the key, never a hard-coded QSsl::Rsa (05-tls.md §1.3)
	void qsslCredentials_data()
	{
		QTest::addColumn<bool>("rsa");

		QTest::newRow("EC P-256") << false;
		QTest::newRow("RSA") << true;
	}

	void qsslCredentials()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		QFETCH(bool, rsa);

		const auto expectedAlgorithm = rsa ? QSsl::Rsa : QSsl::Ec;
		const auto& key = rsa ? m_rsaKey : m_key;
		const auto leaf = TlsTestHostCredentials::createCertificate(key, m_now, -60, 86400, &m_caKey,
																	m_caCertificate.subjectCommonName());

		const auto credentials = TlsHostCredentials::fromEncoded(TlsTestHostCredentials::encode(key, {leaf, m_caCertificate}), m_now);
		QVERIFY(credentials.has_value());
		QCOMPARE(credentials->privateKey().algorithm(), expectedAlgorithm);
		QCOMPARE(credentials->privateKey().type(), QSsl::PrivateKey);
		QCOMPARE(credentials->certificateChain().size(), 2);
		QCOMPARE(credentials->certificateChain().at(0).toDer(), leaf.toDer());
		QCOMPARE(credentials->certificateChain().at(1).toDer(), m_caCertificate.toDer());
		QCOMPARE(credentials->privateKey().length(), key.bits());

		const auto policy = TlsTestHostCredentials::policy(VeyonCore::TlsServerMode::Optional, &*credentials);
		QVERIFY(policy.hasCredentials());
	}

	void serviceResponse()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		const auto encoded = TlsTestHostCredentials::encode(m_key, {m_certificate});
		const auto keyBase64 = QString::fromLatin1(encoded.privateKeyDer.toUnprotectedByteArray().toBase64());
		const auto chain = QString::fromLatin1(encoded.certificateChainPem);

		const auto valid = TlsHostCredentials::fromServiceResponse(
			IpcMessage::success(1, {{IpcProtocol::Key::TlsKey, keyBase64}, {IpcProtocol::Key::CertificateChain, chain}}), m_now);
		QVERIFY(valid.has_value());
		QCOMPARE(valid->certificateChain().first().toDer(), m_certificate.toDer());

		// refused by the service (TLS disabled, no certificate, too many requests)
		QVERIFY(TlsHostCredentials::fromServiceResponse(IpcMessage::failure(1, IpcProtocol::Error::DeniedError), m_now).has_value() == false);
		QVERIFY(m_logMessages.last().contains(QLatin1String("denied")));

		// malformed responses
		const QList<QJsonObject> malformedResults{
			{},
			{{IpcProtocol::Key::TlsKey, keyBase64}},
			{{IpcProtocol::Key::CertificateChain, chain}},
			{{IpcProtocol::Key::TlsKey, 42}, {IpcProtocol::Key::CertificateChain, chain}},
			{{IpcProtocol::Key::TlsKey, keyBase64}, {IpcProtocol::Key::CertificateChain, QJsonArray{chain}}},
			{{IpcProtocol::Key::TlsKey, QStringLiteral("*not base64*")}, {IpcProtocol::Key::CertificateChain, chain}},
			{{IpcProtocol::Key::TlsKey, keyBase64.left(keyBase64.size() - 3)}, {IpcProtocol::Key::CertificateChain, chain}},
		};
		for (const auto& result : malformedResults)
		{
			m_logMessages.clear();
			QVERIFY(TlsHostCredentials::fromServiceResponse(IpcMessage::success(1, result), m_now).has_value() == false);
			QCOMPARE(m_logMessages.size(), 1);
		}

		// key of another host
		const auto otherKeyBase64 = QString::fromLatin1(der(m_otherKey).toUnprotectedByteArray().toBase64());
		QVERIFY(TlsHostCredentials::fromServiceResponse(
					IpcMessage::success(1, {{IpcProtocol::Key::TlsKey, otherKeyBase64}, {IpcProtocol::Key::CertificateChain, chain}}), m_now)
					.has_value() == false);
		QVERIFY(m_logMessages.last().contains(TlsHostCredentials::errorString(TlsHostCredentials::Error::KeyMismatch)));

		// withheld by the service (T-PROC-02)
		QVERIFY(TlsHostCredentials::fromServiceResponse(IpcMessage::failure(1, IpcProtocol::Error::HostKeyWithheldError), m_now)
					.has_value() == false);

		verifyNoKeyMaterialLogged(m_key);
		verifyNoKeyMaterialLogged(m_otherKey);
	}

	void load()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, m_directory.filePath(QStringLiteral("config.json")));
		VeyonConfiguration configuration(&store);

		const auto certificatePath = writeFile(QStringLiteral("host-cert.pem"), m_certificate.toPem());
		configuration.setTlsHostCertificateFile(certificatePath);
		QCOMPARE(TlsHostCredentials::certificateFilePath(configuration), certificatePath);

		// no key in the store
		QVERIFY(TlsHostCredentials::load(configuration, m_now).has_value() == false);
		QVERIFY(m_logMessages.last().contains(QLatin1String("no host key")));

		QVERIFY(SecretStore::write(TlsHostCredentials::PrivateKeySecret, der(m_key)));

		// valid, no expiry warning (730 days)
		m_logMessages.clear();
		auto loaded = TlsHostCredentials::load(configuration, m_now);
		QVERIFY(loaded.has_value());
		QCOMPARE(loaded->certificateChainPem, m_certificate.toPem());
		QCOMPARE(loaded->privateKeyDer.toUnprotectedByteArray(), der(m_key).toUnprotectedByteArray());
		QVERIFY(m_logMessages.isEmpty());

		// expiry warning 60 days ahead
		QVERIFY(TlsHostCredentials::load(configuration, m_now.addDays(730 - 61)).has_value());
		QVERIFY(m_logMessages.isEmpty());
		QVERIFY(TlsHostCredentials::load(configuration, m_now.addDays(730 - 59)).has_value());
		QCOMPARE(m_logMessages.size(), 1);
		QVERIFY(m_logMessages.first().contains(QLatin1String("expires on")));

		// expired or not yet valid: still handed out, with a complaint
		m_logMessages.clear();
		QVERIFY(TlsHostCredentials::load(configuration, m_now.addDays(731)).has_value());
		QCOMPARE(m_logMessages.size(), 1);
		QVERIFY(m_logMessages.first().contains(QLatin1String("expired on")));
		QVERIFY(m_logMessages.first().contains(QLatin1String("still offered")));
		m_logMessages.clear();
		QVERIFY(TlsHostCredentials::load(configuration, m_now.addDays(-1)).has_value());
		QCOMPARE(m_logMessages.size(), 1);
		QVERIFY(m_logMessages.first().contains(QLatin1String("not valid before")));

		// certificate of another key in the file (e.g. key renewed, certificate not yet installed)
		configuration.setTlsHostCertificateFile(writeFile(QStringLiteral("other.pem"),
														  TlsTestHostCredentials::createCertificate(m_otherKey, m_now).toPem()));
		QVERIFY(TlsHostCredentials::load(configuration, m_now).has_value() == false);
		QVERIFY(m_logMessages.last().contains(TlsHostCredentials::errorString(TlsHostCredentials::Error::KeyMismatch)));

		// missing and oversized certificate file
		configuration.setTlsHostCertificateFile(m_directory.filePath(QStringLiteral("missing.pem")));
		QVERIFY(TlsHostCredentials::load(configuration, m_now).has_value() == false);
		QVERIFY(m_logMessages.last().contains(QLatin1String("no host certificate")));

		configuration.setTlsHostCertificateFile(writeFile(QStringLiteral("huge.pem"),
														  m_certificate.toPem() + QByteArray(Veyon::Crypto::Certificate::MaximumPemSize, ' ')));
		QVERIFY(TlsHostCredentials::load(configuration, m_now).has_value() == false);
		QVERIFY(m_logMessages.last().contains(QLatin1String("invalid host certificate")));

		// SecretStore not available (e.g. veyon-server): nothing
		configuration.setTlsHostCertificateFile(certificatePath);
		SecretStore::setTestBackend(&m_secrets, VeyonCore::InitializationFlags());
		QVERIFY(TlsHostCredentials::load(configuration, m_now).has_value() == false);

		verifyNoKeyMaterialLogged(m_key);
	}

	void defaultCertificateFilePath()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, m_directory.filePath(QStringLiteral("default.json")));
		VeyonConfiguration configuration(&store);

		QVERIFY(configuration.tlsHostCertificateFile().isEmpty());
		QCOMPARE(configuration.tlsHostCertificateFileProperty().absoluteKey(), QStringLiteral("TLS/HostCertificateFile"));
#if !defined(Q_OS_WIN)
		QCOMPARE(TlsHostCredentials::certificateFilePath(configuration), QStringLiteral("/var/lib/veyon/tls/host-cert.pem"));
#endif
	}

	void serverPolicyFromConfiguration()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, m_directory.filePath(QStringLiteral("policy.json")));
		VeyonConfiguration configuration(&store);

		// defaults (03-secretstore-config.md §5)
		QCOMPARE(configuration.tlsSniffTimeoutMs(), 200);
		QCOMPARE(configuration.tlsSniffTimeoutMsProperty().absoluteKey(), QStringLiteral("TLS/SniffTimeoutMs"));
		QCOMPARE(configuration.tlsSniffTimeoutMsProperty().flags(), Configuration::Property::Flags(Configuration::Property::Flag::Hidden));
		QCOMPARE(configuration.tlsHostCertificateFileProperty().flags(), Configuration::Property::Flags(Configuration::Property::Flag::Advanced));

		auto policy = TlsServerPolicy::fromConfiguration(configuration);
		QCOMPARE(policy.mode, VeyonCore::TlsServerMode::Optional);
		QCOMPARE(policy.sniffTimeoutMs, 200);
		QCOMPARE(policy.handshakeTimeoutMs, 10000);
		QVERIFY(policy.hasCredentials() == false);

		configuration.setTlsServerMode(VeyonCore::TlsServerMode::Disabled);
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).mode, VeyonCore::TlsServerMode::Disabled);
		configuration.setTlsServerMode(VeyonCore::TlsServerMode::Required);
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).mode, VeyonCore::TlsServerMode::Required);

		// invalid values fail closed
		configuration.setValue(QStringLiteral("ServerMode"), QStringLiteral("Optionall"), QStringLiteral("TLS"));
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).mode, VeyonCore::TlsServerMode::Required);

		// sniff timeout clamped to 50..2000 ms
		configuration.setTlsSniffTimeoutMs(10);
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).sniffTimeoutMs, 50);
		configuration.setTlsSniffTimeoutMs(-1);
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).sniffTimeoutMs, 50);
		configuration.setTlsSniffTimeoutMs(5000);
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).sniffTimeoutMs, 2000);
		configuration.setTlsSniffTimeoutMs(500);
		QCOMPARE(TlsServerPolicy::fromConfiguration(configuration).sniffTimeoutMs, 500);

		// a default policy refuses everything
		QCOMPARE(TlsServerPolicy{}.mode, VeyonCore::TlsServerMode::Required);
	}
};

QStringList* TlsHostCredentialsTest::s_logMessages = nullptr;


QTEST_GUILESS_MAIN(TlsHostCredentialsTest)
#include "TlsHostCredentialsTest.moc"
