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

	static inline const Plugin::Uid LdapUid{QStringLiteral("6f0a491e-c1c6-4338-8244-f823b0bf8670")};
	static inline const Plugin::Uid TestUid{QStringLiteral("2a7d4c91-5e3b-4f08-b6a2-9c1e8d0f7b35")};

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
	}

	void validation()
	{
		QFETCH(QString, parentKey);
		QFETCH(QString, key);
		QFETCH(QString, secretKey);
		QFETCH(QString, message);

		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")));
		const auto registeredProperties = SecretPropertyRegistry::properties();

		QVERIFY(SecretPropertyRegistry::add({parentKey, key, secretKey, true}, TestUid, QStringLiteral("Test")) == false);

		// an invalid row doesn't affect others
		QCOMPARE(SecretPropertyRegistry::properties().size(), registeredProperties.size());
		QCOMPARE(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindPassword"))->secretKey,
				 QStringLiteral("ldap/bindPassword"));
		QCOMPARE(messages().size(), 1);
		QVERIFY2(messages().first().contains(message), qPrintable(messages().first()));
		QVERIFY(messages().first().contains(QStringLiteral("Test")));
	}

	void conflicts_data()
	{
		QTest::addColumn<QString>("parentKey");
		QTest::addColumn<QString>("key");
		QTest::addColumn<QString>("secretKey");
		QTest::addColumn<bool>("isLegacyPassword");
		QTest::addColumn<bool>("sameProvider");

		QTest::newRow("same secret key") << QStringLiteral("Foo") << QStringLiteral("Key")
										 << QStringLiteral("ldap/bindPassword") << true << false;
		QTest::newRow("same secret key (case)") << QStringLiteral("Foo") << QStringLiteral("Key")
												<< QStringLiteral("ldap/bindpassword") << true << false;
		QTest::newRow("same property") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
									   << QStringLiteral("foo/bindPassword") << true << false;
		QTest::newRow("same property (case)") << QStringLiteral("ldap") << QStringLiteral("bindpassword")
											  << QStringLiteral("foo/bindPassword") << true << false;
		QTest::newRow("same property and key (case)") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
													  << QStringLiteral("ldap/bindpassword") << true << false;
		QTest::newRow("other legacy flag") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
										   << QStringLiteral("ldap/bindPassword") << false << false;
		QTest::newRow("identical row of another plugin") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
														 << QStringLiteral("ldap/bindPassword") << true << false;
		QTest::newRow("other legacy flag, same plugin") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
														<< QStringLiteral("ldap/bindPassword") << false << true;
		QTest::newRow("other case, same plugin") << QStringLiteral("LDAP") << QStringLiteral("BindPassword")
												 << QStringLiteral("ldap/bindpassword") << true << true;
	}

	// conflicting rows are all rejected (fail closed), independent of the order of the plugins
	void conflicts()
	{
		QFETCH(QString, parentKey);
		QFETCH(QString, key);
		QFETCH(QString, secretKey);
		QFETCH(bool, isLegacyPassword);
		QFETCH(bool, sameProvider);

		const SecretProperty conflictingRow{parentKey, key, secretKey, isLegacyPassword};
		const auto conflictingUid = sameProvider ? LdapUid : TestUid;
		const auto conflictingName = sameProvider ? QStringLiteral("LDAP") : QStringLiteral("Test");

		for (const auto ldapFirst : {true, false})
		{
			SecretPropertyRegistry::resetForTests();
			messages().clear();

			const auto otherRow = SecretProperty{QStringLiteral("Other"), QStringLiteral("Key"), QStringLiteral("other/key"), false};
			QVERIFY(SecretPropertyRegistry::add(otherRow, TestUid, QStringLiteral("Test")));

			if (ldapFirst)
			{
				QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")));
				QVERIFY(SecretPropertyRegistry::add(conflictingRow, conflictingUid, conflictingName) == false);
			}
			else
			{
				QVERIFY(SecretPropertyRegistry::add(conflictingRow, conflictingUid, conflictingName));
				QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")) == false);
			}

			// none of them, unrelated rows stay
			QCOMPARE(SecretPropertyRegistry::properties().size(), 1);
			QCOMPARE(SecretPropertyRegistry::properties().first().secretKey, otherRow.secretKey);
			QVERIFY(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindPassword")).has_value() == false);
			QVERIFY(SecretPropertyRegistry::find(parentKey, key).has_value() == false);

			// both providers are logged
			QCOMPARE(messages().size(), 2);
			for (const auto& message : messages())
			{
				QVERIFY2(message.contains(QStringLiteral("conflicts")), qPrintable(message));
				QVERIFY2(message.contains(LdapUid.toString(QUuid::WithoutBraces)), qPrintable(message));
				QVERIFY2(message.contains(conflictingUid.toString(QUuid::WithoutBraces)), qPrintable(message));
			}

			// re-adding either row later doesn't bring it back
			QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")) == false);
			QVERIFY(SecretPropertyRegistry::add(conflictingRow, conflictingUid, conflictingName) == false);
			QCOMPARE(SecretPropertyRegistry::properties().size(), 1);
		}
	}

	// a row which conflicts with one rejected before is rejected as well, so that the result of a
	// chain of conflicts does not depend on the order either
	void conflictChain()
	{
		const SecretProperty first{QStringLiteral("A"), QStringLiteral("Key"), QStringLiteral("a/key"), false};
		const SecretProperty second{QStringLiteral("B"), QStringLiteral("Key"), QStringLiteral("a/key"), false};
		const SecretProperty third{QStringLiteral("B"), QStringLiteral("Key"), QStringLiteral("c/key"), false};
		const SecretProperty fourth{QStringLiteral("D"), QStringLiteral("Key"), QStringLiteral("c/key"), false};
		const QList<SecretProperty> rows{first, second, third, fourth};

		for (const auto& order : QList<QList<int>>{{0, 1, 2, 3}, {3, 2, 1, 0}, {0, 3, 1, 2}, {2, 0, 3, 1}})
		{
			SecretPropertyRegistry::resetForTests();
			for (const auto index : order)
			{
				(void)SecretPropertyRegistry::add(rows.at(index), Plugin::Uid::createUuid(), QStringLiteral("Plugin"));
			}
			QVERIFY(SecretPropertyRegistry::properties().isEmpty());
		}
	}

	// e.g. plugins loaded by a second VeyonCore instance in the same process
	void sameRowTwice()
	{
		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")));
		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")));
		QCOMPARE(SecretPropertyRegistry::secretKeys().count(QStringLiteral("ldap/bindPassword")), 1);
		QVERIFY(messages().isEmpty());
	}

	// the rows are copies which remain valid when the plugin's library is unloaded
	void rowsSurvivePluginUnload()
	{
		{
			QPluginLoader pluginLoader(QStringLiteral(SECRET_PROPERTIES_TEST_PLUGIN_FILE_PATH));
			const auto plugin = qobject_cast<PluginInterface*>(PluginManager::loadPluginInstance(pluginLoader));
			QVERIFY(plugin);
			SecretPropertyRegistry::addPluginProperties(*plugin);
			(void)pluginLoader.unload();
		}

		const auto clientSecret = SecretPropertyRegistry::find(QStringLiteral("TestAddOn"), QStringLiteral("ClientSecret"));
		QVERIFY(clientSecret.has_value());
		QCOMPARE(clientSecret->secretKey, QStringLiteral("test-add-on/clientSecret"));
		// no static data of the plugin's QStringLiterals
		for (const auto& secretProperty : SecretPropertyRegistry::properties())
		{
			QVERIFY(secretProperty.parentKey.data_ptr().isMutable());
			QVERIFY(secretProperty.key.data_ptr().isMutable());
			QVERIFY(secretProperty.secretKey.data_ptr().isMutable());
		}
	}

	// the rows of a loaded plugin are merged in, rejected rows are logged with the plugin's name;
	// its rows conflicting with the LDAP plugin's row disable that as well (fail closed)
	void pluginPropertiesMerged()
	{
		QVERIFY(SecretPropertyRegistry::add(ldapBindPassword(), LdapUid, QStringLiteral("LDAP")));

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
		QVERIFY(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindPassword")).has_value() == false);
		QCOMPARE(SecretPropertyRegistry::secretKeysBelow(QStringLiteral("TestAddOn")),
				 (QStringList{QStringLiteral("test-add-on/clientSecret"), QStringLiteral("test-add-on/certificateKey")}));

		// HostKey, Invalid, TestAddOn/BindPassword and the LDAP row, LDAP/BindPassword
		QCOMPARE(messages().size(), 5);
		QCOMPARE(messages().filter(QStringLiteral("SecretPropertiesTest")).size(), 5);
		QCOMPARE(messages().filter(QStringLiteral("5c2e7d0a-6f43-4b8e-9a61-2f0d3b9c8e14")).size(), 5);
	}
};


QTEST_GUILESS_MAIN(SecretPropertyRegistryTest)
#include "SecretPropertyRegistryTest.moc"
