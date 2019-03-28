/*
 * LdapPlugin.cpp - implementation of LdapPlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "CommandLineIO.h"
#include "ConfigurationManager.h"
#include "VeyonConfiguration.h"
#include "LdapNetworkObjectDirectory.h"
#include "LdapPlugin.h"
#include "LdapConfigurationPage.h"
#include "LdapDirectory.h"
#include "PlatformUserFunctions.h"


LdapPlugin::LdapPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration(),
	m_ldapDirectory( nullptr ),
	m_commands( {
{ QStringLiteral("autoconfigurebasedn"), tr( "Auto-configure the base DN via naming context" ) },
{ QStringLiteral("query"), tr( "Query objects from LDAP directory" ) },
{ QStringLiteral("help"), tr( "Show help about command" ) },
				} )
{
}



LdapPlugin::~LdapPlugin()
{
	delete m_ldapDirectory;
	m_ldapDirectory = nullptr;
}



void LdapPlugin::upgrade( const QVersionNumber& oldVersion )
{
	if( oldVersion < QVersionNumber( 1, 1 ) )
	{
		const auto upgradeFilter = []( const QString& filter ) {
			if( filter.isEmpty() == false &&
					filter.startsWith( '(' ) == false )
			{
				return QStringLiteral("(%1)").arg( filter );
			}
			// already upgraded or special filter so don't touch
			return filter;
		};

		m_configuration.setComputerContainersFilter( upgradeFilter( m_configuration.computerContainersFilter() ) );
		m_configuration.setComputerGroupsFilter( upgradeFilter( m_configuration.computerGroupsFilter() ) );
		m_configuration.setComputersFilter( upgradeFilter( m_configuration.computersFilter() ) );
		m_configuration.setUserGroupsFilter( upgradeFilter( m_configuration.userGroupsFilter() ) );
		m_configuration.setUsersFilter( upgradeFilter( m_configuration.usersFilter() ) );

		// bind password not encrypted yet?
		if( m_configuration.bindPasswordPlain().size() < MaximumPlaintextPasswordLength )
		{
			// setting it again will encrypt it
			m_configuration.setBindPassword( m_configuration.bindPasswordPlain() );
		}
	}
}



QStringList LdapPlugin::commands() const
{
	return m_commands.keys();
}



QString LdapPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
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



QStringList LdapPlugin::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	return ldapDirectory().toRelativeDnList( ldapDirectory().userGroups() );
}



QStringList LdapPlugin::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	const auto strippedUsername = VeyonCore::stripDomain( VeyonCore::platform().userFunctions().upnToUsername(username) );

	const QString userDn = ldapDirectory().users( strippedUsername ).value( 0 );

	if( userDn.isEmpty() )
	{
		qWarning() << "LdapPlugin::groupsOfUser(): empty user DN for user" << strippedUsername;
		return QStringList();
	}

	return ldapDirectory().toRelativeDnList( ldapDirectory().groupsOfUser( userDn ) );
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
		m_configuration.setNamingContextAttribute( namingContextAttribute );
	}

	LdapDirectory ldapDirectory( m_configuration, ldapUrl );
	QString baseDn = ldapDirectory.queryNamingContext();

	if( baseDn.isEmpty() )
	{
		qCritical( "Could not query base DN. Please check your LDAP configuration." );
		return Failed;
	}

	qInfo() << "Configuring" << baseDn << "as base DN and disabling naming context queries.";

	m_configuration.setBaseDn( baseDn );
	m_configuration.setQueryNamingContext( false );

	// write configuration
	ConfigurationManager configurationManager;
	if( configurationManager.saveConfiguration() == false )
	{
		CommandLineIO::error( configurationManager.errorString() );
		return Failed;
	}

	return Successful;
}



CommandLinePluginInterface::RunResult LdapPlugin::handle_query( const QStringList& arguments )
{
	QString objectType = arguments.value( 0 );
	QString filter = arguments.value( 1 );
	QStringList results;

	if( objectType == QStringLiteral("rooms") )
	{
		results = ldapDirectory().computerRooms( filter );
	}
	else if( objectType == QStringLiteral("computers") )
	{
		results = ldapDirectory().computers( filter );
	}
	else if( objectType == QStringLiteral("groups") )
	{
		results = ldapDirectory().groups( filter );
	}
	else if( objectType == QStringLiteral("users") )
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

	if( command == QStringLiteral("autoconfigurebasedn") )
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
	else if( command == QStringLiteral("query") )
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
