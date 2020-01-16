/*
 * ConfigCommands.cpp - implementation of ConfigCommands class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#include "CommandLineIO.h"
#include "Configuration/JsonStore.h"
#include "ConfigCommands.h"
#include "ConfigurationManager.h"
#include "CryptoCore.h"


ConfigCommands::ConfigCommands( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ QStringLiteral("clear"), tr( "Clear system-wide Veyon configuration" ) },
{ QStringLiteral("list"), tr( "List all configuration keys and values" ) },
{ QStringLiteral("import"), tr( "Import configuration from given file" ) },
{ QStringLiteral("export"), tr( "Export configuration to given file" ) },
{ QStringLiteral("get"), tr( "Read and output configuration value for given key" ) },
{ QStringLiteral("set"), tr( "Write given value to given configuration key" ) },
{ QStringLiteral("unset"), tr( "Unset (remove) given configuration key" ) },
{ QStringLiteral("upgrade"), tr( "Upgrade and save configuration of program and plugins" ) },
				} )
{
}



QStringList ConfigCommands::commands() const
{
	return m_commands.keys();
}



QString ConfigCommands::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_clear( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	if( ConfigurationManager().clearConfiguration() )
	{
		return Successful;
	}

	return Failed;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_list( const QStringList& arguments )
{
	auto listMode = ListMode::Values;

	if( arguments.value( 0 ) == QLatin1String("defaults") )
	{
		listMode = ListMode::Defaults;
	}
	else if( arguments.value( 0 ) == QLatin1String("types") )
	{
		listMode = ListMode::Types;
	}

	listConfiguration( listMode );

	return NoResult;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_import( const QStringList& arguments )
{
	QString fileName = arguments.value( 0 );

	if( fileName.isEmpty() || QFile( fileName ).exists() == false )
	{
		return operationError( tr( "Please specify an existing configuration file to import." ) );
	}

	if( QFileInfo( fileName ).isReadable() == false )
	{
		return operationError( tr( "Configuration file is not readable!" ) );
	}

	Configuration::JsonStore xs( Configuration::JsonStore::System, fileName );

	// merge configuration
	VeyonCore::config() += VeyonConfiguration( &xs );

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_export( const QStringList& arguments )
{
	QString fileName = arguments.value( 0 );

	if( fileName.isEmpty() )
	{
		return operationError( tr( "Please specify a valid filename for the configuration export." ) );
	}

	QFileInfo fileInfo( fileName );
	if( fileInfo.exists() && fileInfo.isWritable() == false )
	{
		return operationError( tr( "Output file is not writable!" ) );
	}

	if( fileInfo.exists() == false && QFileInfo( fileInfo.dir().path() ).isWritable() == false )
	{
		return operationError( tr( "Output directory is not writable!" ) );
	}

	// write current configuration to output file
	Configuration::JsonStore( Configuration::JsonStore::System, fileName ).flush( &VeyonCore::config() );

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_get( const QStringList& arguments )
{
	QString key = arguments.value( 0 );

	if( key.isEmpty() )
	{
		return operationError( tr( "Please specify a valid key." ) );
	}

	QStringList keyParts = key.split( QLatin1Char('/') );
	key = keyParts.last();
	QString parentKey;

	if( keyParts.size() > 1 )
	{
		parentKey = keyParts.mid( 0, keyParts.size()-1).join( QLatin1Char('/') );
	}

	if( VeyonCore::config().hasValue( key, parentKey ) == false )
	{
		return operationError( tr( "Specified key does not exist in current configuration!" ) );
	}

	CommandLineIO::print( printableConfigurationValue( VeyonCore::config().value( key, parentKey, QVariant() ) ) );

	return NoResult;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_set( const QStringList& arguments )
{
	auto key = arguments.value( 0 );
	auto value = arguments.value( 1 );
	auto type = arguments.value( 2 );

	if( key.isEmpty() )
	{
		return operationError( tr( "Please specify a valid key." ) );
	}

	if( value.isEmpty() )
	{
		return operationError( tr( "Please specify a valid value." ) );
	}

	const auto keyParts = key.split( QLatin1Char('/') );
	key = keyParts.last();
	QString parentKey;

	if( keyParts.size() > 1 )
	{
		parentKey = keyParts.mid( 0, keyParts.size()-1).join( QLatin1Char('/') );
	}

	auto valueType = QMetaType::UnknownType;
	const auto property = Configuration::Property::find( &VeyonCore::config(), key, parentKey );

	if( property )
	{
		valueType = static_cast<QMetaType::Type>( property->variantValue().userType() );
	}

	QVariant configValue = value;

	if( type == QLatin1String("json") ||
		valueType == QMetaType::QJsonArray )
	{
		configValue = QJsonDocument::fromJson( value.toUtf8() ).array();
	}
	else if( key.contains( QStringLiteral("password"), Qt::CaseInsensitive ) ||
			 type == QLatin1String("password") )
	{
		configValue = VeyonCore::cryptoCore().encryptPassword( value.toUtf8() );
	}
	else if( type == QLatin1String("list") ||
			 valueType == QMetaType::QStringList )
	{
		configValue = value.split( QLatin1Char( ';' ) );
	}

	VeyonCore::config().setValue( key, configValue, parentKey );

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_unset( const QStringList& arguments )
{
	QString key = arguments.value( 0 );

	if( key.isEmpty() )
	{
		return operationError( tr( "Please specify a valid key." ) );
	}

	QStringList keyParts = key.split( QLatin1Char('/') );
	key = keyParts.last();
	QString parentKey;

	if( keyParts.size() > 1 )
	{
		parentKey = keyParts.mid( 0, keyParts.size()-1).join( QLatin1Char('/') );
	}

	VeyonCore::config().removeValue( key, parentKey );

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_upgrade( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	// upgrade already happened while loading plugins so only save upgraded configuration
	return applyConfiguration();
}



void ConfigCommands::listConfiguration( ListMode listMode ) const
{
	QTextStream stdoutStream( stdout );

	const auto properties = VeyonCore::config().findChildren<Configuration::Property *>();

	for( auto property : properties )
	{
		if( property->flags().testFlag( Configuration::Property::Flag::Legacy ) )
		{
			continue;
		}

		stdoutStream << property->absoluteKey() << "=";
		switch( listMode )
		{
		case ListMode::Values:
			stdoutStream << printableConfigurationValue( property->variantValue() );
			break;
		case ListMode::Defaults:
			stdoutStream << printableConfigurationValue( property->defaultValue() );
			break;
		case ListMode::Types:
			stdoutStream << QStringLiteral("[%1]").arg( QString::fromLatin1(
															property->defaultValue().typeName() ).
														replace( QLatin1Char('Q'), QString() ).toLower() );
			break;
		}
		stdoutStream << endl;
	}
}



CommandLinePluginInterface::RunResult ConfigCommands::applyConfiguration()
{
	ConfigurationManager configurationManager;

	if( configurationManager.saveConfiguration() == false ||
		configurationManager.applyConfiguration() == false )
	{
		return operationError( configurationManager.errorString() );
	}

	return Successful;
}



QString ConfigCommands::printableConfigurationValue( const QVariant& value )
{
	if( value.type() == QVariant::String ||
		value.type() == QVariant::Uuid ||
		value.type() == QVariant::UInt ||
		value.type() == QVariant::Int ||
		value.type() == QVariant::Bool )
	{
		return value.toString();
	}
	else if( value.type() == QVariant::StringList )
	{
		return value.toStringList().join( QLatin1Char(';') );
	}
	else if( value.userType() == QMetaType::QJsonArray )
	{
		return QString::fromUtf8( QJsonDocument( value.toJsonArray() ).toJson( QJsonDocument::Compact ) );
	}
	else if( value.userType() == QMetaType::QJsonObject )
	{
		return QString::fromUtf8( QJsonDocument( value.toJsonObject() ).toJson( QJsonDocument::Compact ) );
	}
	else if( QMetaType( value.userType() ).flags().testFlag( QMetaType::IsEnumeration ) )
	{
		return QString::number( value.toInt() );
	}

	return {};
}



CommandLinePluginInterface::RunResult ConfigCommands::operationError( const QString& message )
{
	CommandLineIO::error( message );

	return Failed;
}
