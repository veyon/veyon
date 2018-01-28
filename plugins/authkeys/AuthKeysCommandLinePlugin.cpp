/*
 * AuthKeysCommandLinePlugin.cpp - implementation of AuthKeysCommandLinePlugin class
 *
 * Copyright (c) 2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "AuthKeysCommandLinePlugin.h"
#include "AuthKeysManager.h"


AuthKeysCommandLinePlugin::AuthKeysCommandLinePlugin( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ "create", tr( "Create new authentication key pair" ) },
{ "delete", tr( "Delete authentication keys" ) },
{ "list", tr( "List authentication keys" ) },
{ "import", tr( "Import public or private key" ) },
{ "export", tr( "Export public or private key" ) },
{ "extract", tr( "Extract public key from existing private key" ) },
				} )
{
}



AuthKeysCommandLinePlugin::~AuthKeysCommandLinePlugin()
{
}



QStringList AuthKeysCommandLinePlugin::commands() const
{
	return m_commands.keys();
}



QString AuthKeysCommandLinePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_create( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	AuthKeysManager manager;
	if( manager.createKeyPair( arguments.first() ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_delete( const QStringList& arguments )
{
	if( arguments.size() < 1 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments.first().split( '/' );
	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	AuthKeysManager manager;
	if( manager.deleteKey( name, type ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_export( const QStringList& arguments )
{
	if( arguments.size() < 2 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments[0].split( '/' );
	const auto outputFile = arguments[1];

	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	AuthKeysManager manager;
	if( manager.exportKey( name, type, outputFile ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_import( const QStringList& arguments )
{
	if( arguments.size() < 2 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments[0].split( '/' );
	const auto inputFile = arguments[1];

	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	AuthKeysManager manager;
	if( manager.importKey( name, type, inputFile ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_list( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	const auto keys = AuthKeysManager().listKeys();

	for( const auto& key : keys )
	{
		print( key );
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult AuthKeysCommandLinePlugin::handle_extract( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	AuthKeysManager manager;
	if( manager.extractPublicFromPrivateKey( arguments.first() ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}
