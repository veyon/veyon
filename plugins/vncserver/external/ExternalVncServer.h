// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ExternalVncServerConfiguration.h"
#include "PluginInterface.h"
#include "VncServerPluginInterface.h"

class ExternalVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ExternalVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	explicit ExternalVncServer(QObject* parent = nullptr);

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("67dfc1c1-8f37-4539-a298-16e74e34fd8b")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("ExternalVncServer");
	}

	QString description() const override
	{
		return tr("External VNC server");
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
		return Plugin::NoFlags;
	}

	QStringList supportedSessionTypes() const override
	{
		return {};
	}

	void upgrade(const QVersionNumber& oldVersion) override;

	QWidget* configurationWidget() override;

	void prepareServer() override;

	bool runServer(int serverPort, const Password& password) override;

	int configuredServerPort() override;

	Password configuredPassword() override;

private:
	enum {
		MaximumPlaintextPasswordLength = 64
	};

	ExternalVncServerConfiguration m_configuration;
};
