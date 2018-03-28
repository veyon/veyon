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

#include <QFile>

#include "BuiltinDirectoryConfigurationPage.h"
#include "BuiltinDirectory.h"
#include "BuiltinDirectoryPlugin.h"
#include "CommandLineIO.h"
#include "ConfigurationManager.h"
#include "ObjectManager.h"


BuiltinDirectoryPlugin::BuiltinDirectoryPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration(),
	m_commands( {
{ "help", tr( "Show help for specific command" ) },
{ "clear", tr( "Clear all rooms and computers" ) },
{ "dump", tr( "Dump all rooms and computers" ) },
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
		CommandLineIO::print( tr("\nUSAGE\n\n%1 import <file> [room <room>] [format <format-string-with-variables>] "
								 "[regex <regular-expression-with-variables>]\n\n"
								 "Valid variables: %name% %host% %mac% %room%\n\n"
								 "Examples:\n\n"
								 "* Import simple CSV file to a single room:\n\n"
								 "    %1 import computers.csv room \"Room 01\" format \"%name%;%host%;%mac%\"\n\n"
								 "* Import CSV file with room name in first column:\n\n"
								 "    %1 import computers-with-rooms.csv format \"%room%,%name%,%mac%\"\n\n"
								 "* Import text file with with key/value pairs using regular expressions:\n\n"
								 "    %1 import hostlist.txt room \"Room 01\" regex \"^NAME:(%name%:.*)\\s+HOST:(%host%:.*)$\"\n\n"
								 "* Import arbitrarily formatted data:\n\n"
								 "    %1 import data.txt regex '^\"(%room%:[^\"]+)\";\"(%host%:[a-z\\d\\.]+)\".*$'\n").
							  arg( commandLineModuleName() ) );

		return NoResult;
	}

	return InvalidArguments;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_clear( const QStringList& arguments )
{
	m_configuration.setNetworkObjects( {} );

	ConfigurationManager configurationManager;
	if( configurationManager.saveConfiguration() == false )
	{
		CommandLineIO::error( configurationManager.errorString() );
		return Failed;
	}

	return Successful;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_dump( const QStringList& arguments )
{
	Q_UNUSED(arguments);

	CommandLineIO::TableHeader tableHeader( { tr("Object UID"), tr("Parent UID"), tr("Type"), tr("Name"), tr("Host address"), tr("MAC address") } );
	CommandLineIO::TableRows tableRows;

	const auto objects = m_configuration.networkObjects();

	tableRows.reserve( objects.size() );

	for( const auto& networkObjectValue : objects )
	{
		tableRows.append( dumpNetworkObject( networkObjectValue.toObject() ) );
	}

	CommandLineIO::printTable( CommandLineIO::Table( tableHeader, tableRows ) );

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
	if( arguments.count() < 3 )
	{
		return NotEnoughArguments;
	}

	const auto inputFileName = arguments.first();
	QFile inputFile( inputFileName );

	if( inputFile.exists() == false )
	{
		CommandLineIO::error( tr( "File \"%1\" does not exist!" ).arg( inputFileName ) );
		return Failed;
	}
	else if( inputFile.open( QFile::ReadOnly | QFile::Text ) == false )
	{
		CommandLineIO::error( tr( "Can't open file \"%1\" for reading!" ).arg( inputFileName ) );
		return Failed;
	}

	QString room;
	QString formatString;
	QString regularExpression;

	for( int i = 1; i < arguments.count(); i += 2 )
	{
		if( i+1 >= arguments.count() )
		{
			return NotEnoughArguments;
		}

		const auto key = arguments[i];
		const auto value = arguments[i+1];
		if( key == QStringLiteral("room") )
		{
			room = value;
		}
		else if( key == QStringLiteral("format") )
		{
			formatString = value;
		}
		else if( key == QStringLiteral("regex") )
		{
			regularExpression = value;
		}
		else
		{
			CommandLineIO::error( tr( "Unknown argument \"%1\"." ).arg( key ) );
			return InvalidArguments;
		}
	}

	if( formatString.isEmpty() == false )
	{
		regularExpression = formatString;

		for( const auto& var : fileImportVariables() )
		{
			regularExpression.replace( var, QStringLiteral("(%1:[^\\n\\r]*)").arg( var ) );
		}
	}

	if( regularExpression.isEmpty() == false )
	{
		if( importFile( inputFile, regularExpression, room ) )
		{
			ConfigurationManager configurationManager;

			if( configurationManager.saveConfiguration() )
			{
				return Successful;
			}

			CommandLineIO::error( configurationManager.errorString() );
			return Failed;
		}

		return Failed;
	}

	CommandLineIO::error( "No format string or regular expression specified!" );

	return InvalidArguments;
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

		if( ( parent.type() == NetworkObject::None && networkObject.parentUid().isNull() ) ||
				networkObject.parentUid() == parent.uid() )
		{
			printf( "%s\n", qUtf8Printable( listNetworkObject( networkObject ) ) );
			listObjects( objects, networkObject );
		}
	}
}



QStringList BuiltinDirectoryPlugin::dumpNetworkObject( const NetworkObject& object )
{
	return { VeyonCore::formattedUuid( object.uid() ),
				VeyonCore::formattedUuid( object.parentUid() ),
				networkObjectTypeName( object ),
				object.name(),
				object.hostAddress(),
				object.macAddress() };
}



QString BuiltinDirectoryPlugin::listNetworkObject( const NetworkObject& object )
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



QString BuiltinDirectoryPlugin::networkObjectTypeName( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::None: return tr( "None" );
	case NetworkObject::Group: return tr( "Room" );
	case NetworkObject::Host: return tr( "Computer" );
	case NetworkObject::Root: return tr( "Root");
	default:
		break;
	}

	return tr( "Invalid" );
}



bool BuiltinDirectoryPlugin::importFile( QFile& inputFile,
										 const QString& regExWithVariables,
										 const QString& room )
{
	int lineCount = 0;
	QMap<QString, QList<NetworkObject> > networkObjects;
	while( inputFile.atEnd() == false )
	{
		++lineCount;

		QString targetRoom = room;

		const auto line = inputFile.readLine();
		const auto networkObject = toNetworkObject( line, regExWithVariables, targetRoom );

		if( networkObject.isValid() )
		{
			networkObjects[targetRoom].append( networkObject );
		}
		else
		{
			CommandLineIO::error( tr( "Error while parsing line %1." ).arg( lineCount ) );
			return false;
		}
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

	for( auto it = networkObjects.constBegin(), end = networkObjects.constEnd(); it != end; ++it )
	{
		auto parentRoom = objectManager.findByName( it.key() );
		if( parentRoom.isValid() == false )
		{
			parentRoom = NetworkObject( NetworkObject::Group, it.key() );
			objectManager.add( parentRoom );
		}

		for( const NetworkObject& networkObject : qAsConst(it.value()) )
		{
			objectManager.add( NetworkObject( networkObject.type(),
											  networkObject.name(),
											  networkObject.hostAddress(),
											  networkObject.macAddress(),
											  QString(), NetworkObject::Uid(),
											  parentRoom.uid() ) );
		}
	}

	m_configuration.setNetworkObjects( objectManager.objects() );

	return true;
}



NetworkObject BuiltinDirectoryPlugin::toNetworkObject( const QString& line, const QString& regExWithVariables, QString& room )
{
	QStringList variables;
	QRegExp varDetectionRX( "\\((%\\w+%):[^)]+\\)" );
	int pos = 0;

	while( ( pos = varDetectionRX.indexIn( regExWithVariables, pos ) ) != -1 )
	{
		variables.append( varDetectionRX.cap(1) );
		pos += varDetectionRX.matchedLength();
	}

	QString rxString = regExWithVariables;
	for( const auto& var : qAsConst(variables) )
	{
		rxString.replace( QStringLiteral("%1:").arg( var ), QString() );
	}

	QRegExp rx( rxString );
	if( rx.indexIn( line ) != -1 )
	{
		const auto roomIndex = variables.indexOf( QStringLiteral("%room%") );
		const auto nameIndex = variables.indexOf( QStringLiteral("%name%") );
		const auto hostIndex = variables.indexOf( QStringLiteral("%host%") );
		const auto macIndex = variables.indexOf( QStringLiteral("%mac%") );

		if( room.isEmpty() && roomIndex != -1 )
		{
			room = rx.cap( 1 + roomIndex ).trimmed();
		}
		auto name = ( nameIndex != -1 ) ? rx.cap( 1 + nameIndex ).trimmed() : QString();
		auto host = ( hostIndex != -1 ) ? rx.cap( 1 + hostIndex ).trimmed() : QString();
		auto mac = ( macIndex != -1 ) ? rx.cap( 1 + macIndex ).trimmed() : QString();

		if( host.isEmpty() )
		{
			host = name;
		}
		else if( name.isEmpty() )
		{
			name = host;
		}
		return NetworkObject( NetworkObject::Host, name, host, mac );
	}

	return NetworkObject::None;
}



QStringList BuiltinDirectoryPlugin::fileImportVariables()
{
	return { "%room%", "%name%", "%host%", "%mac%" };
}
