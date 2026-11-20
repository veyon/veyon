// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPluginLoader>
#include <QtTest>

#include "Configuration/SecretPropertyRegistry.h"
#include "PluginInterface.h"
#include "PluginManager.h"

using Configuration::SecretProperty;
using Configuration::SecretPropertyRegistry;

// registry of secret properties: core rows plus the validated rows of the loaded plugins
// (03-secretstore-config.md §3a)
class SecretPropertyRegistryTest : public QObject
{
	Q_OBJECT
private:
	static QStringList& messages()
	{
		static QStringList list;
		return list;
	}

	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(context)
		if (type == QtCriticalMsg)
		{
			messages().append(message);
		}
	}

	static SecretProperty ldapBindPassword()
	{
		return {QStringLiteral("LDAP"), QStringLiteral("BindPassword"), QStringLiteral("ldap/bindPassword"), true};
	}

	QtMessageHandler m_previousMessageHandler = nullptr;

private Q_SLOTS:
	void init()
	{
		SecretPropertyRegistry::resetForTests();
		messages().clear();
		m_previousMessageHandler = qInstallMessageHandler(messageHandler);
	}

	void cleanup()
	{
		qInstallMessageHandler(m_previousMessageHandler);
		SecretPropertyRegistry::resetForTests();
	}

	// core has no SecretReference properties yet; whatever it registers uses reserved scopes only
	void coreProperties()
	{
		for (const auto& secretProperty : SecretPropertyRegistry::properties())
		{
			QVERIFY(SecretPropertyRegistry::isReservedScope(secretProperty.secretKey));
		}

		for (const auto& scope : {QStringLiteral("core"), QStringLiteral("ipc"), QStringLiteral("pki"), QStringLiteral("tls")})
		{
			QVERIFY(SecretPropertyRegistry::isReservedScope(scope + QStringLiteral("/key")));
		}
		QVERIFY(SecretPropertyRegistry::isReservedScope(QStringLiteral("ldap/bindPassword")) == false);
		QVERIFY(SecretPropertyRegistry::isReservedScope(QStringLiteral("tlsx/key")) == false);
	}

	void validation_data()
	{
		QTest::addColumn<QString>("parentKey");
		QTest::addColumn<QString>("key");
		QTest::addColumn<QString>("secretKey");
		QTest::addColumn<QString>("message");

		QTest::newRow("reserved scope tls") << QStringLiteral("Foo") << QStringLiteral("Key") << QStringLiteral("tls/hostKey")
											<< QStringLiteral("reserved scope");
		QTest::newRow("reserved scope pki") << QStringLiteral("Foo") << QStringLiteral("Key") << QStringLiteral("pki/caKey")
											<< QStringLiteral("reserved scope");
		QTest::newRow("reserved scope core") << QStringLiteral("Foo") << QStringLiteral("Key")
											 << QStringLiteral("core/preMigrationConfig") << QStringLiteral("reserved scope");
		QTest::newRow("reserved scope ipc") << QStringLiteral("Foo") << QStringLiteral("Key") << QStringLiteral("ipc/key")
											<< QStringLiteral("reserved scope");
		QTest::newRow("invalid key") << QStringLiteral("Foo") << QStringLiteral("Key") << QStringLiteral("../etc/shadow")
									 << QStringLiteral("invalid secret key");
		QTest::newRow("upper-case scope") << QStringLiteral("Foo") << QStringLiteral("Key") << QStringLiteral("Foo/key")
										  << QStringLiteral("invalid secret key");
		QTest::newRow("empty key") << QStringLiteral("Foo") << QStringLiteral("Key") << QString()
								   << QStringLiteral("invalid secret key");
		QTest::newRow("empty property key") << QStringLiteral("Foo") << QString() << QStringLiteral("foo/key")
											<< QStringLiteral("invalid property key");
		QTest::newRow("empty parent key") << QString() << QStringLiteral("Key") << QStringLiteral("foo/key")
										  << QStringLiteral("invalid property key");
		QTest::newRow("empty group") << QStringLiteral("Foo//Bar") << QStringLiteral("Key") << QStringLiteral("foo/key")
									 << QStringLiteral("invalid property key");
		QTest::newRow("duplicate secret key") << QStringLiteral("Foo") << QStringLiteral("Key")
											  << QStringLiteral("ldap/bindPassword") << QStringLiteral("conflicts");
		QTest::newRow("duplicate property") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
											<< QStringLiteral("foo/bindPassword") << QStringLiteral("conflicts");
		QTest::newRow("duplicate property (case)") << QStringLiteral("ldap") << QStringLiteral("bindpassword")
												   << QStringLiteral("foo/bindPassword") << QStringLiteral("conflicts");
	}

	void validation()
	{
		QFETCH(QString, parentKey);
		QFETCH(QString, key);
		QFETCH(QString, secretKey);
		QFETCH(QString, message);

		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), QStringLiteral("LDAP")));
		const auto registeredProperties = SecretPropertyRegistry::properties();

		QVERIFY(SecretPropertyRegistry::add({parentKey, key, secretKey, true}, QStringLiteral("Test")) == false);

		// first registration wins
		QCOMPARE(SecretPropertyRegistry::properties().size(), registeredProperties.size());
		QCOMPARE(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindPassword"))->secretKey,
				 QStringLiteral("ldap/bindPassword"));
		QCOMPARE(messages().size(), 1);
		QVERIFY2(messages().first().contains(message), qPrintable(messages().first()));
		QVERIFY(messages().first().contains(QStringLiteral("Test")));
	}

	// e.g. plugins loaded by a second VeyonCore instance in the same process
	void sameRowTwice()
	{
		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), QStringLiteral("LDAP")));
		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), QStringLiteral("LDAP")));
		QCOMPARE(SecretPropertyRegistry::secretKeys().count(QStringLiteral("ldap/bindPassword")), 1);
		QVERIFY(messages().isEmpty());

		// a different legacy flag is a conflict
		auto row = ldapBindPassword();
		row.isLegacyPassword = false;
		QVERIFY(SecretPropertyRegistry::add(row, QStringLiteral("LDAP")) == false);
		QCOMPARE(messages().size(), 1);
	}

	// the rows of a loaded plugin are merged in, rejected rows are logged with the plugin's name
	void pluginPropertiesMerged()
	{
		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), QStringLiteral("LDAP")));

		QPluginLoader pluginLoader(QStringLiteral(SECRET_PROPERTIES_TEST_PLUGIN_FILE_PATH));
		const auto pluginObject = PluginManager::loadPluginInstance(pluginLoader);
		QVERIFY2(pluginObject, qPrintable(pluginLoader.errorString()));
		const auto plugin = qobject_cast<PluginInterface*>(pluginObject);
		QVERIFY(plugin);

		SecretPropertyRegistry::addPluginProperties(*plugin);

		const auto clientSecret = SecretPropertyRegistry::find(QStringLiteral("TestAddOn"), QStringLiteral("ClientSecret"));
		QVERIFY(clientSecret.has_value());
		QCOMPARE(clientSecret->secretKey, QStringLiteral("test-add-on/clientSecret"));
		QVERIFY(clientSecret->isLegacyPassword);
		const auto certificateKey = SecretPropertyRegistry::find(QStringLiteral("TestAddOn"), QStringLiteral("CertificateKey"));
		QVERIFY(certificateKey.has_value());
		QVERIFY(certificateKey->isLegacyPassword == false);

		for (const auto& rejectedKey : {QStringLiteral("HostKey"), QStringLiteral("Invalid"), QStringLiteral("BindPassword")})
		{
			QVERIFY(SecretPropertyRegistry::find(QStringLiteral("TestAddOn"), rejectedKey).has_value() == false);
		}
		QCOMPARE(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindPassword"))->secretKey,
				 QStringLiteral("ldap/bindPassword"));
		QCOMPARE(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("TestAddOn")),
				 (QStringList{QStringLiteral("test-add-on/clientSecret"), QStringLiteral("test-add-on/certificateKey")}));

		QCOMPARE(messages().size(), 4);
		QCOMPARE(messages().filter(QStringLiteral("SecretPropertiesTest")).size(), 4);
	}
};


QTEST_GUILESS_MAIN(SecretPropertyRegistryTest)
#include "SecretPropertyRegistryTest.moc"
