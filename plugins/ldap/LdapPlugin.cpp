/*
 * LdapPlugin.cpp - implementation of LdapPlugin class
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

#include "Configuration/LocalStore.h"
#include "VeyonConfiguration.h"
#include "LdapNetworkObjectDirectory.h"
#include "LdapPlugin.h"
#include "LdapConfigurationPage.h"
#include "LdapDirectory.h"
#include "LocalSystem.h"


LdapPlugin::LdapPlugin() :
	m_configuration(),
	m_ldapDirectory( nullptr ),
	m_commands( {
{ "autoconfigurebasedn", tr( "Auto-configure the base DN via naming context" ) },
{ "query", tr( "Query objects from LDAP directory" ) },
{ "help", tr( "Show help about command" ) },
				} )
{
}



LdapPlugin::~LdapPlugin()
{
	delete m_ldapDirectory;
	m_ldapDirectory = nullptr;
}



QStringList LdapPlugin::commands() const
{
	return m_commands.keys();
}



QString LdapPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult LdapPlugin::runCommand( const QStringList& arguments )
{
	// all commands are handled as slots so if we land here an unsupported command has been called
	return InvalidCommand;
}



NetworkObjectDirectory *LdapPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new LdapNetworkObjectDirectory( m_configuration, parent );
}



void LdapPlugin::reloadConfiguration()
{
	delete m_ldapDirectory;
	m_ldapDirectory = new LdapDirectory( m_configuration );
}



QStringList LdapPlugin::users()
{
	return ldapDirectory().users();
}



QStringList LdapPlugin::userGroups()
{
	return ldapDirectory().toRelativeDnList( ldapDirectory().userGroups() );
}



QStringList LdapPlugin::groupsOfUser( const QString& username )
{
	const auto strippedUsername = LocalSystem::User::stripDomain( username );

	const QString userDn = ldapDirectory().users( strippedUsername ).value( 0 );

	if( userDn.isEmpty() )
	{
		qWarning() << "LdapPlugin::groupsOfUser(): empty user DN for user" << strippedUsername;
		return QStringList();
	}

	return ldapDirectory().toRelativeDnList( ldapDirectory().groupsOfUser( userDn ) );
}



QStringList LdapPlugin::allRooms()
{
	return ldapDirectory().computerRooms();
}



QStringList LdapPlugin::roomsOfComputer( const QString& computerName )
{
	const QString computerDn = ldapDirectory().computerObjectFromHost( computerName );

	if( computerDn.isEmpty() )
	{
		qWarning() << "LdapPlugin::roomsOfComputer(): empty computer DN for computer" << computerName;
		return QStringList();
	}

	return ldapDirectory().computerRoomsOfComputer( computerDn );
}



ConfigurationPage *LdapPlugin::createConfigurationPage()
{
	return new LdapConfigurationPage( m_configuration );
}



CommandLinePluginInterface::RunResult LdapPlugin::handle_autoconfigurebasedn( const QStringList& arguments )
{
	QUrl ldapUrl;
	ldapUrl.setUrl( arguments.value( 0 ), QUrl::StrictMode );

	if( ldapUrl.isValid() == false || ldapUrl.host().isEmpty() )
	{
		qCritical() << "Please specify a valid LDAP url following the schema \"ldap[s]://[user[:password]@]hostname[:port]\"";
		return InvalidArguments;
	}

	QString namingContextAttribute = arguments.value( 1 );

	if( namingContextAttribute.isEmpty() )
	{
		qWarning( "No naming context attribute name given - falling back to configured value." );
	}
	else
	{
		m_configuration.setLdapNamingContextAttribute( namingContextAttribute );
	}

	LdapDirectory ldapDirectory( m_configuration, ldapUrl );
	QString baseDn = ldapDirectory.queryNamingContext();

	if( baseDn.isEmpty() )
	{
		qCritical( "Could not query base DN. Please check your LDAP configuration." );
		return Failed;
	}

	qInfo() << "Configuring" << baseDn << "as base DN and disabling naming context queries.";

	m_configuration.setLdapBaseDn( baseDn );
	m_configuration.setLdapQueryNamingContext( false );

	// write configuration
	Configuration::LocalStore localStore( Configuration::LocalStore::System );
	localStore.flush( &VeyonCore::config() );

	return Successful;
}



CommandLinePluginInterface::RunResult LdapPlugin::handle_query( const QStringList& arguments )
{
	QString objectType = arguments.value( 0 );
	QString filter = arguments.value( 1 );
	QStringList results;

	if( objectType == "rooms" )
	{
		results = ldapDirectory().computerRooms( filter );
	}
	else if( objectType == "computers" )
	{
		results = ldapDirectory().computers( filter );
	}
	else if( objectType == "groups" )
	{
		results = ldapDirectory().groups( filter );
	}
	else if( objectType == "users" )
	{
		results = ldapDirectory().users( filter );
	}
	else
	{
		return InvalidArguments;
	}

	for( const auto& result : qAsConst( results ) )
	{
		printf( "%s\n", qUtf8Printable( result ) );
	}

	return Successful;
}




CommandLinePluginInterface::RunResult LdapPlugin::handle_help( const QStringList& arguments )
{
	QString command = arguments.value( 0 );

	if( command == "autoconfigurebasedn" )
	{
		printf( "\n"
				"ldap autoconfigurebasedn <LDAP URL> [<naming context attribute name>]\n"
				"\n"
				"Automatically configures the LDAP base DN configuration setting by querying\n"
				"the naming context attribute from given LDAP server. The LDAP url parameter\n"
				"needs to follow the schema:\n"
				"\n"
				"  ldap[s]://[user[:password]@]hostname[:port]\n\n" );
		return NoResult;
	}
	else if( command == "query" )
	{
		printf( "\n"
				"ldap query <object type> [filter]\n"
				"\n"
				"Query objects from configured LDAP directory where <object type> may be one\n"
				"of \"rooms\", \"computers\", \"groups\" or \"users\". You can optionally\n"
				"specify a filter such as \"foo*\".\n"
				"\n" );
		return NoResult;
	}

	return InvalidCommand;
}



LdapDirectory& LdapPlugin::ldapDirectory()
{
	if( m_ldapDirectory == nullptr )
	{
		m_ldapDirectory = new LdapDirectory( m_configuration );
	}

	// TODO: check whether still connected and reconnect if neccessary

	return *m_ldapDirectory;
}
