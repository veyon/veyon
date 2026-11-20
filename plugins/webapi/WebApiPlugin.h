// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QThread>

#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "WebApiConfiguration.h"

class WebApiHttpServer;

class WebApiPlugin : public QObject, PluginInterface, CommandLinePluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.WebApi")
	Q_INTERFACES(PluginInterface
					 CommandLinePluginInterface
						 ConfigurationPagePluginInterface)
public:
	explicit WebApiPlugin(QObject* parent = nullptr);
	~WebApiPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("e11bee03-b99c-465c-bf90-7e5339b83f6b")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("WebAPI");
	}

	QString description() const override
	{
		return tr("Provide access to a computer via HTTP");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	QString commandLineModuleName() const override
	{
		return QStringLiteral("webapi");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for running the WebAPI server");
	}

	QStringList commands() const override
	{
		return m_commands.keys();
	}

	QString commandHelp(const QString& command) const override
	{
		return m_commands.value(command);
	}

	ConfigurationPage* createConfigurationPage() override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_help(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_runserver(const QStringList& arguments);

private:
	void startHttpServerThread();

	static constexpr auto ServerThreadTerminationTimeout = 1000;

	WebApiConfiguration m_configuration;

	QThread m_httpServerThread;
	WebApiHttpServer* m_httpServer{nullptr};

	const QMap<QString, QString> m_commands;
};
