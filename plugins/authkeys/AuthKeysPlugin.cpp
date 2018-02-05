/*
 * AuthKeysPlugin.cpp - implementation of AuthKeysPlugin class
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

#include "AuthKeysConfigurationPage.h"
#include "AuthKeysPlugin.h"
#include "AuthKeysManager.h"


AuthKeysPlugin::AuthKeysPlugin( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ "create", tr( "Create new authentication key pair" ) },
{ "delete", tr( "Delete authentication key" ) },
{ "list", tr( "List authentication keys" ) },
{ "import", tr( "Import public or private key" ) },
{ "export", tr( "Export public or private key" ) },
{ "extract", tr( "Extract public key from existing private key" ) },
{ "setaccessgroup", tr( "Set user group allowed to access a key" ) },
				} )
{
}



AuthKeysPlugin::~AuthKeysPlugin()
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



CommandLinePluginInterface::RunResult AuthKeysPlugin::handle_export( const QStringList& arguments )
{
	if( arguments.size() < 1 )
	{
		return NotEnoughArguments;
	}

	const auto nameAndType = arguments[0].split( '/' );
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

	printAuthKeyTableSeparatorRow();
	printAuthKeyTableRow( tr("NAME"), tr("TYPE"), tr("PAIR ID"), tr("ACCESS GROUP") );
	printAuthKeyTableSeparatorRow();

	for( int i = 0; i < tableModel.rowCount( QModelIndex() ); ++i )
	{
		printAuthKeyTableRow( authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyName ),
							  authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyType ),
							  authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnKeyPairID ),
							  authKeysTableData( tableModel, i, AuthKeysTableModel::ColumnAccessGroup ) );
	}

	printAuthKeyTableSeparatorRow();
}



QString AuthKeysPlugin::authKeysTableData( const AuthKeysTableModel& tableModel, int row, int column )
{
	return tableModel.data( tableModel.index( row, column ), Qt::DisplayRole ).toString();
}



void AuthKeysPlugin::printAuthKeyTableRow( const QString& name, const QString& type, const QString& pairId, const QString& accessGroup )
{
	printf( "|%-20s|%-7s|%-8s|%-32s|\n", qPrintable(name), qPrintable(type), qPrintable(pairId), qPrintable(accessGroup) );
}



void AuthKeysPlugin::printAuthKeyTableSeparatorRow()
{
	printAuthKeyTableRow( "--------------------", "-------", "--------", "--------------------------------" );
}



void AuthKeysPlugin::printAuthKeyList()
{
	const auto keys = AuthKeysManager().listKeys();

	for( const auto& key : keys )
	{
		print( key );
	}
}
