/*
 * PluginsCommands.cpp - implementation of PluginsCommands class
 *
 * Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "PluginCommands.h"
#include "PluginManager.h"


PluginCommands::PluginCommands( QObject* parent ) :
	QObject( parent ),
	m_commands( {
		{ QStringLiteral("list"), tr( "List names of all installed plugins" ) },
		{ QStringLiteral("show"), tr( "Show table with details of all installed plugins" ) },
		} )
{
}



QStringList PluginCommands::commands() const
{
	return m_commands.keys();
}



QString PluginCommands::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult PluginCommands::handle_list( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	const auto plugins = VeyonCore::pluginManager().pluginInterfaces();
	for( auto plugin : plugins )
	{
		print( plugin->name() );
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult PluginCommands::handle_show( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	const auto plugins = VeyonCore::pluginManager().pluginInterfaces();

	TableHeader tableHeader( { tr("Name"), tr("Description"), tr("Version"), tr("UID") } );
	TableRows tableRows;
	tableRows.reserve( plugins.count() );

	for( auto plugin : plugins )
	{
		tableRows.append( {
			plugin->name(),
			plugin->description(),
			plugin->version().toString(),
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
				plugin->uid().toString( QUuid::WithoutBraces )
#else
				VeyonCore::formattedUuid( plugin->uid().toString() )
#endif
		} );
	}

	std::sort( tableRows.begin(), tableRows.end(), []( const TableRow& a, const TableRow& b ) {
		return a.first() < b.first();
	}) ;

	printTable( Table( tableHeader, tableRows ) );

	return NoResult;
}
