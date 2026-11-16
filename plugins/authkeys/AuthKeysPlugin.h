// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"

class AuthKeysTableModel;

class AuthKeysPlugin : public QObject,
					   CommandLinePluginInterface,
					   PluginInterface,
					   CommandLineIO,
					   ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AuthKeys")
	Q_INTERFACES(PluginInterface
					 CommandLinePluginInterface
						 ConfigurationPagePluginInterface)
public:
	explicit AuthKeysPlugin(QObject* parent = nullptr);
	~AuthKeysPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("4790bad8-4c56-40d5-8361-099a68f0c24b")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("AuthKeys");
	}

	QString description() const override
	{
		return tr("Command line support for managing authentication keys");
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
		return QStringLiteral("authkeys");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for managing authentication keys");
	}

	QStringList commands() const override;
	QString commandHelp(const QString& command) const override;

	ConfigurationPage* createConfigurationPage() override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_help(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_setaccessgroup(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_create(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_delete(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_export(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_import(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_list(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_extract(const QStringList& arguments);

private:
	static void printAuthKeyTable();
	static QString authKeysTableData(const AuthKeysTableModel& tableModel, int row, int column);
	static void printAuthKeyList();

	QMap<QString, QString> m_commands;
};
