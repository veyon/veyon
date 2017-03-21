/*
 * ConfigCommandLinePlugin.cpp - implementation of ConfigCommandLinePlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include "Configuration/LocalStore.h"
#include "ConfigCommandLinePlugin.h"


ConfigCommandLinePlugin::ConfigCommandLinePlugin() :
	m_subCommands( {
				   std::pair<QString, QString>( "clear", "clear system-wide iTALC configuration" ),
				   std::pair<QString, QString>( "list", "list all configuration keys and values" ),
				   std::pair<QString, QString>( "import", "import configuration from given file" ),
				   std::pair<QString, QString>( "export", "export configuration to given file" ),
				   } )
{
}



ConfigCommandLinePlugin::~ConfigCommandLinePlugin()
{
}



QStringList ConfigCommandLinePlugin::subCommands() const
{
	return m_subCommands.keys();
}



QString ConfigCommandLinePlugin::subCommandHelp( const QString& subCommand ) const
{
	return m_subCommands.value( subCommand );
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::runCommand( const QStringList& arguments )
{
	// all subcommands are handled as slots so if we land here an unsupported subcommand has been called
	return InvalidCommand;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_clear( const QStringList& arguments )
{
	// clear global configuration
	Configuration::LocalStore( Configuration::LocalStore::System ).clear();

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_list( const QStringList& arguments )
{
	// clear global configuration
	listConfiguration( ItalcCore::config().data(), QString() );

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_import( const QStringList& arguments )
{
	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_export( const QStringList& arguments )
{
	return Successful;
}



void ConfigCommandLinePlugin::listConfiguration( const ItalcConfiguration::DataMap &map,
												 const QString &parentKey )
{
	for( auto it = map.begin(); it != map.end(); ++it )
	{
		QString curParentKey = parentKey.isEmpty() ? it.key() : parentKey + "/" + it.key();

		if( it.value().type() == QVariant::Map )
		{
			listConfiguration( it.value().toMap(), curParentKey );
		}
		else if( it.value().type() == QVariant::String ||
				 it.value().type() == QVariant::Uuid )
		{
			QTextStream( stdout ) << curParentKey << "=" << it.value().toString() << endl;
		}
		else
		{
			qWarning() << "Key" << it.key() << "has unknown value type:" << it.value();
		}
	}
}
