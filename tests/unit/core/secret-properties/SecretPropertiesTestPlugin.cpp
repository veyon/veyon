// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QObject>

#include "PluginInterface.h"

// plugin declaring secret properties like an add-on, with some rows the registry must reject
class SecretPropertiesTestPlugin : public QObject, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.SecretPropertiesTest")
	Q_INTERFACES(PluginInterface)
public:
	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("5c2e7d0a-6f43-4b8e-9a61-2f0d3b9c8e14")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("SecretPropertiesTest");
	}

	QString description() const override
	{
		return {};
	}

	QString vendor() const override
	{
		return {};
	}

	QString copyright() const override
	{
		return {};
	}

	Configuration::SecretPropertyList secretProperties() const override
	{
		return {
			{QStringLiteral("TestAddOn"), QStringLiteral("ClientSecret"), QStringLiteral("test-add-on/clientSecret"), true},
			{QStringLiteral("TestAddOn"), QStringLiteral("CertificateKey"), QStringLiteral("test-add-on/certificateKey"), false},
			{QStringLiteral("TestAddOn"), QStringLiteral("HostKey"), QStringLiteral("tls/hostKey"), false},
			{QStringLiteral("TestAddOn"), QStringLiteral("Invalid"), QStringLiteral("../etc/shadow"), false},
			{QStringLiteral("TestAddOn"), QStringLiteral("BindPassword"), QStringLiteral("ldap/bindPassword"), true},
			{QStringLiteral("LDAP"), QStringLiteral("BindPassword"), QStringLiteral("test-add-on/bindPassword"), true},
		};
	}
};

#include "SecretPropertiesTestPlugin.moc"
