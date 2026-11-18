// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "QuickConfigurationPluginInterface.h"

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"
#include "X11VncConfiguration.h"

class BuiltinX11VncServer : public QObject, VncServerPluginInterface, PluginInterface,
		public QuickConfigurationPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.BuiltinX11VncServer")
	Q_INTERFACES(QuickConfigurationPluginInterface PluginInterface VncServerPluginInterface)
public:
	QObject* createQuickConfiguration(Configuration::Object* draft, QObject* parent) override;

	explicit BuiltinX11VncServer( QObject* parent = nullptr );

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
		return QStringLiteral("BuiltinX11VncServer");
	}

	QString description() const override
	{
		return tr("Builtin VNC server (x11vnc)");
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
		return {QStringLiteral("x11")};
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

private:
	X11VncConfiguration m_configuration;
};
