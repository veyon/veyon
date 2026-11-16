// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "UltraVncConfiguration.h"
#include "VncServerPluginInterface.h"

class LogoffEventFilter;

class BuiltinUltraVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.BuiltinUltraVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	BuiltinUltraVncServer();
	~BuiltinUltraVncServer() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("39d7a07f-94db-4912-aa1a-c4df8aee3879")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("BuiltinUltraVncServer");
	}

	QString description() const override
	{
		return tr("Builtin VNC server (UltraVNC)");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QStringList supportedSessionTypes() const override
	{
		return {QStringLiteral("console"), QStringLiteral("rdp")};
	}

	QWidget* configurationWidget() override;

	void prepareServer() override;

	bool runServer(int serverPort, const Password& password) override;

	int configuredServerPort() override
	{
		return -1;
	}

	Password configuredPassword() override
	{
		return {};
	}

	const UltraVncConfiguration& configuration() const
	{
		return m_configuration;
	}

	int serverPort() const
	{
		return m_serverPort;
	}

	const Password& password() const
	{
		return m_password;
	}

private:
	UltraVncConfiguration m_configuration;

	static constexpr auto DefaultServerPort = 5900;

	int m_serverPort;
	Password m_password;

	LogoffEventFilter* m_logoffEventFilter;
};
