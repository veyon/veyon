// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLinePluginInterface.h"

class ShellCommands : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit ShellCommands(QObject* parent = nullptr);
	~ShellCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("85f6c631-e75a-4c78-8cb2-a7f3f502015a")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("Shell");
	}

	QString description() const override
	{
		return tr("Interactive shell and script execution for Veyon CLI");
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
		return QStringLiteral("shell");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for shell functionalities");
	}

	QStringList commands() const override;
	QString commandHelp(const QString& command) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_main();
	CommandLinePluginInterface::RunResult handle_run(const QStringList& arguments);

private:
	void runCommand(const QString& command);

	QMap<QString, QString> m_commands;
};
