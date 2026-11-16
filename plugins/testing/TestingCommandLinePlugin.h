// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLinePluginInterface.h"
#include "VeyonConfiguration.h"

class TestingCommandLinePlugin : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.TestingCommandLineInterface")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit TestingCommandLinePlugin(QObject* parent = nullptr);
	~TestingCommandLinePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("a8a84654-40ca-4731-811e-7e05997ed081")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("Testing");
	}

	QString description() const override
	{
		return tr("Test internal Veyon components and functions");
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
		return QStringLiteral("testing");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for testing internal components and functions of Veyon");
	}

	QStringList commands() const override;
	QString commandHelp(const QString& command) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_checkaccess(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_authorizedgroups(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_accesscontrolrules(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_isaccessdeniedbylocalstate(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_ping(const QStringList& arguments);

private:
	QMap<QString, QString> m_commands;
};
