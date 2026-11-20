// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "Configuration/Property.h"
#include "Configuration/SecretPropertyRegistry.h"
#include "PluginInterface.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

using Configuration::SecretPropertyRegistry;

// The rows the real plugins of the build tree declare (PluginInterface::secretProperties()) and
// their SecretReference properties agree (WP26 review M2): a typo or a forgotten row in a plugin
// would otherwise only show up as a disabled secret at runtime.
class SecretPropertyPluginsTest : public QObject
{
	Q_OBJECT
private:
	VeyonCore* m_core = nullptr;

private Q_SLOTS:
	void initTestCase()
	{
		// like veyon-cli, but the system configuration is neither written nor migrated (no SecretStore)
		auto flags = VeyonCore::defaultInitialization();
		flags.setFlag(VeyonCore::InitializationFlag::WritableConfiguration, false);

		VeyonCore::setupApplicationParameters();
		m_core = new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::CLI,
							   QStringLiteral("SecretPropertyPluginsTest"), flags);
	}

	void cleanupTestCase()
	{
		delete m_core;
	}

	// every row of every loaded plugin is registered unchanged, i.e. no row was rejected
	void pluginRowsRegistered()
	{
		auto rows = 0;
		const auto pluginInterfaces = VeyonCore::pluginManager().pluginInterfaces();
		for (const auto* pluginInterface : pluginInterfaces)
		{
			const auto secretProperties = pluginInterface->secretProperties();
			for (const auto& secretProperty : secretProperties)
			{
				const auto registeredProperty = SecretPropertyRegistry::find(secretProperty.parentKey, secretProperty.key);
				QVERIFY2(registeredProperty.has_value(), qPrintable(pluginInterface->name() + QLatin1Char(' ') + secretProperty.propertyKey()));
				QCOMPARE(registeredProperty->parentKey, secretProperty.parentKey);
				QCOMPARE(registeredProperty->key, secretProperty.key);
				QCOMPARE(registeredProperty->secretKey, secretProperty.secretKey);
				QCOMPARE(registeredProperty->isLegacyPassword, secretProperty.isLegacyPassword);
				++rows;
			}
		}

		QCOMPARE(SecretPropertyRegistry::properties().size(), rows);

		// the plugins with secrets have been loaded
#if defined(VEYON_TEST_WITH_LDAP_PLUGIN)
		QCOMPARE(SecretPropertyRegistry::find(QStringLiteral("LDAP"), QStringLiteral("BindPassword"))->secretKey,
				 QStringLiteral("ldap/bindPassword"));
#endif
		QCOMPARE(SecretPropertyRegistry::find(QStringLiteral("ExternalVncServer"), QStringLiteral("Password"))->secretKey,
				 QStringLiteral("vncserver-external/password"));
	}

	// every SecretReference property resolves to its own reference, and every row has a property
	void propertiesMatchRows()
	{
		QStringList propertyKeys;
		const auto properties = VeyonCore::config().findChildren<Configuration::Property*>();
		for (const auto* property : properties)
		{
			if (property->isSecretReference() == false)
			{
				continue;
			}

			const auto reference = SecretPropertyRegistry::ownReference(*property);
			QVERIFY2(reference.isValid(), qPrintable(property->absoluteKey()));
			QCOMPARE(property->defaultValue(), Configuration::SecretReference::defaultValue(reference.key()));
			propertyKeys.append(property->parentKey() + QLatin1Char('/') + property->key());
		}

#if defined(VEYON_TEST_WITH_LDAP_PLUGIN)
		QVERIFY(propertyKeys.contains(QStringLiteral("LDAP/BindPassword")));
#endif
		QVERIFY(propertyKeys.contains(QStringLiteral("ExternalVncServer/Password")));

		for (const auto& secretProperty : SecretPropertyRegistry::properties())
		{
			QVERIFY2(propertyKeys.contains(secretProperty.propertyKey()), qPrintable(secretProperty.propertyKey()));
		}
	}
};


QTEST_GUILESS_MAIN(SecretPropertyPluginsTest)
#include "SecretPropertyPluginsTest.moc"
