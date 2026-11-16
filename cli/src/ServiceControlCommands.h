// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLinePluginInterface.h"

class ServiceControlCommands : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit ServiceControlCommands(QObject* parent = nullptr);
	~ServiceControlCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("b47bcae0-24ff-4bf5-869c-484d64af5c4c")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("ServiceControl");
	}

	QString description() const override
	{
		return tr("Configure and control Veyon service");
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
		return QStringLiteral("service");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for configuring and controlling Veyon Service");
	}

	QStringList commands() const override
	{
		return m_commands.keys();
	}

	QString commandHelp(const QString& command) const override
	{
		return m_commands.value(command);
	}

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_register(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_unregister(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_start(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_stop(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_restart(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_status(const QStringList& arguments);

private:
	QMap<QString, QString> m_commands;
};
