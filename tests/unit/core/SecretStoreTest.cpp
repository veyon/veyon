// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDirIterator>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "Configuration/LegacySecretMigration.h"
#include "Configuration/Proxy.h"
#include "Configuration/SecretPropertyRegistry.h"
#include "CryptoCore.h"
#include "PlatformSecretFunctions.h"
#include "SecretExport.h"
#include "SecretStore.h"
#include "VeyonConfiguration.h"

#include <map>

using Configuration::LegacySecretMigration;
using Configuration::SecretPropertyRegistry;
using Configuration::SecretReference;
using Veyon::Crypto::SecureBuffer;

static const Plugin::Uid LdapPluginUid{QStringLiteral("6f0a491e-c1c6-4338-8244-f823b0bf8670")};
static const Plugin::Uid ExternalVncServerUid{QStringLiteral("67dfc1c1-8f37-4539-a298-16e74e34fd8b")};
static const Plugin::Uid AddOnUid{QStringLiteral("d3b5e8a1-2c4f-4a96-8e17-5b0c9f6d2a78")};

// in-memory backend standing in for the platform implementations (tested separately)
class MemorySecretFunctions : public PlatformSecretFunctions
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
		if (m_failWrites)
		{
			return false;
		}
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

	bool prepareStorage() override
	{
		++m_prepareCount;
		return m_privileged;
	}

	bool isAvailable() override
	{
		return m_privileged;
	}

	QByteArray value(const QString& key)
	{
		const auto secret = readSecret(key);
		return secret.has_value() ? secret->toUnprotectedByteArray() : QByteArray();
	}

	bool m_privileged = true;
	bool m_failWrites = false;
	int m_prepareCount = 0;
	std::map<QString, SecureBuffer> m_secrets;
};


// property proxy like those of the plugins (LdapConfiguration, ExternalVncServerConfiguration)
#define FOREACH_TEST_SECRET_CONFIG_PROPERTY(OP) \
	OP(TestSecretConfiguration, m_configuration, Configuration::SecretReference, bindPassword, setBindPassword, "BindPassword", "LDAP", Configuration::SecretReference::defaultValue(QStringLiteral("ldap/bindPassword")), Configuration::Property::Flag::Standard)

DECLARE_CONFIG_PROXY(TestSecretConfiguration, FOREACH_TEST_SECRET_CONFIG_PROPERTY)
IMPLEMENT_CONFIG_PROXY(TestSecretConfiguration)

// a secret property of a plugin which is not loaded
#define FOREACH_UNREGISTERED_SECRET_CONFIG_PROPERTY(OP) \
	OP(UnregisteredSecretConfiguration, m_configuration, Configuration::SecretReference, clientSecret, setClientSecret, "ClientSecret", "NotInstalled", Configuration::SecretReference::defaultValue(QStringLiteral("not-installed/clientSecret")), Configuration::Property::Flag::Standard)

DECLARE_CONFIG_PROXY(UnregisteredSecretConfiguration, FOREACH_UNREGISTERED_SECRET_CONFIG_PROPERTY)
IMPLEMENT_CONFIG_PROXY(UnregisteredSecretConfiguration)


// 03-secretstore-config.md §6 (platform-independent part; file permissions, symlinks and
// atomicity of the Linux backend: tests/unit/LinuxSecretFunctionsTest.cpp)
class SecretStoreTest : public QObject
{
	Q_OBJECT
private:
	static constexpr auto MarkerSecret = "MarkerSecret-4b1d9e";
	static constexpr auto Passphrase = "correct horse battery staple";

	static QStringList* s_logMessages;

	QStringList m_logMessages;
	MemorySecretFunctions m_backend;
	QTemporaryDir m_temporaryDirectory;
	CryptoCore* m_cryptoCore = nullptr;

	static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_logMessages->append(message);
	}

	QString configurationFile(const QString& name) const
	{
		return m_temporaryDirectory.filePath(name + QStringLiteral(".json"));
	}

	QString legacyValue(const QByteArray& plainText) const
	{
		return m_cryptoCore->encryptPassword(CryptoCore::PlaintextPassword(plainText));
	}

	static QString value(const Configuration::Object& configuration, const QString& key, const QString& parentKey)
	{
		return configuration.value(key, parentKey, {}).toString();
	}

	static Veyon::Crypto::ByteView passphrase()
	{
		return Veyon::Crypto::ByteView(Passphrase, qsizetype(strlen(Passphrase)));
	}

	void verifyNoSecretInLog(const QByteArray& secret)
	{
		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY2(message.contains(QString::fromUtf8(secret)) == false, qPrintable(message));
			QVERIFY(message.contains(QString::fromLatin1(secret.toBase64())) == false);
			QVERIFY(message.contains(QString::fromLatin1(secret.toHex())) == false);
		}
	}

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_temporaryDirectory.isValid());
		m_cryptoCore = new CryptoCore;

		// the rows of the LDAP and external VNC server plugins (LdapPlugin, ExternalVncServer) and of
		// an add-on, as PluginManager::loadPlugins() adds them
		QVERIFY(SecretPropertyRegistry::add({QStringLiteral("LDAP"), QStringLiteral("BindPassword"),
											 QStringLiteral("ldap/bindPassword"), true},
											LdapPluginUid, QStringLiteral("LDAP")));
		QVERIFY(SecretPropertyRegistry::add({QStringLiteral("ExternalVncServer"), QStringLiteral("Password"),
											 QStringLiteral("vncserver-external/password"), true},
											ExternalVncServerUid, QStringLiteral("ExternalVncServer")));
		QVERIFY(SecretPropertyRegistry::add({QStringLiteral("AddOn/Client"), QStringLiteral("Secret"),
											 QStringLiteral("add-on/clientSecret"), true},
											AddOnUid, QStringLiteral("AddOn")));
		QVERIFY(SecretPropertyRegistry::add({QStringLiteral("AddOn/Client"), QStringLiteral("CertificateKey"),
											 QStringLiteral("add-on/clientCertificateKey"), false},
											AddOnUid, QStringLiteral("AddOn")));
	}

	void cleanupTestCase()
	{
		SecretPropertyRegistry::resetForTests();
		delete m_cryptoCore;
	}

	void init()
	{
		m_backend.m_privileged = true;
		m_backend.m_failWrites = false;
		m_backend.m_secrets.clear();
		SecretStore::setTestBackend(&m_backend, VeyonCore::InitializationFlag::SecretStore);
		m_logMessages.clear();
		s_logMessages = &m_logMessages;
		qInstallMessageHandler(captureMessage);
	}

	void cleanup()
	{
		qInstallMessageHandler(nullptr);
		SecretStore::setTestBackend(nullptr, VeyonCore::InitializationFlags());
		verifyNoSecretInLog(MarkerSecret);
	}

	void keyValidation_data()
	{
		QTest::addColumn<QString>("key");
		QTest::addColumn<bool>("valid");

		QTest::newRow("ldap") << QStringLiteral("ldap/bindPassword") << true;
		QTest::newRow("plugin scope") << QStringLiteral("vncserver-external/password") << true;
		QTest::newRow("dot in name") << QStringLiteral("tls/hostKey.next") << true;
		QTest::newRow("name charset") << QStringLiteral("a0-/Az09._-x") << true;
		QTest::newRow("maximum lengths") << QString(QString(32, u'a') + QLatin1Char('/') + QString(64, u'b')) << true;

		QTest::newRow("empty") << QString() << false;
		QTest::newRow("no slash") << QStringLiteral("ldapbindPassword") << false;
		QTest::newRow("two slashes") << QStringLiteral("ldap/bind/password") << false;
		QTest::newRow("double slash") << QStringLiteral("ldap//password") << false;
		QTest::newRow("absolute") << QStringLiteral("/etc/shadow") << false;
		QTest::newRow("parent directory") << QStringLiteral("../ldap") << false;
		QTest::newRow("dot dot name") << QStringLiteral("ldap/..") << false;
		QTest::newRow("dot name") << QStringLiteral("ldap/.") << false;
		QTest::newRow("trailing dot") << QStringLiteral("ldap/password.") << false;
		QTest::newRow("backslash") << QStringLiteral("ldap/a\\b") << false;
		QTest::newRow("NUL") << QString(QStringLiteral("ldap/pass") + QChar(0) + QStringLiteral("word")) << false;
		QTest::newRow("empty scope") << QStringLiteral("/password") << false;
		QTest::newRow("empty name") << QStringLiteral("ldap/") << false;
		QTest::newRow("upper-case scope") << QStringLiteral("LDAP/password") << false;
		QTest::newRow("underscore in scope") << QStringLiteral("my_plugin/password") << false;
		QTest::newRow("dot in scope") << QStringLiteral("my.plugin/password") << false;
		QTest::newRow("space") << QStringLiteral("ldap/bind password") << false;
		QTest::newRow("non-ASCII") << QStringLiteral("ldap/pässword") << false;
		QTest::newRow("scope too long") << QString(QString(33, u'a') + QStringLiteral("/b")) << false;
		QTest::newRow("name too long") << QString(QStringLiteral("a/") + QString(65, u'b')) << false;
	}

	void keyValidation()
	{
		QFETCH(QString, key);
		QFETCH(bool, valid);

		QCOMPARE(SecretStore::isValidKey(key), valid);

		if (valid == false)
		{
			// rejected before they reach the backend
			QVERIFY(SecretStore::write(key, SecureBuffer::fromByteArray("x")) == false);
			QVERIFY(SecretStore::read(key).has_value() == false);
			QVERIFY(SecretStore::remove(key) == false);
			QVERIFY(m_backend.m_secrets.empty());
		}
	}

	void roundtrip()
	{
		QVERIFY(SecretStore::isAvailable());
		QVERIFY(SecretStore::write(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray(MarkerSecret)));
		const auto secret = SecretStore::read(QStringLiteral("ldap/bindPassword"));
		QVERIFY(secret.has_value());
		QCOMPARE(secret->toUnprotectedByteArray(), QByteArray(MarkerSecret));
		QCOMPARE(SecretStore::keys(), QStringList{QStringLiteral("ldap/bindPassword")});

		// keys of the backend that are invalid are not reported
		m_backend.m_secrets.emplace(QStringLiteral("../x"), SecureBuffer::fromByteArray("x"));
		QCOMPARE(SecretStore::keys(), QStringList{QStringLiteral("ldap/bindPassword")});

		QVERIFY(SecretStore::remove(QStringLiteral("ldap/bindPassword")));
		QVERIFY(SecretStore::read(QStringLiteral("ldap/bindPassword")).has_value() == false);
	}

	// veyon-server/veyon-worker (root on X11, SYSTEM on Windows), master, helpers: never available,
	// as they are not initialized with the SecretStore flag (the component itself does not matter)
	void availabilityPerInitializationFlags_data()
	{
		QTest::addColumn<int>("flags");
		QTest::addColumn<bool>("expected");

		const auto secretStore = VeyonCore::InitializationFlag::SecretStore;
		QTest::newRow("service, cli, configurator") << int(VeyonCore::defaultInitialization() | secretStore) << true;
		QTest::newRow("secret store only") << int(VeyonCore::InitializationFlags(secretStore)) << true;
		QTest::newRow("server") << int(VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc) << false;
		QTest::newRow("worker, master, auth helper") << int(VeyonCore::defaultInitialization()) << false;
		QTest::newRow("minimal") << int(VeyonCore::minimalInitialization()) << false;
		QTest::newRow("all but secret store") << int(~VeyonCore::InitializationFlags(secretStore)) << false;
	}

	void availabilityPerInitializationFlags()
	{
		QFETCH(int, flags);
		QFETCH(bool, expected);

		QVERIFY(SecretStore::write(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray(MarkerSecret)));

		// the backend is privileged (as root/SYSTEM): only the flags decide
		SecretStore::setTestBackend(&m_backend, VeyonCore::InitializationFlags(flags));
		QCOMPARE(SecretStore::isAvailable(), expected);
		QCOMPARE(SecretStore::read(QStringLiteral("ldap/bindPassword")).has_value(), expected);
		QCOMPARE(SecretStore::keys().isEmpty(), expected == false);
		QCOMPARE(SecretStore::write(QStringLiteral("ldap/other"), SecureBuffer::fromByteArray("x")), expected);
		QCOMPARE(SecretStore::remove(QStringLiteral("ldap/bindPassword")), expected);
		// available: one written, one removed; unavailable: nothing changed
		QCOMPARE(m_backend.m_secrets.size(), size_t(1));
	}

	// start of a process with the SecretStore flag: the storage location is prepared in privileged
	// processes with the SecretStore flag only
	void prepareStorage()
	{
		QVERIFY(SecretStore::prepareStorage());
		QCOMPARE(m_backend.m_prepareCount, 1);

		SecretStore::setTestBackend(&m_backend, VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc);
		QVERIFY(SecretStore::prepareStorage() == false);
		QCOMPARE(m_backend.m_prepareCount, 1);

		SecretStore::setTestBackend(&m_backend, VeyonCore::InitializationFlag::SecretStore);
		m_backend.m_privileged = false;
		QVERIFY(SecretStore::prepareStorage() == false);
		QCOMPARE(m_backend.m_prepareCount, 1);
	}

	void unprivilegedProcess()
	{
		QVERIFY(SecretStore::write(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray(MarkerSecret)));

		m_backend.m_privileged = false;
		QVERIFY(SecretStore::isAvailable() == false);
		QVERIFY(SecretStore::read(QStringLiteral("ldap/bindPassword")).has_value() == false);
		QVERIFY(SecretStore::write(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("x")) == false);
		QVERIFY(SecretStore::remove(QStringLiteral("ldap/bindPassword")) == false);
		QVERIFY(SecretStore::keys().isEmpty());
		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray(MarkerSecret));
	}

	// L8: the registry decides which configuration keys are secrets
	void secretPropertyRegistry()
	{
		QStringList secretKeys;
		for (const auto& secretProperty : SecretPropertyRegistry::properties())
		{
			QVERIFY(SecretStore::isValidKey(secretProperty.secretKey));
			QVERIFY(secretKeys.contains(secretProperty.secretKey) == false);
			secretKeys.append(secretProperty.secretKey);
			const auto foundProperty = SecretPropertyRegistry::find(secretProperty.parentKey, secretProperty.key);
			QVERIFY(foundProperty.has_value());
			QCOMPARE(foundProperty->secretKey, secretProperty.secretKey);
		}
		QCOMPARE(SecretPropertyRegistry::secretKeys(), secretKeys);
		QVERIFY(secretKeys.contains(QStringLiteral("ldap/bindPassword")));
		QVERIFY(secretKeys.contains(QStringLiteral("vncserver-external/password")));
		QVERIFY(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindDN")).has_value() == false);
		QVERIFY(SecretPropertyRegistry::find(QStringLiteral("Foo"), QStringLiteral("MyPassword")).has_value() == false);

		// N4: case variants and empty key components never bypass the registry
		QVERIFY(SecretPropertyRegistry::find(QStringLiteral("ldap"), QStringLiteral("bindpassword")).has_value());
		QString key;
		QString parentKey;
		QVERIFY(Configuration::splitConfigurationKey(QStringLiteral("LDAP/BindPassword"), key, parentKey));
		QCOMPARE(parentKey, QStringLiteral("LDAP"));
		QCOMPARE(key, QStringLiteral("BindPassword"));
		QVERIFY(Configuration::splitConfigurationKey(QStringLiteral("Core"), key, parentKey));
		QCOMPARE(parentKey, QString());
		for (const auto& invalidKey : {QStringLiteral("/LDAP/BindPassword"), QStringLiteral("LDAP//BindPassword"),
									   QStringLiteral("LDAP/BindPassword/"), QString()})
		{
			QVERIFY2(Configuration::splitConfigurationKey(invalidKey, key, parentKey) == false, qPrintable(invalidKey));
		}

		// N7: unsetting a group removes the secrets of its properties
		QCOMPARE(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("LDAP")), QStringList{QStringLiteral("ldap/bindPassword")});
		QCOMPARE(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("ldap/bindpassword")), QStringList{QStringLiteral("ldap/bindPassword")});
		QCOMPARE(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("ExternalVncServer")),
				 QStringList{QStringLiteral("vncserver-external/password")});
		QVERIFY(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("LDAP/BindDN")).isEmpty());
		QVERIFY(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("LDA")).isEmpty());
		QVERIFY(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("Network")).isEmpty());
		// rows of plugins (add-ons), also in sub groups
		QCOMPARE(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("AddOn")),
				 (QStringList{QStringLiteral("add-on/clientSecret"), QStringLiteral("add-on/clientCertificateKey")}));
		QVERIFY(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("NotInstalled")).isEmpty());
	}

	void secretReference()
	{
		QCOMPARE(SecretReference(QStringLiteral("ldap/bindPassword")).toString(), QStringLiteral("secret:ldap/bindPassword"));
		QVERIFY(SecretReference(QStringLiteral("../etc")).isValid() == false);
		QCOMPARE(SecretReference(QStringLiteral("../etc")).toString(), QString());
		QCOMPARE(SecretReference::fromString(QStringLiteral("secret:tls/hostKey")).key(), QStringLiteral("tls/hostKey"));
		QVERIFY(SecretReference::fromString(QStringLiteral("tls/hostKey")).isValid() == false);
		QVERIFY(SecretReference::fromString(QStringLiteral("secret:a/b/c")).isValid() == false);
		QCOMPARE(SecretReference::defaultValue(QStringLiteral("ldap/bindPassword")).toString(),
				 QStringLiteral("secret:ldap/bindPassword"));
		QVERIFY(SecretReference::isLegacyValue(QStringLiteral("0a1b2c")));
		QVERIFY(SecretReference::isLegacyValue(QStringLiteral("secret:ldap/bindPassword")) == false);
		QVERIFY(SecretReference::isLegacyValue(QString()) == false);
	}

	// the configuration value never selects a different store key
	void secretReferenceProperty()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("property")));
		VeyonConfiguration configuration(&store);
		TestSecretConfiguration proxy(&configuration);

		QVERIFY(proxy.bindPasswordProperty().isSecretReference());
		QVERIFY(configuration.secretMigrationVersionProperty().isSecretReference() == false);
		QCOMPARE(proxy.bindPassword().key(), QStringLiteral("ldap/bindPassword"));

		configuration.setValue(QStringLiteral("BindPassword"), QStringLiteral("secret:ldap/bindPassword"), QStringLiteral("LDAP"));
		QCOMPARE(proxy.bindPassword().key(), QStringLiteral("ldap/bindPassword"));

		configuration.setValue(QStringLiteral("BindPassword"), QStringLiteral("secret:pki/caKey"), QStringLiteral("LDAP"));
		QVERIFY(proxy.bindPassword().isValid() == false);

		configuration.setValue(QStringLiteral("BindPassword"), legacyValue(MarkerSecret), QStringLiteral("LDAP"));
		QVERIFY(proxy.bindPassword().isValid() == false);

		proxy.setBindPassword(SecretReference(QStringLiteral("ldap/bindPassword")));
		QCOMPARE(value(configuration, QStringLiteral("BindPassword"), QStringLiteral("LDAP")),
				 QStringLiteral("secret:ldap/bindPassword"));
	}

	// §3a: a property which is not in the registry (e.g. its row was rejected) is never resolved
	void unregisteredSecretReferenceProperty()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("unregistered")));
		VeyonConfiguration configuration(&store);
		UnregisteredSecretConfiguration proxy(&configuration);

		QVERIFY(proxy.clientSecretProperty().isSecretReference());
		QVERIFY(proxy.clientSecret().isValid() == false);
		QVERIFY(m_logMessages.filter(QStringLiteral("is not registered")).isEmpty() == false);
	}

	// H1 of the WP4 review: top-level values (e.g. the secrets document of an export) must be removable
	void removeTopLevelValue()
	{
		Configuration::Object configuration;
		configuration.setValue(QStringLiteral("v"), 1, QStringLiteral("VeyonSecrets"));
		configuration.setValue(QStringLiteral("BindDN"), QStringLiteral("cn=veyon"), QStringLiteral("LDAP"));
		QVERIFY(configuration.hasValue(QStringLiteral("VeyonSecrets"), {}));

		configuration.removeValue(QStringLiteral("VeyonSecrets"), {});
		QVERIFY(configuration.hasValue(QStringLiteral("VeyonSecrets"), {}) == false);
		QVERIFY(configuration.data().contains(QStringLiteral("VeyonSecrets")) == false);
		QCOMPARE(value(configuration, QStringLiteral("BindDN"), QStringLiteral("LDAP")), QStringLiteral("cn=veyon"));
	}

	// H1: configuration files never load or save the secrets document of an export
	void jsonStoreSkipsSecretsDocument()
	{
		const auto fileName = configurationFile(QStringLiteral("bundle"));
		{
			QFile file(fileName);
			QVERIFY(file.open(QFile::WriteOnly));
			file.write(R"({"VeyonSecrets":{"v":1,"kind":"veyon-secrets","ct":"AAAA"},"LDAP":{"BindDN":"cn=veyon"}})");
		}

		Configuration::JsonStore store(Configuration::Store::Scope::System, fileName);
		Configuration::Object configuration(&store);
		QVERIFY(configuration.hasValue(QStringLiteral("VeyonSecrets"), {}) == false);
		QCOMPARE(value(configuration, QStringLiteral("BindDN"), QStringLiteral("LDAP")), QStringLiteral("cn=veyon"));

		// e.g. a configuration with the document from an earlier import
		configuration.setValue(QStringLiteral("ct"), QStringLiteral("AAAA"), QStringLiteral("VeyonSecrets"));
		configuration.setValue(QStringLiteral("v"), 1, QStringLiteral("LDAP/VeyonSecrets"));
		const auto json = Configuration::JsonStore::toJsonObject(&configuration);
		QVERIFY(json.contains(QStringLiteral("VeyonSecrets")) == false);
		// only the top level is reserved
		QVERIFY(json.value(QStringLiteral("LDAP")).toObject().contains(QStringLiteral("VeyonSecrets")));
		configuration.flushStore();
		QFile file(fileName);
		QVERIFY(file.open(QFile::ReadOnly));
		QVERIFY(file.readAll().contains("\"ct\"") == false);
	}

	void migration()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("migration")));
		VeyonConfiguration configuration(&store);
		configuration.setValue(QStringLiteral("BindPassword"), legacyValue(MarkerSecret), QStringLiteral("LDAP"));
		configuration.setValue(QStringLiteral("Password"), legacyValue("vnc-password"), QStringLiteral("ExternalVncServer"));
		configuration.setValue(QStringLiteral("BindDN"), QStringLiteral("cn=veyon"), QStringLiteral("LDAP"));

		QVERIFY(LegacySecretMigration::hasLegacySecrets(configuration));
		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::Migrated);

		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray(MarkerSecret));
		QCOMPARE(m_backend.value(QStringLiteral("vncserver-external/password")), QByteArray("vnc-password"));
		QCOMPARE(value(configuration, QStringLiteral("BindPassword"), QStringLiteral("LDAP")),
				 QStringLiteral("secret:ldap/bindPassword"));
		QCOMPARE(value(configuration, QStringLiteral("Password"), QStringLiteral("ExternalVncServer")),
				 QStringLiteral("secret:vncserver-external/password"));
		QCOMPARE(value(configuration, QStringLiteral("BindDN"), QStringLiteral("LDAP")), QStringLiteral("cn=veyon"));
		QCOMPARE(configuration.secretMigrationVersion(), 1);
		QVERIFY(LegacySecretMigration::hasLegacySecrets(configuration) == false);

		// second run: no-op
		m_backend.m_secrets.erase(QStringLiteral("ldap/bindPassword"));
		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::NothingToMigrate);
		QVERIFY(m_backend.m_secrets.count(QStringLiteral("ldap/bindPassword")) == 0);
	}

	// a former password property declared by a plugin (add-on) is migrated like the core ones;
	// its non-legacy secret property is not touched
	void migrationOfPluginSecret()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("add-on")));
		VeyonConfiguration configuration(&store);
		configuration.setValue(QStringLiteral("Secret"), legacyValue(MarkerSecret), QStringLiteral("AddOn/Client"));
		configuration.setValue(QStringLiteral("CertificateKey"), QStringLiteral("plain"), QStringLiteral("AddOn/Client"));

		QVERIFY(LegacySecretMigration::hasLegacySecrets(configuration));
		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::Migrated);
		QCOMPARE(m_backend.value(QStringLiteral("add-on/clientSecret")), QByteArray(MarkerSecret));
		QCOMPARE(value(configuration, QStringLiteral("Secret"), QStringLiteral("AddOn/Client")),
				 QStringLiteral("secret:add-on/clientSecret"));
		QCOMPARE(value(configuration, QStringLiteral("CertificateKey"), QStringLiteral("AddOn/Client")), QStringLiteral("plain"));
		QVERIFY(m_backend.m_secrets.count(QStringLiteral("add-on/clientCertificateKey")) == 0);
		QVERIFY(LegacySecretMigration::hasLegacySecrets(configuration) == false);
	}

	// §3a: values of properties of plugins which are not loaded are left untouched, also at
	// privileged start, so that they can be migrated once the plugin is installed
	void unregisteredPropertiesUntouched()
	{
		const auto fileName = configurationFile(QStringLiteral("not-installed"));
		const auto legacyPassword = legacyValue(MarkerSecret);
		Configuration::JsonStore store(Configuration::Store::Scope::System, fileName);
		VeyonConfiguration stored(&store);
		stored.setValue(QStringLiteral("ClientSecret"), legacyPassword, QStringLiteral("NotInstalled"));
		stored.setValue(QStringLiteral("ClientCertificateKey"), QStringLiteral("secret:not-installed/clientCertificateKey"),
						QStringLiteral("NotInstalled"));
		stored.setValue(QStringLiteral("Other"), QStringLiteral("secret:pki/caKey"), QStringLiteral("NotInstalled"));
		VeyonConfiguration running(&store);

		QVERIFY(LegacySecretMigration::hasLegacySecrets(stored) == false);
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::NothingToMigrate);
		QVERIFY(LegacySecretMigration::scrubConfiguration(stored) == false);
		QCOMPARE(LegacySecretMigration::removeLegacySecrets(stored), 0);
		QVERIFY(LegacySecretMigration::missingSecretKeys(stored).isEmpty());

		QCOMPARE(value(stored, QStringLiteral("ClientSecret"), QStringLiteral("NotInstalled")), legacyPassword);
		QCOMPARE(value(stored, QStringLiteral("ClientCertificateKey"), QStringLiteral("NotInstalled")),
				 QStringLiteral("secret:not-installed/clientCertificateKey"));
		QCOMPARE(value(stored, QStringLiteral("Other"), QStringLiteral("NotInstalled")), QStringLiteral("secret:pki/caKey"));
		QVERIFY(m_backend.m_secrets.empty());

		// once the plugin is installed, the next privileged start migrates the legacy value
		QVERIFY(SecretPropertyRegistry::add({QStringLiteral("NotInstalled"), QStringLiteral("ClientSecret"),
											 QStringLiteral("not-installed/clientSecret"), true},
											Plugin::Uid{QStringLiteral("0c6a4f1e-3b8d-4e27-9f50-7d2a1c9e8b43")},
											QStringLiteral("NotInstalled")));
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Migrated);
		QCOMPARE(m_backend.value(QStringLiteral("not-installed/clientSecret")), QByteArray(MarkerSecret));
		QCOMPARE(value(stored, QStringLiteral("ClientSecret"), QStringLiteral("NotInstalled")),
				 QStringLiteral("secret:not-installed/clientSecret"));
		QCOMPARE(value(running, QStringLiteral("ClientSecret"), QStringLiteral("NotInstalled")),
				 QStringLiteral("secret:not-installed/clientSecret"));
		QCOMPARE(value(stored, QStringLiteral("Other"), QStringLiteral("NotInstalled")), QStringLiteral("secret:pki/caKey"));
	}

	void migrationOfSpecialValues()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("special")));
		VeyonConfiguration configuration(&store);

		// empty password (encrypted) means "no secret"; plain text of old versions is migrated as is
		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("old"));
		configuration.setValue(QStringLiteral("BindPassword"), legacyValue(QByteArray()), QStringLiteral("LDAP"));
		configuration.setValue(QStringLiteral("Password"), QStringLiteral("plain"), QStringLiteral("ExternalVncServer"));

		// automatic migration: an existing secret is neither replaced nor deleted (WP4 re-review N2)
		auto automatic = configuration;
		QStringList replacedSecretKeys;
		QCOMPARE(LegacySecretMigration::migrate(automatic, *m_cryptoCore, LegacySecretMigration::ExistingSecrets::Keep,
												&replacedSecretKeys),
				 LegacySecretMigration::Result::Migrated);
		QVERIFY(replacedSecretKeys.isEmpty());
		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray("old"));
		QCOMPARE(value(automatic, QStringLiteral("BindPassword"), QStringLiteral("LDAP")),
				 QStringLiteral("secret:ldap/bindPassword"));
		m_backend.m_secrets.erase(QStringLiteral("vncserver-external/password"));

		// explicit import: overwrites (here: deletes) with a warning naming the key
		m_logMessages.clear();
		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore, LegacySecretMigration::ExistingSecrets::Overwrite,
												&replacedSecretKeys),
				 LegacySecretMigration::Result::Migrated);
		// reported for the admin (WP4 review E): only the secret that was stored before
		QCOMPARE(replacedSecretKeys, QStringList{QStringLiteral("ldap/bindPassword")});
		QVERIFY(m_backend.m_secrets.count(QStringLiteral("ldap/bindPassword")) == 0);
		QVERIFY(m_logMessages.filter(QStringLiteral("ldap/bindPassword")).isEmpty() == false);
		QCOMPARE(m_backend.value(QStringLiteral("vncserver-external/password")), QByteArray("plain"));
		QCOMPARE(value(configuration, QStringLiteral("BindPassword"), QStringLiteral("LDAP")),
				 QStringLiteral("secret:ldap/bindPassword"));
	}

	// WP4 review C: references to secrets that are not stored (import without the secrets)
	void missingSecretKeys()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("missing")));
		VeyonConfiguration configuration(&store);
		QVERIFY(LegacySecretMigration::missingSecretKeys(configuration).isEmpty());

		configuration.setValue(QStringLiteral("BindPassword"), QStringLiteral("secret:ldap/bindPassword"), QStringLiteral("LDAP"));
		configuration.setValue(QStringLiteral("Password"), QStringLiteral("secret:vncserver-external/password"),
							   QStringLiteral("ExternalVncServer"));
		m_backend.m_secrets.emplace(QStringLiteral("vncserver-external/password"), SecureBuffer::fromByteArray("vnc"));
		QCOMPARE(LegacySecretMigration::missingSecretKeys(configuration), QStringList{QStringLiteral("ldap/bindPassword")});

		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("bind"));
		QVERIFY(LegacySecretMigration::missingSecretKeys(configuration).isEmpty());

		// a legacy value is no reference
		configuration.setValue(QStringLiteral("BindPassword"), legacyValue(MarkerSecret), QStringLiteral("LDAP"));
		m_backend.m_secrets.clear();
		QCOMPARE(LegacySecretMigration::missingSecretKeys(configuration),
				 QStringList{QStringLiteral("vncserver-external/password")});
	}

	// N2: the configuration management replaced the whole configuration (no marker) - a stored,
	// rotated secret survives at every start
	void startupMigrationWithoutMarker()
	{
		const auto fileName = configurationFile(QStringLiteral("nomarker"));
		Configuration::JsonStore store(Configuration::Store::Scope::System, fileName);
		VeyonConfiguration stored(&store);
		VeyonConfiguration running(&store);
		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("rotated-password"));

		for (const auto& legacyPlainText : {QByteArray("old-password"), QByteArray()})
		{
			stored.removeValue(QStringLiteral("SecretMigrationVersion"), QStringLiteral("Core"));
			stored.setValue(QStringLiteral("BindPassword"), legacyValue(legacyPlainText), QStringLiteral("LDAP"));
			QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
					 LegacySecretMigration::Result::Migrated);
			QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray("rotated-password"));
			QCOMPARE(value(stored, QStringLiteral("BindPassword"), QStringLiteral("LDAP")),
					 QStringLiteral("secret:ldap/bindPassword"));
		}
		verifyNoSecretInLog("old-password");
	}

	// partial failure leaves the legacy value in place (retried at the next start), no marker
	void migrationPartialFailure()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("partial")));
		VeyonConfiguration configuration(&store);
		const auto undecryptable = QString(QString(1024, u'0'));
		configuration.setValue(QStringLiteral("BindPassword"), undecryptable, QStringLiteral("LDAP"));
		configuration.setValue(QStringLiteral("Password"), legacyValue("vnc-password"), QStringLiteral("ExternalVncServer"));

		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::Failed);
		QCOMPARE(value(configuration, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), undecryptable);
		QCOMPARE(value(configuration, QStringLiteral("Password"), QStringLiteral("ExternalVncServer")),
				 QStringLiteral("secret:vncserver-external/password"));
		QCOMPARE(configuration.secretMigrationVersion(), 0);

		// store write failure
		const auto legacyPassword = legacyValue(MarkerSecret);
		configuration.setValue(QStringLiteral("BindPassword"), legacyPassword, QStringLiteral("LDAP"));
		m_backend.m_failWrites = true;
		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::Failed);
		QCOMPARE(value(configuration, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), legacyPassword);
	}

	void migrationUnprivileged()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("unprivileged")));
		VeyonConfiguration configuration(&store);
		const auto legacyPassword = legacyValue(MarkerSecret);
		configuration.setValue(QStringLiteral("BindPassword"), legacyPassword, QStringLiteral("LDAP"));

		for (const auto flags : {VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc,
								 VeyonCore::defaultInitialization(), VeyonCore::minimalInitialization()})
		{
			SecretStore::setTestBackend(&m_backend, flags);
			QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::Failed);
		}

		SecretStore::setTestBackend(&m_backend, VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::SecretStore);
		m_backend.m_privileged = false;
		QCOMPARE(LegacySecretMigration::migrate(configuration, *m_cryptoCore), LegacySecretMigration::Result::Failed);

		VeyonConfiguration running(&store);
		running.setValue(QStringLiteral("BindPassword"), legacyPassword, QStringLiteral("LDAP"));
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(configuration, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Failed);

		QCOMPARE(value(configuration, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), legacyPassword);
		QVERIFY(m_backend.m_secrets.empty());
		QVERIFY(QFileInfo::exists(configurationFile(QStringLiteral("unprivileged"))) == false);
	}

	// service start: stored configuration migrated and saved (with one-time backup), running one updated
	void startupMigration()
	{
		const auto fileName = configurationFile(QStringLiteral("startup"));
		const auto legacyPassword = legacyValue(MarkerSecret);
		{
			Configuration::JsonStore store(Configuration::Store::Scope::System, fileName);
			VeyonConfiguration configuration(&store);
			configuration.setValue(QStringLiteral("BindPassword"), legacyPassword, QStringLiteral("LDAP"));
			configuration.flushStore();
		}

		Configuration::JsonStore store(Configuration::Store::Scope::System, fileName);
		VeyonConfiguration stored(&store);
		VeyonConfiguration running(&store);
		running.setValue(QStringLiteral("ApplicationVersion"), 99, QStringLiteral("Core"));

		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Migrated);

		QFile file(fileName);
		QVERIFY(file.open(QFile::ReadOnly));
		const auto savedConfiguration = file.readAll();
		QVERIFY(savedConfiguration.contains(legacyPassword.toLatin1()) == false);
		QVERIFY(savedConfiguration.contains("secret:ldap/bindPassword"));
		QVERIFY(savedConfiguration.contains("SecretMigrationVersion"));
		// in-memory upgrades of the running configuration are kept
		QCOMPARE(running.value(QStringLiteral("ApplicationVersion"), QStringLiteral("Core"), {}).toInt(), 99);
		QCOMPARE(value(running, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), QStringLiteral("secret:ldap/bindPassword"));
		QCOMPARE(running.secretMigrationVersion(), 1);
		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray(MarkerSecret));

		// backup of the pre-migration configuration (legacy value included) for downgrades
		const auto backup = m_backend.value(QStringLiteral("core/preMigrationConfig"));
		QVERIFY(backup.contains(legacyPassword.toLatin1()));
		QVERIFY(QJsonDocument::fromJson(backup).isObject());

		// re-applied by a GPO after the secret was rotated: scrubbed with a warning, the stored
		// (rotated) secret and the backup are kept (WP4 review M4)
		m_backend.m_secrets[QStringLiteral("ldap/bindPassword")] = SecureBuffer::fromByteArray("rotated-password");
		stored.setValue(QStringLiteral("BindPassword"), legacyValue("old-password"), QStringLiteral("LDAP"));
		m_logMessages.clear();
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Migrated);
		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray("rotated-password"));
		QCOMPARE(value(stored, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), QStringLiteral("secret:ldap/bindPassword"));
		QCOMPARE(m_backend.value(QStringLiteral("core/preMigrationConfig")), backup);
		QVERIFY(m_logMessages.filter(QStringLiteral("re-applied by configuration management")).isEmpty() == false);
		verifyNoSecretInLog("old-password");
		verifyNoSecretInLog("rotated-password");

		// a re-applied encrypted empty password doesn't delete the stored secret either
		stored.setValue(QStringLiteral("BindPassword"), legacyValue(QByteArray()), QStringLiteral("LDAP"));
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Migrated);
		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray("rotated-password"));

		// without a stored secret the re-applied value is migrated
		m_backend.m_secrets.erase(QStringLiteral("ldap/bindPassword"));
		stored.setValue(QStringLiteral("BindPassword"), legacyValue("new-password"), QStringLiteral("LDAP"));
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Migrated);
		QCOMPARE(m_backend.value(QStringLiteral("ldap/bindPassword")), QByteArray("new-password"));

		// nothing to do afterwards
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::NothingToMigrate);
	}

	// H1 and point 4 of the WP4 review: a stored configuration never keeps the secrets document of
	// an export or references of secret properties to other keys
	void startupScrubbing()
	{
		const auto fileName = configurationFile(QStringLiteral("scrub"));
		Configuration::JsonStore store(Configuration::Store::Scope::System, fileName);
		VeyonConfiguration stored(&store);
		stored.setValue(QStringLiteral("ct"), QStringLiteral("AAAA"), QStringLiteral("VeyonSecrets"));
		stored.setValue(QStringLiteral("BindPassword"), QStringLiteral("secret:pki/caKey"), QStringLiteral("LDAP"));
		VeyonConfiguration running(&store);
		running.setValue(QStringLiteral("ct"), QStringLiteral("AAAA"), QStringLiteral("VeyonSecrets"));

		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Migrated);
		QVERIFY(stored.hasValue(QStringLiteral("VeyonSecrets"), {}) == false);
		QVERIFY(running.hasValue(QStringLiteral("VeyonSecrets"), {}) == false);
		QCOMPARE(value(stored, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), QStringLiteral("secret:ldap/bindPassword"));
		QCOMPARE(value(running, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), QStringLiteral("secret:ldap/bindPassword"));
		QFile file(fileName);
		QVERIFY(file.open(QFile::ReadOnly));
		const auto saved = file.readAll();
		QVERIFY(saved.contains("pki/caKey") == false);
		QVERIFY(saved.contains("secret:ldap/bindPassword"));

		// unprivileged: nothing is changed
		stored.setValue(QStringLiteral("BindPassword"), QStringLiteral("secret:pki/caKey"), QStringLiteral("LDAP"));
		m_backend.m_privileged = false;
		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::NothingToMigrate);
		QCOMPARE(value(stored, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), QStringLiteral("secret:pki/caKey"));
	}

	// L6: a backup that doesn't fit into the store fails the migration instead of being skipped
	void startupMigrationBackupTooLarge()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("large")));
		VeyonConfiguration stored(&store);
		VeyonConfiguration running(&store);
		const auto legacyPassword = legacyValue(MarkerSecret);
		stored.setValue(QStringLiteral("BindPassword"), legacyPassword, QStringLiteral("LDAP"));
		stored.setValue(QStringLiteral("Large"), QString(int(SecureBuffer::MaximumSize) + 1, u'x'), QStringLiteral("Test"));

		QCOMPARE(LegacySecretMigration::migrateStoredConfiguration(stored, running, *m_cryptoCore),
				 LegacySecretMigration::Result::Failed);
		QCOMPARE(value(stored, QStringLiteral("BindPassword"), QStringLiteral("LDAP")), legacyPassword);
		QVERIFY(m_backend.m_secrets.empty());
	}

	// exports (configurator, veyon-cli) never contain legacy ciphertext
	void exportWithoutSecrets()
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, configurationFile(QStringLiteral("export")));
		VeyonConfiguration configuration(&store);
		configuration.setValue(QStringLiteral("BindPassword"), legacyValue(MarkerSecret), QStringLiteral("LDAP"));
		configuration.setValue(QStringLiteral("Password"), QStringLiteral("secret:vncserver-external/password"),
							   QStringLiteral("ExternalVncServer"));

		Configuration::Object exported(configuration);
		QCOMPARE(LegacySecretMigration::removeLegacySecrets(exported), 1);
		const auto json = QJsonDocument(Configuration::JsonStore::toJsonObject(&exported)).toJson();
		QVERIFY(json.contains("BindPassword") == false);
		QVERIFY(json.contains("secret:vncserver-external/password"));
		QVERIFY(LegacySecretMigration::hasLegacySecrets(configuration));
	}

	void exportWithSecrets_data()
	{
		QTest::addColumn<bool>("argon2id");
		QTest::newRow("scrypt") << false;
		QTest::newRow("argon2id") << true;
	}

	void exportWithSecrets()
	{
		QFETCH(bool, argon2id);
		const auto kdf = argon2id ? SecretExport::Kdf::Argon2id : SecretExport::Kdf::Scrypt;
		if (kdf == SecretExport::Kdf::Argon2id && Veyon::Crypto::PasswordKdf::isArgon2idAvailable() == false)
		{
			QVERIFY(SecretExport::encrypt({}, passphrase(), kdf).has_value() == false);
			QSKIP("Argon2id requires OpenSSL 3.2");
		}

		SecretExport::Secrets secrets;
		secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray(MarkerSecret));
		secrets.emplace(QStringLiteral("vncserver-external/password"), SecureBuffer::fromByteArray(QByteArray("\0\x01\xff", 3)));
		secrets.emplace(QStringLiteral("entra/clientSecret"), SecureBuffer());

		const auto document = SecretExport::encrypt(secrets, passphrase(), kdf);
		QVERIFY(document.has_value());
		const auto json = QJsonDocument(*document).toJson();
		QVERIFY(json.contains(MarkerSecret) == false);
		QVERIFY(json.contains(QByteArray(MarkerSecret).toBase64()) == false);
		QCOMPARE(document->value(QStringLiteral("v")).toInt(), 1);
		QCOMPARE(document->value(QStringLiteral("kind")).toString(), QStringLiteral("veyon-secrets"));
		QCOMPARE(document->value(QStringLiteral("kdf")).toString(),
				 kdf == SecretExport::Kdf::Scrypt ? QStringLiteral("scrypt") : QStringLiteral("argon2id"));
		const auto kdfParameters = document->value(QStringLiteral("kdfParams")).toObject();
		if (kdf == SecretExport::Kdf::Scrypt)
		{
			QCOMPARE(kdfParameters.value(QStringLiteral("N")).toInt(), 32768);
			QCOMPARE(kdfParameters.value(QStringLiteral("r")).toInt(), 8);
			QCOMPARE(kdfParameters.value(QStringLiteral("p")).toInt(), 1);
		}
		else
		{
			QCOMPARE(kdfParameters.value(QStringLiteral("t")).toInt(), 3);
			QCOMPARE(kdfParameters.value(QStringLiteral("m")).toInt(), 65536);
			QCOMPARE(kdfParameters.value(QStringLiteral("p")).toInt(), 1);
		}

		// roundtrip through JSON text
		const auto decrypted = SecretExport::decrypt(QJsonDocument::fromJson(json).object(), passphrase());
		QVERIFY(decrypted.has_value());
		QCOMPARE(decrypted->size(), secrets.size());
		for (const auto& entry : secrets)
		{
			QVERIFY(decrypted->count(entry.first) == 1);
			QCOMPARE(decrypted->at(entry.first).toUnprotectedByteArray(), entry.second.toUnprotectedByteArray());
		}

		// wrong passphrase
		const QByteArray wrongPassphrase("correct horse battery stapler");
		QVERIFY(SecretExport::decrypt(*document, wrongPassphrase).has_value() == false);

		// modified ciphertext, salt, nonce or KDF parameters (associated data)
		const auto modified = [&document](const QString& key, const QJsonValue& value) {
			auto copy = *document;
			copy[key] = value;
			return copy;
		};
		auto ciphertext = QByteArray::fromBase64(document->value(QStringLiteral("ct")).toString().toLatin1());
		ciphertext[0] = char(ciphertext[0] ^ 1);
		QVERIFY(SecretExport::decrypt(modified(QStringLiteral("ct"), QString::fromLatin1(ciphertext.toBase64())),
									  passphrase())
					.has_value() == false);
		auto salt = QByteArray::fromBase64(document->value(QStringLiteral("salt")).toString().toLatin1());
		salt[0] = char(salt[0] ^ 1);
		QVERIFY(SecretExport::decrypt(modified(QStringLiteral("salt"), QString::fromLatin1(salt.toBase64())),
									  passphrase())
					.has_value() == false);
		auto nonce = QByteArray::fromBase64(document->value(QStringLiteral("nonce")).toString().toLatin1());
		nonce[0] = char(nonce[0] ^ 1);
		QVERIFY(SecretExport::decrypt(modified(QStringLiteral("nonce"), QString::fromLatin1(nonce.toBase64())),
									  passphrase())
					.has_value() == false);
		auto changedParameters = kdfParameters;
		changedParameters[QStringLiteral("p")] = 2;
		QVERIFY(SecretExport::decrypt(modified(QStringLiteral("kdfParams"), changedParameters), passphrase()).has_value() == false);
	}

	void exportPassphrase()
	{
		QVERIFY(SecretExport::isAcceptablePassphrase(QByteArray("123456789012")));
		QVERIFY(SecretExport::isAcceptablePassphrase(QByteArray("12345678901")) == false);
		// characters, not bytes
		QVERIFY(SecretExport::isAcceptablePassphrase(QStringLiteral("ääääääääääää").toUtf8()));
		QVERIFY(SecretExport::isAcceptablePassphrase(QStringLiteral("äääääääääää").toUtf8()) == false);
		QVERIFY(SecretExport::isAcceptablePassphrase(QByteArray("\xff\xfe\xfd\xfc\xfb\xfa\xf9\xf8\xf7\xf6\xf5\xf4\xf3")) == false);
		QVERIFY(SecretExport::encrypt({}, QByteArray("short"), SecretExport::Kdf::Scrypt).has_value() == false);
	}

	void exportDocumentParser()
	{
		const QJsonObject valid{
			{QStringLiteral("v"), 1},
			{QStringLiteral("kind"), QStringLiteral("veyon-secrets")},
			{QStringLiteral("kdf"), QStringLiteral("scrypt")},
			{QStringLiteral("kdfParams"), QJsonObject{{QStringLiteral("N"), 1024}, {QStringLiteral("r"), 8}, {QStringLiteral("p"), 1}}},
			{QStringLiteral("salt"), QString::fromLatin1(QByteArray(16, 's').toBase64())},
			{QStringLiteral("nonce"), QString::fromLatin1(QByteArray(12, 'n').toBase64())},
			{QStringLiteral("ct"), QString::fromLatin1(QByteArray(32, 'c').toBase64())},
		};
		QVERIFY(SecretExport::parseDocument(valid).has_value());

		const auto modified = [&valid](const QString& key, const QJsonValue& value) {
			auto copy = valid;
			if (value.isUndefined())
			{
				copy.remove(key);
			}
			else
			{
				copy[key] = value;
			}
			return SecretExport::parseDocument(copy).has_value();
		};
		const auto scrypt = [](qint64 cost, qint64 blockSize, qint64 parallelism) {
			return QJsonObject{{QStringLiteral("N"), cost}, {QStringLiteral("r"), blockSize}, {QStringLiteral("p"), parallelism}};
		};

		QVERIFY(modified(QStringLiteral("v"), 2) == false);
		QVERIFY(modified(QStringLiteral("v"), QStringLiteral("1")) == false);
		QVERIFY(modified(QStringLiteral("v"), 1.5) == false);
		QVERIFY(modified(QStringLiteral("kind"), QStringLiteral("veyon-config")) == false);
		QVERIFY(modified(QStringLiteral("kdf"), QStringLiteral("pbkdf2")) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), QJsonValue::Undefined) == false);
		// reader bounds against resource exhaustion by crafted files
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1 << 18, 8, 1))); // 256 MiB is the limit
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1 << 18, 9, 1)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1 << 19, 1, 1)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1 << 20, 1, 1)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1000, 8, 1)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1024, 17, 1)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1024, 8, 5)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(1024, 8, 0)) == false);
		QVERIFY(modified(QStringLiteral("kdfParams"), scrypt(-1024, 8, 1)) == false);
		auto extraParameter = scrypt(1024, 8, 1);
		extraParameter[QStringLiteral("x")] = 1;
		QVERIFY(modified(QStringLiteral("kdfParams"), extraParameter) == false);
		QVERIFY(modified(QStringLiteral("salt"), QString::fromLatin1(QByteArray(15, 's').toBase64())) == false);
		QVERIFY(modified(QStringLiteral("salt"), QStringLiteral("not base64!")) == false);
		QVERIFY(modified(QStringLiteral("nonce"), QString::fromLatin1(QByteArray(16, 'n').toBase64())) == false);
		QVERIFY(modified(QStringLiteral("ct"), QString::fromLatin1(QByteArray(15, 'c').toBase64())) == false);
		QVERIFY(modified(QStringLiteral("ct"), QString(2 * 1024 * 1024, u'A')) == false);
		QVERIFY(modified(QStringLiteral("ct"), 42) == false);

		auto argon2id = valid;
		argon2id[QStringLiteral("kdf")] = QStringLiteral("argon2id");
		argon2id[QStringLiteral("kdfParams")] = QJsonObject{{QStringLiteral("t"), 3}, {QStringLiteral("m"), 65536}, {QStringLiteral("p"), 1}};
		QVERIFY(SecretExport::parseDocument(argon2id).has_value());
		argon2id[QStringLiteral("kdfParams")] = QJsonObject{{QStringLiteral("t"), 3}, {QStringLiteral("m"), 512 * 1024}, {QStringLiteral("p"), 1}};
		QVERIFY(SecretExport::parseDocument(argon2id).has_value() == false);
		argon2id[QStringLiteral("kdfParams")] = QJsonObject{{QStringLiteral("t"), 9}, {QStringLiteral("m"), 65536}, {QStringLiteral("p"), 1}};
		QVERIFY(SecretExport::parseDocument(argon2id).has_value() == false);
	}

	void exportPlaintextParser()
	{
		const auto parse = [](const QByteArray& json) { return SecretExport::parsePlaintext(json).has_value(); };

		QVERIFY(parse(R"({"secrets":{"ldap/bindPassword":"c2VjcmV0"}})"));
		QVERIFY(parse(R"({"secrets":{}})"));
		QVERIFY(parse(R"({"secrets":{"../x":"c2VjcmV0"}})") == false);
		QVERIFY(parse(R"({"secrets":{"ldap/bindPassword":"!!"}})") == false);
		QVERIFY(parse(R"({"secrets":{"ldap/bindPassword":1}})") == false);
		QVERIFY(parse(R"({"secrets":[]})") == false);
		QVERIFY(parse(R"({"secrets":{},"other":1})") == false);
		QVERIFY(parse(R"([1])") == false);
		QVERIFY(parse("{") == false);
	}

	// 05-tls.md §7: the public default-pkey.pem only decrypts legacy values in LegacySecretMigration
	void noLegacyPasswordEncryption()
	{
		static const QRegularExpression legacyApiRX(QStringLiteral("encryptPassword|decryptPassword|fromPlainText"));
		const QStringList allowedFiles{
			QStringLiteral("core/src/CryptoCore.h"),
			QStringLiteral("core/src/CryptoCore.cpp"),
			QStringLiteral("core/src/Configuration/LegacySecretMigration.cpp"),
			// UserLogin feature message: replaced by a plain argument over verified TLS in WP5b, which
			// needs TransportState::TlsVerified and isSecureTransport (WP6/WP7); the redaction of the
			// argument in logs is in place already (05-tls.md §7, §7a)
			QStringLiteral("plugins/usersessioncontrol/UserSessionControlPlugin.cpp"),
		};

		const QDir sourceDirectory(QStringLiteral(VEYON_SOURCE_DIR));
		QStringList offendingFiles;
		for (const auto& subDirectory : {"cli", "configurator", "core", "master", "plugins", "server", "service", "worker"})
		{
			QDirIterator it(sourceDirectory.filePath(QLatin1String(subDirectory)), {QStringLiteral("*.cpp"), QStringLiteral("*.h")},
							QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext())
			{
				const auto filePath = it.next();
				const auto relativePath = sourceDirectory.relativeFilePath(filePath);
				QFile file(filePath);
				QVERIFY(file.open(QFile::ReadOnly));
				if (QString::fromUtf8(file.readAll()).contains(legacyApiRX) && allowedFiles.contains(relativePath) == false)
				{
					offendingFiles.append(relativePath);
				}
			}
		}

		QVERIFY2(offendingFiles.isEmpty(), qPrintable(offendingFiles.join(QLatin1Char(' '))));
	}
};


QStringList* SecretStoreTest::s_logMessages = nullptr;


QTEST_GUILESS_MAIN(SecretStoreTest)
#include "SecretStoreTest.moc"
