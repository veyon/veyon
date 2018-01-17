/*
 * ConfigCommandLinePlugin.cpp - implementation of ConfigCommandLinePlugin class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include "BuiltinDirectoryConfigurationPage.h"
#include "BuiltinDirectory.h"
#include "BuiltinDirectoryPlugin.h"
#include "ConfigurationManager.h"


BuiltinDirectoryPlugin::BuiltinDirectoryPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration(),
	m_commands( {
{ "help", tr( "Show help for specific command" ) },
{ "clear", tr( "Clear all rooms and computers" ) },
{ "list", tr( "List all rooms and computers" ) },
{ "import", tr( "Import objects from given file" ) },
{ "export", tr( "Export objects to given file" ) },
				} )
{
}



BuiltinDirectoryPlugin::~BuiltinDirectoryPlugin()
{
}



void BuiltinDirectoryPlugin::upgrade( const QVersionNumber& oldVersion )
{
	if( oldVersion < QVersionNumber( 1, 1 ) &&
			m_configuration.localDataNetworkObjects().isEmpty() == false )
	{
		m_configuration.setNetworkObjects( m_configuration.localDataNetworkObjects() );
		m_configuration.setLocalDataNetworkObjects( QJsonArray() );
	}
}



NetworkObjectDirectory *BuiltinDirectoryPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new BuiltinDirectory( m_configuration, parent );
}



ConfigurationPage *BuiltinDirectoryPlugin::createConfigurationPage()
{
	return new BuiltinDirectoryConfigurationPage( m_configuration );
}



QStringList BuiltinDirectoryPlugin::commands() const
{
	return m_commands.keys();
}



QString BuiltinDirectoryPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );

	if( command == QStringLiteral("import") )
	{
		printf( "\nimport <file> [-d <delimiter>] [-r <room column>] [-n <name column>] "
				"[-h <host address column>] [-m <MAC address column>]\n\n" );

		return NoResult;
	}

	return InvalidArguments;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_clear( const QStringList& arguments )
{
	m_configuration.setNetworkObjects( {} );
	ConfigurationManager().saveConfiguration();

	return Successful;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_list( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		listObjects( m_configuration.networkObjects(), NetworkObject::None );
	}
	else
	{
		const auto parents = BuiltinDirectory( m_configuration, this ).queryObjects( NetworkObject::Group, arguments.first() );

		for( const auto& parent : parents )
		{
			listObjects( m_configuration.networkObjects(), parent );
		}
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_import( const QStringList& arguments )
{
	return Successful;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_export( const QStringList& arguments )
{
	return Successful;
}



void BuiltinDirectoryPlugin::listObjects( const QJsonArray& objects, const NetworkObject& parent )
{
	for( const auto& networkObjectValue : objects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( parent.type() == NetworkObject::None ||
				networkObject.parentUid() == parent.uid() )
		{
			printf( "%s\n", qUtf8Printable( dumpNetworkObject( networkObject ) ) );
			listObjects( objects, networkObject );
		}
	}
}



QString BuiltinDirectoryPlugin::dumpNetworkObject( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Group:
		return tr( "Room \"%1\"" ).arg( object.name() );
	case NetworkObject::Host:
		return QChar('\t') +
				tr( "Computer \"%1\" (host address: \"%2\" MAC address:\"%3\")" ).
				arg( object.name() ).
				arg( object.hostAddress() ).
				arg( object.macAddress() );
	default:
		break;
	}

	return tr( "Unclassified object \"%1\" with ID \"%2\"" ).arg( object.name() ).arg( object.uid().toString() );
}
