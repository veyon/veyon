// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"
#include "FeatureProviderInterface.h"

class FeatureCommands : public QObject, CommandLinePluginInterface, PluginInterface, CommandLineIO
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit FeatureCommands(QObject* parent = nullptr);
	~FeatureCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("2376b972-0bf1-4f50-bcfd-79d4d90730c6")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("FeatureCommands");
	}

	QString description() const override
	{
		return tr("Feature-related CLI operations");
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
		return QStringLiteral("feature");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for controlling features");
	}

	QStringList commands() const override;
	QString commandHelp(const QString& command) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_help(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_list(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_show(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_start(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_stop(const QStringList& arguments);

private:
	static QString listCommand()
	{
		return QStringLiteral("list");
	}

	static QString showCommand()
	{
		return QStringLiteral("show");
	}

	static QString startCommand()
	{
		return QStringLiteral("start");
	}

	static QString stopCommand()
	{
		return QStringLiteral("stop");
	}

	RunResult controlComputer(FeatureProviderInterface::Operation operation, const QStringList& arguments);

	const QMap<QString, QString> m_commands;
};
