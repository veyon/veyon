// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"

class PluginCommands : public QObject, CommandLinePluginInterface, PluginInterface, CommandLineIO
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit PluginCommands(QObject* parent = nullptr);
	~PluginCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("cbea24b5-4f6b-446d-8f54-4f98ec796a8c")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("Plugin");
	}

	QString description() const override
	{
		return tr("Plugin-related CLI operations");
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
		return QStringLiteral("plugin");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for managing plugins");
	}

	QStringList commands() const override;
	QString commandHelp(const QString& command) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_list(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_show(const QStringList& arguments);

private:
	const QMap<QString, QString> m_commands;
};
