// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PluginCommands.h"
#include "PluginManager.h"


PluginCommands::PluginCommands(QObject* parent) :
	QObject(parent),
	m_commands({
		{QStringLiteral("list"), tr("List names of all installed plugins")},
		{QStringLiteral("show"), tr("Show table with details of all installed plugins")},
	})
{
}



QStringList PluginCommands::commands() const
{
	return m_commands.keys();
}



QString PluginCommands::commandHelp(const QString& command) const
{
	return m_commands.value(command);
}



CommandLinePluginInterface::RunResult PluginCommands::handle_list(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	const auto plugins = VeyonCore::pluginManager().pluginInterfaces();
	for (auto plugin : plugins)
	{
		print(plugin->name());
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult PluginCommands::handle_show(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	const auto plugins = VeyonCore::pluginManager().pluginInterfaces();

	TableHeader tableHeader({tr("Name"), tr("Description"), tr("Version"), tr("UID")});
	TableRows tableRows;
	tableRows.reserve(plugins.count());

	for (auto plugin : plugins)
	{
		tableRows.append({plugin->name(),
						  plugin->description(),
						  plugin->version().toString(),
						  plugin->uid().toString(QUuid::WithoutBraces)});
	}

	std::sort(tableRows.begin(), tableRows.end(), [](const TableRow& a, const TableRow& b) {
		return a.first() < b.first();
	});

	printTable(Table(tableHeader, tableRows));

	return NoResult;
}
