/*
 * ConfigCommandLinePlugin.cpp - implementation of ConfigCommandLinePlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QJsonDocument>

#include "Configuration/JsonStore.h"
#include "Configuration/LocalStore.h"
#include "ConfigCommandLinePlugin.h"
#include "SystemConfigurationModifier.h"


ConfigCommandLinePlugin::ConfigCommandLinePlugin() :
	m_subCommands( {
				   std::pair<QString, QString>( "clear", "clear system-wide Veyon configuration" ),
				   std::pair<QString, QString>( "list", "list all configuration keys and values" ),
				   std::pair<QString, QString>( "import", "import configuration from given file" ),
				   std::pair<QString, QString>( "export", "export configuration to given file" ),
				   std::pair<QString, QString>( "get", "read and output configuration value for given key" ),
				   std::pair<QString, QString>( "set", "write given value to given configuration key" ),
				   std::pair<QString, QString>( "unset", "unset (remove) given configuration key" ),
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
	listConfiguration( VeyonCore::config().data(), QString() );

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_import( const QStringList& arguments )
{
	QString fileName = arguments.value( 0 );

	if( fileName.isEmpty() || QFile( fileName ).exists() == false )
	{
		return operationError( tr( "Please specify an existing configuration file to import." ) );
	}

	Configuration::JsonStore xs( Configuration::JsonStore::System, fileName );

	// merge configuration
	VeyonCore::config() += VeyonConfiguration( &xs );

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_export( const QStringList& arguments )
{
	QString fileName = arguments.value( 0 );

	if( fileName.isEmpty() )
	{
		return operationError( tr( "Please specify a valid filename for the configuration export." ) );
	}

	// write current configuration to output file
	Configuration::JsonStore( Configuration::JsonStore::System, fileName ).flush( &VeyonCore::config() );

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_get( const QStringList& arguments )
{
	QString key = arguments.value( 0 );

	if( key.isEmpty() )
	{
		return operationError( tr( "Please specify a valid key." ) );
	}

	QStringList keyParts = key.split( '/' );
	key = keyParts.last();
	QString parentKey;

	if( keyParts.size() > 1 )
	{
		parentKey = keyParts.mid( 0, keyParts.size()-1).join( '/' );
	}

	if( VeyonCore::config().hasValue( key, parentKey ) == false )
	{
		return operationError( tr( "Specified key does not exist in current configuration!" ) );
	}

	printf( "%s\n", qUtf8Printable( printableConfigurationValue( VeyonCore::config().value( key, parentKey ) ) ) );

	return NoResult;
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_set( const QStringList& arguments )
{
	QString key = arguments.value( 0 );
	QString value = arguments.value( 1 );

	if( key.isEmpty() )
	{
		return operationError( tr( "Please specify a valid key." ) );
	}

	if( value.isEmpty() )
	{
		return operationError( tr( "Please specify a valid value." ) );
	}

	QStringList keyParts = key.split( '/' );
	key = keyParts.last();
	QString parentKey;

	if( keyParts.size() > 1 )
	{
		parentKey = keyParts.mid( 0, keyParts.size()-1).join( '/' );
	}

	VeyonCore::config().setValue( key, value, parentKey );

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::handle_unset( const QStringList& arguments )
{
	QString key = arguments.value( 0 );

	if( key.isEmpty() )
	{
		return operationError( tr( "Please specify a valid key." ) );
	}

	QStringList keyParts = key.split( '/' );
	key = keyParts.last();
	QString parentKey;

	if( keyParts.size() > 1 )
	{
		parentKey = keyParts.mid( 0, keyParts.size()-1).join( '/' );
	}

	VeyonCore::config().removeValue( key, parentKey );

	return applyConfiguration();
}



void ConfigCommandLinePlugin::listConfiguration( const VeyonConfiguration::DataMap &map,
												 const QString &parentKey )
{
	for( auto it = map.begin(); it != map.end(); ++it )
	{
		QString curParentKey = parentKey.isEmpty() ? it.key() : parentKey + "/" + it.key();

		if( it.value().type() == QVariant::Map )
		{
			listConfiguration( it.value().toMap(), curParentKey );
		}
		else
		{
			QString value = printableConfigurationValue( it.value() );
			if( value.isNull() )
			{
				qWarning() << "Key" << it.key() << "has unknown value type:" << it.value();
			}
			else
			{
				QTextStream( stdout ) << curParentKey << "=" << value << endl;
			}
		}
	}
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::applyConfiguration()
{
	// do necessary modifications of system configuration
	if( SystemConfigurationModifier::setServiceAutostart( VeyonCore::config().autostartService() ) == false )
	{
		return operationError( tr( "Could not modify the autostart property for the %1 Service." ).arg( VeyonCore::applicationName() ) );
	}

	if( SystemConfigurationModifier::setServiceArguments( VeyonCore::config().serviceArguments() ) == false )
	{
		return operationError( tr( "Could not modify the service arguments for the %1 Service." ).arg( VeyonCore::applicationName() ) );
	}

	if( SystemConfigurationModifier::enableFirewallException( VeyonCore::config().isFirewallExceptionEnabled() ) == false )
	{
		return operationError( tr( "Could not change the firewall configuration for the %1 Service." ).arg( VeyonCore::applicationName() ) );
	}

	if( SystemConfigurationModifier::enableSoftwareSAS( VeyonCore::config().isSoftwareSASEnabled() ) == false )
	{
		return operationError( tr( "Could not change the setting for SAS generation by software. Sending Ctrl+Alt+Del via remote control will not work!" ) );
	}

	// write global configuration
	Configuration::LocalStore localStore( Configuration::LocalStore::System );
	localStore.flush( &VeyonCore::config() );

	return Successful;
}



QString ConfigCommandLinePlugin::printableConfigurationValue( const QVariant& value )
{
	if( value.type() == QVariant::String ||
			value.type() == QVariant::Uuid ||
			value.type() == QVariant::UInt ||
			value.type() == QVariant::Bool )
	{
		return value.toString();
	}
	else if( value.type() == QVariant::StringList )
	{
		QStringList list = value.toStringList();
		for( auto& str : list )
		{
			str = "\"" + str + "\"";
		}
		return "(" + list.join( ',' ) + ")";
	}
	else if( value.userType() == QMetaType::type( "QJsonArray" ) )
	{
		return QString::fromUtf8( QJsonDocument( value.value<QJsonArray>() ).toJson( QJsonDocument::Compact ) );
	}

	return QString();
}



CommandLinePluginInterface::RunResult ConfigCommandLinePlugin::operationError( const QString& message )
{
	qCritical( "%s", qUtf8Printable( message ) );

	return Failed;
}
