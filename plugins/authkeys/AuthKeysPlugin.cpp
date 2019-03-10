/*
 * AuthKeysPlugin.cpp - implementation of AuthKeysPlugin class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthKeysConfigurationPage.h"
#include "AuthKeysPlugin.h"
#include "AuthKeysManager.h"


AuthKeysPlugin::AuthKeysPlugin( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ QStringLiteral("create"), tr( "Create new authentication key pair" ) },
{ QStringLiteral("delete"), tr( "Delete authentication key" ) },
{ QStringLiteral("list"), tr( "List authentication keys" ) },
{ QStringLiteral("import"), tr( "Import public or private key" ) },
{ QStringLiteral("export"), tr( "Export public or private key" ) },
{ QStringLiteral("extract"), tr( "Extract public key from existing private key" ) },
{ QStringLiteral("setaccessgroup"), tr( "Set user group allowed to access a key" ) },
				} )
{
}



QStringList AuthKeysPlugin::commands() const
{
	return m_commands.keys();
}



QString AuthKeysPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



ConfigurationPage* AuthKeysPlugin::createConfigurationPage()
{
	return new AuthKeysConfigurationPage();
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );

	const QMap<QString, QStringList> commands = {
		{ QStringLiteral("create"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("NAME") ),
						 tr( "This command creates a new authentication key pair with name <NAME> and saves private and "
						 "public key to the configured key directories. The parameter must be a name for the key, which "
						 "may only contain letters." ) } ) },
		{ QStringLiteral("delete"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("KEY") ),
						 tr( "This command deletes the authentication key <KEY> from the configured key directory. "
						 "Please note that a key can't be recovered once it has been deleted." ) } ) },
		{ QStringLiteral("export"),
		  QStringList( { QStringLiteral("<%1> [<%2>]").arg( tr("KEY"), tr("FILE") ),
						 tr( "This command exports the authentication key <KEY> to <FILE>. "
						 "If <FILE> is not specified a name will be constructed from name and type of <KEY>." ) } ) },
		{ QStringLiteral("extract"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("KEY") ),
						 tr( "This command extracts the public key part from the private key <KEY> and saves it as the "
						 "corresponding public key. When setting up another master computer, it is therefore sufficient "
						 "to transfer the private key only. The public key can then be extracted." ) } ) },
		{ QStringLiteral("import"),
		  QStringList( { QStringLiteral("<%1> [<%2>]").arg( tr( "KEY" ), tr( "FILE" ) ),
						 tr( "This command imports the authentication key <KEY> from <FILE>. "
						 "If <FILE> is not specified a name will be constructed from name and type of <KEY>." ) } ) },
		{ QStringLiteral("list"),
		  QStringList( { QStringLiteral("[details]"),
						 tr( "This command lists all available authentication keys in the configured key directory. "
						 "If the option \"%1\" is specified a table with key details will be displayed instead. "
						 "Some details might be missing if a key is not accessible e.g. due to the lack of read permissions." ).arg( QLatin1String("details") ) } ) },
		{ QStringLiteral("setaccessgroup"),
		  QStringList( { QStringLiteral("<%1> <%2>").arg( tr("KEY"), tr("ACCESS GROUP") ),
						 tr( "This command adjusts file access permissions to <KEY> such that only the "
						 "user group <ACCESS GROUP> has read access to it." ) } ) },
	};

	if( commands.contains( command ) )
	{
		const auto& helpString = commands[command];
		print( QStringLiteral("\n%1 %2\n\n%3\n\n").arg( command, helpString[0], helpString[1] ) );

		return NoResult;
	}

	print( tr("Please specify the command to display help for!") );

	return Unknown;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_setaccessgroup( const QStringList& arguments )
{
	if( arguments.size() < 2 )
	{
		return NotEnoughArguments;
	}

	const auto key = arguments[0];
	const auto accessGroup = arguments[1];

	AuthKeysManager manager;
	if( manager.setAccessGroup( key, accessGroup ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_create( const QStringList& arguments )
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



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_delete( const QStringList& arguments )
{
	if( arguments.size() < 1 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments.first().split( QLatin1Char('/') );
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



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_export( const QStringList& arguments )
{
	if( arguments.size() < 1 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments[0].split( QLatin1Char('/') );
	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	auto outputFile = arguments.value( 1 );

	if( outputFile.isEmpty() )
	{
		outputFile = AuthKeysManager::exportedKeyFileName( name, type );
	}

	AuthKeysManager manager;
	if( manager.exportKey( name, type, outputFile ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_import( const QStringList& arguments )
{
	if( arguments.size() < 1 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments[0].split( QLatin1Char('/') );
	const auto name = nameAndType.value( 0 );
	const auto type = nameAndType.value( 1 );

	auto inputFile = arguments.value( 1 );

	if( inputFile.isEmpty() )
	{
		inputFile = AuthKeysManager::exportedKeyFileName( name, type );
	}

	AuthKeysManager manager;
	if( manager.importKey( name, type, inputFile ) == false )
	{
		error( manager.resultMessage() );

		return Failed;
	}

	info( manager.resultMessage() );

	return Successful;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_list( const QStringList& arguments )
{
	if( arguments.value( 0 ) == QStringLiteral("details") )
	{
		printAuthKeyTable();
	}
	else
	{
		printAuthKeyList();
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_extract( const QStringList& arguments )
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



void AuthKeysPlugin::printAuthKeyTable()
{
	AuthKeysTableModel tableModel;
	tableModel.reload();

	CommandLineIO::TableHeader tableHeader( { tr("NAME"), tr("TYPE"), tr("PAIR ID"), tr("ACCESS GROUP") } );
	CommandLineIO::TableRows tableRows;

	tableRows.reserve( tableModel.rowCount() );

	for( int i = 0; i < tableModel.rowCount(); ++i )
	{
		tableRows.append( { authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyName ),
							  authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyType ),
							  authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyPairID ),
							  authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnAccessGroup ) } );
	}

	CommandLineIO::printTable( CommandLineIO::Table( tableHeader, tableRows ) );
}



QString AuthKeysPlugin::authKeysTableData( const AuthKeysTableModel& tableModel, int row, int column )
{
	return tableModel.data( tableModel.index( row, column ), Qt::DisplayRole ).toString();
}



void AuthKeysPlugin::printAuthKeyList()
{
	const auto keys = AuthKeysManager().listKeys();

	for( const auto& key : keys )
	{
		print( key );
	}
}
