/*
 * BuiltinDirectoryPlugin.cpp - implementation of BuiltinDirectoryPlugin class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QFile>
#include <QRegularExpression>

#include "BuiltinDirectoryConfigurationPage.h"
#include "BuiltinDirectory.h"
#include "BuiltinDirectoryPlugin.h"
#include "CommandLineIO.h"
#include "ConfigurationManager.h"
#include "ObjectManager.h"
#include "PlatformFilesystemFunctions.h"
#include "VeyonConfiguration.h"


BuiltinDirectoryPlugin::BuiltinDirectoryPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() ),
	m_commands( {
{ QStringLiteral("help"), tr( "Show help for specific command" ) },
{ addCommand(), tr( "Add a location or computer" ) },
{ QStringLiteral("clear"), tr( "Clear all locations and computers" ) },
{ QStringLiteral("dump"), tr( "Dump all or individual locations and computers" ) },
{ QStringLiteral("list"), tr( "List all locations and computers" ) },
{ removeCommand(), tr( "Remove a location or computer" ) },
{ importCommand(), tr( "Import objects from given file" ) },
{ exportCommand(), tr( "Export objects to given file" ) },
				} )
{
}



void BuiltinDirectoryPlugin::upgrade( const QVersionNumber& oldVersion )
{
	if( oldVersion < QVersionNumber( 1, 1 ) &&
		m_configuration.legacyLocalDataNetworkObjects().isEmpty() == false )
	{
		m_configuration.setNetworkObjects( m_configuration.legacyLocalDataNetworkObjects() );
	}
}



NetworkObjectDirectory *BuiltinDirectoryPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new BuiltinDirectory( m_configuration, parent );
}



ConfigurationPage* BuiltinDirectoryPlugin::createNetworkObjectDirectoryConfigurationPage()
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

	if( command == importCommand() )
	{
		printUsage( commandLineModuleName(), importCommand(), { { tr("FILE"), {} } }, {
						{ tr("LOCATION"), locationArgument() },
						{ tr("FORMAT-STRING-WITH-PLACEHOLDERS"), formatArgument() },
						{ tr("REGULAR-EXPRESSION-WITH-PLACEHOLDER"), regexArgument() } } );

		printDescription( tr("Imports objects from the specified text file using the given format string or "
							 "regular expression containing one or multiple placeholders. "
							 "Valid placeholders are: %1").arg( importExportPlaceholders().join(QLatin1Char(' ') ) ) );

		printExamples( commandLineModuleName(), importCommand(), {
						   { tr( "Import simple CSV file to a single room" ),
							 { QStringLiteral("computers.csv"),
							   locationArgument(), exampleRoom(),
							   formatArgument(), QStringLiteral("\"%name%;%host%;%mac%\"") } },
						   { tr( "Import CSV file with location name in first column" ),
							 { QStringLiteral("computers-with-rooms.csv"),
							   formatArgument(), QStringLiteral("\"%location%;%name%;%mac%\"") } },
						   { tr( "Import text file with with key/value pairs using regular expressions" ),
							 { QStringLiteral("hostlist.txt"),
							   locationArgument(), exampleRoom(),
							   regexArgument(), QStringLiteral("\"^NAME:(%name%:.*)\\s+HOST:(%host%:.*)$\"") } },
						   { tr( "Import arbitrarily formatted data" ),
							 { QStringLiteral("data.txt"),
							   regexArgument(), QStringLiteral("'^\"(%location%:[^\"]+)\";\"(%host%:[a-z\\d\\.]+)\".*$'") } }
					   } );

		return NoResult;
	}

	if( command == exportCommand() )
	{
		printUsage( commandLineModuleName(), exportCommand(), { { tr("FILE"), {} } }, {
						{ tr("LOCATION"), locationArgument() },
						{ tr("FORMAT-STRING-WITH-PLACEHOLDERS"), formatArgument() } } );

		printDescription( tr("Exports objects to the specified text file using the given format string containing "
							 "one or multiple placeholders. "
							 "Valid placeholders are: %1").arg( importExportPlaceholders().join(QLatin1Char(' ') ) ) );

		printExamples( commandLineModuleName(), exportCommand(), {
						   { tr( "Export all objects to a CSV file" ),
							 { QStringLiteral("objects.csv"),
							   formatArgument(), QStringLiteral("\"%type%;%name%;%host%;%mac%\"") } },
						   { tr( "Export all computers in a specific location to a CSV file" ),
							 { QStringLiteral("computers.csv"),
							   locationArgument(), exampleRoom(),
							   formatArgument(), QStringLiteral("\"%name%;%host%;%mac%\"") } }
					   } );

		return NoResult;
	}

	if( command == addCommand() )
	{
		printUsage( commandLineModuleName(), addCommand(), { { tr("TYPE"), {} }, { tr("NAME"), {} } }, {
						{ tr("HOST ADDRESS"), {} },
						{ tr("MAC ADDRESS"), {} },
						{ tr("PARENT"), {} } } );

		printDescription( tr("Adds an object where %1 can be one of \"%2\" or \"%3\". %4 can be specified by name or UUID.").
						  arg( tr("TYPE"), typeLocation(), typeComputer(), tr("PARENT") ) );

		printExamples( commandLineModuleName(), addCommand(), {
						   { tr( "Add a room" ), { typeLocation(), exampleRoom() } },
						   { tr( "Add a computer to room %1" ).arg( exampleRoom() ), { typeComputer(), exampleComputer(), QStringLiteral("comp01.example.com"), QStringLiteral("11:22:33:44:55:66"), exampleRoom() } }
					   } );

		return NoResult;
	}

	if( command == removeCommand() )
	{
		printUsage( commandLineModuleName(), removeCommand(), { { tr("OBJECT"), {} } } );

		printDescription( tr("Removes the specified object from the directory. %1 can be specified by name or UUID. "
							 "Removing a location will also remove all related computers.").arg( tr("OBJECT") ) );

		printExamples( commandLineModuleName(), removeCommand(), {
						   { tr( "Remove a computer by name" ), { exampleComputer() } },
						   { tr( "Remove an object by UUID" ), { QStringLiteral("068914fc-0f87-45df-a5b9-099a2a6d9141") } }
					   } );

		return NoResult;
	}

	error( tr("The specified command does not exist or no help is available for it.") );

	return NoResult;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_add( const QStringList& arguments )
{
	if( arguments.count() < 2 )
	{
		return NotEnoughArguments;
	}

	NetworkObject object;

	const auto type = arguments[0];
	const auto name = arguments[1];

	if( type == typeLocation() )
	{
		object = NetworkObject( nullptr, NetworkObject::Type::Location, name );
	}
	else if( type == typeComputer() )
	{
		auto hostAddress = arguments.value( 2 );
		if( hostAddress.isEmpty() )
		{
			hostAddress = name;
		}
		const auto macAddress = arguments.value( 3 );
		const auto parent = findNetworkObject( arguments.value( 4 ) );
		object = NetworkObject( nullptr, NetworkObject::Type::Host, name,
								{
									{ NetworkObject::propertyKey(NetworkObject::Property::HostAddress), hostAddress },
									{ NetworkObject::propertyKey(NetworkObject::Property::MacAddress), macAddress }
								},
								{}, parent.isValid() ? parent.uid() : NetworkObject::Uid{} );
	}
	else
	{
		error( tr("Invalid type specified. Valid values are \"%1\" or \"%2\"." ).arg( typeComputer(), typeLocation() ) );
		return Failed;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add( object );
	m_configuration.setNetworkObjects( objectManager.objects() );

	return saveConfiguration();
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_clear( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	m_configuration.setNetworkObjects( {} );

	return saveConfiguration();
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_dump( const QStringList& arguments )
{
	TableHeader tableHeader( { tr("Object UUID"), tr("Parent UUID"), tr("Type"), tr("Name"), tr("Host address"), tr("MAC address") } );
	TableRows tableRows;

	const auto objects = m_configuration.networkObjects();

	tableRows.reserve( objects.size() );

	if( arguments.isEmpty() )
	{
		for( const auto& networkObjectValue : objects )
		{
			tableRows.append( dumpNetworkObject( NetworkObject{networkObjectValue.toObject()} ) );
		}
	}
	else
	{
		tableRows.append( dumpNetworkObject( findNetworkObject( arguments.first() ) ) );
	}

	printTable( Table( tableHeader, tableRows ) );

	return NoResult;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_list( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		listObjects( m_configuration.networkObjects(), NetworkObject{} );
	}
	else
	{
		const auto parents = BuiltinDirectory( m_configuration, this ).queryObjects( NetworkObject::Type::Location,
																					 NetworkObject::Property::Name,
																					 arguments.first() );

		for( const auto& parent : parents )
		{
			listObjects( m_configuration.networkObjects(), parent );
		}
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_remove( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	const auto object = findNetworkObject( arguments.first() );

	if( object.isValid() )
	{
		ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
		objectManager.remove( object.uid(), true );
		m_configuration.setNetworkObjects( objectManager.objects() );

		return saveConfiguration();
	}

	error( tr( "Specified object not found." ) );

	return Failed;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_import( const QStringList& arguments )
{
	if( arguments.count() < 3 )
	{
		return NotEnoughArguments;
	}

	const auto& inputFileName = arguments.first();
	QFile inputFile( inputFileName );

	if( inputFile.exists() == false )
	{
		error( tr( "File \"%1\" does not exist!" ).arg( inputFileName ) );
		return Failed;
	}

	if( inputFile.open( QFile::ReadOnly | QFile::Text ) == false )
	{
		error( tr( "Can't open file \"%1\" for reading!" ).arg( inputFileName ) );
		return Failed;
	}

	QString location;
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
		if( key == locationArgument() )
		{
			location = value;
		}
		else if( key == formatArgument() )
		{
			formatString = value;
		}
		else if( key == regexArgument() )
		{
			regularExpression = value;
		}
		else
		{
			error( tr( "Unknown argument \"%1\"." ).arg( key ) );
			return InvalidArguments;
		}
	}

	if( formatString.isEmpty() == false )
	{
		regularExpression = formatString;

		const auto placeholders = importExportPlaceholders();

		for( const auto& placeholder : placeholders )
		{
			regularExpression.replace( placeholder, QStringLiteral("(%1:[^\\n\\r]*)").arg( placeholder ) );
		}
	}

	if( regularExpression.isEmpty() == false )
	{
		if( importFile( inputFile, regularExpression, location ) )
		{
			return saveConfiguration();
		}

		return Failed;
	}

	error( tr("No format string or regular expression specified!") );

	return InvalidArguments;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_export( const QStringList& arguments )
{
	if( arguments.count() < 3 )
	{
		return NotEnoughArguments;
	}

	const auto& outputFileName = arguments.first();
	QFile outputFile( outputFileName );

	if( VeyonCore::platform().filesystemFunctions().openFileSafely(
			&outputFile,
			QFile::WriteOnly | QFile::Truncate | QFile::Text,
			QFile::ReadOwner | QFile::WriteOwner ) == false )
	{
		error( tr( "Can't open file \"%1\" for writing!" ).arg( outputFileName ) );
		return Failed;
	}

	QString location;
	QString formatString;

	for( int i = 1; i < arguments.count(); i += 2 )
	{
		if( i+1 >= arguments.count() )
		{
			return NotEnoughArguments;
		}

		const auto key = arguments[i];
		const auto value = arguments[i+1];
		if( key == locationArgument() )
		{
			location = value;
		}
		else if( key == formatArgument() )
		{
			formatString = value;
		}
		else
		{
			error( tr( "Unknown argument \"%1\"." ).arg( key ) );
			return InvalidArguments;
		}
	}

	if( formatString.isEmpty() == false )
	{
		if( exportFile( outputFile, formatString, location ) )
		{
			return Successful;
		}

		return Failed;
	}

	error( tr("No format string specified!") );

	return InvalidArguments;
}



void BuiltinDirectoryPlugin::listObjects( const QJsonArray& objects, const NetworkObject& parent )
{
	for( const auto& networkObjectValue : objects )
	{
		const NetworkObject networkObject{networkObjectValue.toObject()};

		if( ( parent.type() == NetworkObject::Type::None && networkObject.parentUid().isNull() ) ||
			networkObject.parentUid() == parent.uid() )
		{
			printf( "%s\n", qUtf8Printable( listNetworkObject( networkObject ) ) );
			listObjects( objects, networkObject );
		}
	}
}



QStringList BuiltinDirectoryPlugin::dumpNetworkObject( const NetworkObject& object )
{
	return {
		VeyonCore::formattedUuid( object.uid() ),
		VeyonCore::formattedUuid( object.parentUid() ),
		networkObjectTypeName( object ),
		object.name(),
		object.property( NetworkObject::Property::HostAddress ).toString(),
		object.property( NetworkObject::Property::MacAddress ).toString()
	};
}



QString BuiltinDirectoryPlugin::listNetworkObject( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Type::Location:
		return tr( "Location \"%1\"" ).arg( object.name() );
	case NetworkObject::Type::Host:
		return QLatin1Char('\t') +
				tr( "Computer \"%1\" (host address: \"%2\" MAC address: \"%3\")" ).
			   arg( object.name(),
					object.property( NetworkObject::Property::HostAddress ).toString(),
					object.property( NetworkObject::Property::MacAddress ).toString() );
	default:
		break;
	}

	return tr( "Unclassified object \"%1\" with ID \"%2\"" ).arg( object.name(), object.uid().toString() );
}



QString BuiltinDirectoryPlugin::networkObjectTypeName( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Type::None: return tr( "None" );
	case NetworkObject::Type::Location: return typeNameLocation();
	case NetworkObject::Type::Host: return typeNameComputer();
	case NetworkObject::Type::Root: return typeNameRoot();
	default:
		break;
	}

	return tr( "Invalid" );
}



NetworkObject::Type BuiltinDirectoryPlugin::parseNetworkObjectType( const QString& typeName )
{
	if( typeName == typeNameLocation() )
	{
		return NetworkObject::Type::Location;
	}

	if( typeName == typeNameComputer() )
	{
		return NetworkObject::Type::Host;
	}

	if( typeName == typeNameRoot() )
	{
		return NetworkObject::Type::Root;
	}

	return NetworkObject::Type::None;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::saveConfiguration()
{
	ConfigurationManager configurationManager;
	if( configurationManager.saveConfiguration() == false )
	{
		error( configurationManager.errorString() );
		return Failed;
	}

	return Successful;
}



bool BuiltinDirectoryPlugin::importFile( QFile& inputFile,
										 const QString& regExWithPlaceholders,
										 const QString& location )
{
	int lineCount = 0;
	QMap<QString, NetworkObjectList > networkObjects;
	while( inputFile.atEnd() == false )
	{
		++lineCount;

		auto targetLocation = location;

		const auto line = inputFile.readLine();
		const auto networkObject = toNetworkObject( QString::fromUtf8( line ), regExWithPlaceholders, targetLocation );

		if( networkObject.isValid() )
		{
			networkObjects[targetLocation].append( networkObject );
		}
		else
		{
			error( tr( "Error while parsing line %1." ).arg( lineCount ) );
			return false;
		}
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

	for( auto it = networkObjects.constBegin(), end = networkObjects.constEnd(); it != end; ++it )
	{
		auto parentLocation = objectManager.findByName( it.key() );
		auto parentLocationUid = parentLocation.uid();
		if( it.key().isEmpty() )
		{
			parentLocationUid = QUuid();
		}
		else if( parentLocation.isValid() == false )
		{
			parentLocation = NetworkObject( nullptr, NetworkObject::Type::Location, it.key() );
			objectManager.update( parentLocation, true );
			parentLocationUid = parentLocation.uid();
		}

		for( const NetworkObject& networkObject : std::as_const(it.value()) )
		{
			objectManager.update( NetworkObject( nullptr,
												 networkObject.type(),
												 networkObject.name(),
												 networkObject.properties(),
												 {},
												 parentLocationUid ), true );
		}
	}

	m_configuration.setNetworkObjects( objectManager.objects() );

	return true;
}



bool BuiltinDirectoryPlugin::exportFile( QFile& outputFile, const QString& formatString, const QString& location )
{
	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

	const auto& networkObjects = objectManager.objects();

	NetworkObject locationObject;
	if( location.isEmpty() == false )
	{
		locationObject = objectManager.findByName( location );
		if (locationObject.isValid() == false)
		{
			error(tr("Location \"%1\" not found." ).arg(location));
			return false;
		}
	}

	QStringList lines;
	lines.reserve( networkObjects.count() );

	for( auto it = networkObjects.constBegin(), end = networkObjects.constEnd(); it != end; ++it )
	{
		const NetworkObject networkObject{it->toObject()};
		if (networkObject.type() != NetworkObject::Type::Host)
		{
			continue;
		}

		auto currentLocation = location;

		if( locationObject.isValid() )
		{
			if( networkObject.parentUid() != locationObject.uid() )
			{
				continue;
			}
		}
		else
		{
			currentLocation = objectManager.findByUid( networkObject.parentUid() ).name();
		}

		lines.append( toFormattedString( networkObject, formatString, currentLocation ) );
	}

	// append empty string to generate final newline at end of file
	lines += QString();

	outputFile.write(lines.join(QLatin1Char('\n')).toUtf8());

	return true;
}



NetworkObject BuiltinDirectoryPlugin::findNetworkObject( const QString& uidOrName ) const
{
	const ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	const QUuid uid{uidOrName};

	if( uid.isNull() )
	{
		return objectManager.findByName( uidOrName );
	}

	return objectManager.findByUid( uid );
}



NetworkObject BuiltinDirectoryPlugin::toNetworkObject( const QString& line, const QString& regExWithPlaceholders,
													   QString& location )
{
	QStringList placeholders;
	static const QRegularExpression varDetectionRX{QStringLiteral("\\((%\\w+%):[^)]+\\)")};
	auto varDetectionMatchIterator = varDetectionRX.globalMatch(regExWithPlaceholders);

	while( varDetectionMatchIterator.hasNext() )
	{
		placeholders.append( varDetectionMatchIterator.next().captured(1) );
	}

	QString rxString = regExWithPlaceholders;
	for( const auto& var : std::as_const(placeholders) )
	{
		rxString.replace( QStringLiteral("%1:").arg( var ), QString() );
	}

	auto match = QRegularExpression( rxString ).match( line );
	if( match.hasMatch() )
	{
		auto objectType = NetworkObject::Type::Host;
		const auto typeIndex = placeholders.indexOf( QStringLiteral("%type%") );
		if( typeIndex != -1 )
		{
			objectType = parseNetworkObjectType( match.captured( 1 + typeIndex ) );
		}

		const auto locationIndex = placeholders.indexOf( QStringLiteral("%location%") );
		const auto nameIndex = placeholders.indexOf( QStringLiteral("%name%") );
		const auto hostIndex = placeholders.indexOf( QStringLiteral("%host%") );
		const auto macIndex = placeholders.indexOf( QStringLiteral("%mac%") );

		auto name = ( nameIndex != -1 ) ? match.captured( 1 + nameIndex ).trimmed() : QString();
		auto host = ( hostIndex != -1 ) ? match.captured( 1 + hostIndex ).trimmed() : QString();
		auto mac = ( macIndex != -1 ) ? match.captured( 1 + macIndex ).trimmed() : QString();

		if( objectType == NetworkObject::Type::Location )
		{
			return NetworkObject( nullptr, NetworkObject::Type::Location, name );
		}

		if( location.isEmpty() && locationIndex != -1 )
		{
			location = match.captured( 1 + locationIndex ).trimmed();
		}

		if( host.isEmpty() )
		{
			host = name;
		}
		else if( name.isEmpty() )
		{
			name = host;
		}
		return NetworkObject( nullptr,
							  NetworkObject::Type::Host, name,
							  {
								  { NetworkObject::propertyKey(NetworkObject::Property::HostAddress), host },
								  { NetworkObject::propertyKey(NetworkObject::Property::MacAddress), mac }
							  } );
	}

	return NetworkObject{};
}



QString BuiltinDirectoryPlugin::toFormattedString( const NetworkObject& networkObject,
												   const QString& formatString,
												   const QString& location )
{
	return QString( formatString ).
			replace( QStringLiteral("%location%"), location ).
			replace( QStringLiteral("%name%"), networkObject.name() ).
			replace( QStringLiteral("%host%"), networkObject.property( NetworkObject::Property::HostAddress ).toString() ).
			replace( QStringLiteral("%mac%"), networkObject.property( NetworkObject::Property::MacAddress ).toString() ).
			replace( QStringLiteral("%type%"), networkObjectTypeName( networkObject ) );
}



QStringList BuiltinDirectoryPlugin::importExportPlaceholders()
{
	return { QStringLiteral("%location%"),
				QStringLiteral("%name%"),
				QStringLiteral("%host%"),
				QStringLiteral("%mac%"),
				QStringLiteral("%type%") };
}



IMPLEMENT_CONFIG_PROXY(BuiltinDirectoryConfiguration)
