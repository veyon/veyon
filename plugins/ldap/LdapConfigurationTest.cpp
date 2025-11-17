// Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "LdapConfiguration.h"
#include "LdapConfigurationTest.h"
#include "LdapDirectory.h"


LdapConfigurationTest::LdapConfigurationTest( LdapConfiguration& configuration ) :
	m_configuration( configuration )
{
}



LdapConfigurationTest::Result LdapConfigurationTest::testBind()
{
	vDebug() << "[TEST][LDAP] Testing bind";

	LdapClient ldapClient( m_configuration );

	if( ldapClient.isConnected() == false )
	{
		return { false,
				LdapConfiguration::tr( "LDAP connection failed"),
				LdapConfiguration::tr( "Could not connect to the LDAP server. "
									   "Please check the server parameters.\n\n"
									   "%1" ).arg( ldapClient.errorDescription() ) };
	}
	else if( ldapClient.isBound() == false )
	{
		return { false,
				LdapConfiguration::tr( "LDAP bind failed"),
				LdapConfiguration::tr( "Could not bind to the LDAP server. "
									   "Please check the server parameters "
									   "and bind credentials.\n\n"
									   "%1" ).arg( ldapClient.errorDescription() ) };
	}

	return { true,
			LdapConfiguration::tr( "LDAP bind successful"),
			LdapConfiguration::tr( "Successfully connected to the LDAP "
								   "server and performed an LDAP bind. "
								   "The basic LDAP settings are "
								   "configured correctly." ) };
}



LdapConfigurationTest::Result LdapConfigurationTest::testBaseDn()
{
	const auto bindTestResult = testBind();
	if( bindTestResult )
	{
		vDebug() << "[TEST][LDAP] Testing base DN";

		LdapClient ldapClient( m_configuration );
		QStringList entries = ldapClient.queryBaseDn();

		if( entries.isEmpty() )
		{
			return { false,
					LdapConfiguration::tr( "LDAP base DN test failed"),
					LdapConfiguration::tr( "Could not query the configured base DN. "
										   "Please check the base DN parameter.\n\n"
										   "%1" ).arg( ldapClient.errorDescription() ) };
		}

		return { true,
				LdapConfiguration::tr( "LDAP base DN test successful" ),
				LdapConfiguration::tr( "The LDAP base DN has been queried successfully. "
									   "The following entries were found:\n\n%1" ).
				arg( entries.join(QLatin1Char('\n')) ) };
	}

	return bindTestResult;
}



LdapConfigurationTest::Result LdapConfigurationTest::testNamingContext()
{
	const auto bindTestResult = testBind();
	if( bindTestResult )
	{
		vDebug() << "[TEST][LDAP] Testing naming context";

		LdapClient ldapClient( m_configuration );
		const auto baseDn = ldapClient.queryNamingContexts().value( 0 );

		if( baseDn.isEmpty() )
		{
			return { false,
					LdapConfiguration::tr( "LDAP naming context test failed"),
					LdapConfiguration::tr( "Could not query the base DN via naming contexts. "
										   "Please check the naming context attribute parameter.\n\n"
										   "%1" ).arg( ldapClient.errorDescription() ) };
		}

		return { true,
				LdapConfiguration::tr( "LDAP naming context test successful" ),
				LdapConfiguration::tr( "The LDAP naming context has been queried successfully. "
									   "The following base DN was found:\n%1" ).
				arg( baseDn ) };
	}

	return bindTestResult;
}



LdapConfigurationTest::Result LdapConfigurationTest::testUserTree()
{
	const auto bindTestResult = testBind();
	if( bindTestResult )
	{
		vDebug() << "[TEST][LDAP] Testing user tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.users().count();

		return reportLdapTreeQueryResult( LdapConfiguration::tr( "user tree" ), count, LdapConfiguration::tr( "User tree" ),
								   ldapDirectory.client().errorDescription() );
	}

	return bindTestResult;
}



LdapConfigurationTest::Result LdapConfigurationTest::testGroupTree()
{
	const auto bindTestResult = testBind();
	if( bindTestResult )
	{
		vDebug() << "[TEST][LDAP] Testing group tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		const auto count = ldapDirectory.groups().count();

		return reportLdapTreeQueryResult( LdapConfiguration::tr( "group tree" ), count, LdapConfiguration::tr( "Group tree" ),
										  ldapDirectory.client().errorDescription() );
	}

	return bindTestResult;
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerTree()
{
	const auto bindTestResult = testBind();
	if( bindTestResult )
	{
		vDebug() << "[TEST][LDAP] Testing computer tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.computersByHostName().count();

		return reportLdapTreeQueryResult( LdapConfiguration::tr( "computer tree" ), count,
										  LdapConfiguration::tr( "Computer tree" ),
										  ldapDirectory.client().errorDescription() );
	}

	return bindTestResult;
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerGroupTree()
{
	const auto bindTestResult = testBind();
	if( bindTestResult )
	{
		vDebug() << "[TEST][LDAP] Testing computer group tree";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableAttributes();
		ldapDirectory.disableFilters();
		int count = ldapDirectory.computerGroups().count();

		return reportLdapTreeQueryResult( LdapConfiguration::tr( "computer group tree" ), count,
										  LdapConfiguration::tr( "Computer group tree" ),
										  ldapDirectory.client().errorDescription() );
	}

	return bindTestResult;
}



LdapConfigurationTest::Result LdapConfigurationTest::testUserLoginNameAttribute( const QString& userFilter  )
{
	if( userFilter.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing user login attribute for" << userFilter;

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		return reportLdapObjectQueryResults( LdapConfiguration::tr( "user objects" ),
											 { LdapConfiguration::tr( "User login name attribute" ) },
											 ldapDirectory.users( userFilter ), ldapDirectory );
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testGroupMemberAttribute( const QString& groupFilter )
{
	if( groupFilter.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing group member attribute for" << groupFilter;

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		QStringList groups = ldapDirectory.groups( groupFilter );

		if( groups.isEmpty() == false )
		{
			return reportLdapObjectQueryResults( LdapConfiguration::tr( "group members" ),
												 { LdapConfiguration::tr( "Group member attribute") },
												 ldapDirectory.groupMembers( groups.first() ), ldapDirectory );
		}

		return { false,
				LdapConfiguration::tr( "Group not found"),
				LdapConfiguration::tr( "Could not find a group with the name \"%1\". "
									   "Please check the group name or the group "
									   "tree parameter.").arg( groupFilter ) };
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerDisplayNameAttribute( const QString& computerName )
{
	if( computerName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer display name attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		return reportLdapObjectQueryResults( LdapConfiguration::tr( "computer objects" ),
											 { LdapConfiguration::tr( "Computer display name attribute" ) },
											 ldapDirectory.computersByDisplayName( computerName ), ldapDirectory );
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerHostNameAttribute( const QString& computerName )
{
	if( computerName.isEmpty() == false )
	{
		if( m_configuration.computerHostNameAsFQDN() &&
			computerName.contains( QLatin1Char('.') ) == false )
		{
			return { false,
					LdapConfiguration::tr( "Invalid hostname" ),
					LdapConfiguration::tr( "You configured computer hostnames to be stored "
										   "as fully qualified domain names (FQDN) but entered "
										   "a hostname without domain." ) };
		}

		if( m_configuration.computerHostNameAsFQDN() == false &&
			computerName.contains( QLatin1Char('.') ) )
		{
			return { false,
					LdapConfiguration::tr( "Invalid hostname" ),
					LdapConfiguration::tr( "You configured computer hostnames to be stored "
										   "as simple hostnames without a domain name but "
										   "entered a hostname with a domain name part." ) };
		}

		vDebug() << "[TEST][LDAP] Testing computer hostname attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		return reportLdapObjectQueryResults( LdapConfiguration::tr( "computer objects" ),
											 { LdapConfiguration::tr( "Computer hostname attribute" ) },
											 ldapDirectory.computersByHostName( computerName ), ldapDirectory );
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerMacAddressAttribute( const QString& computerDn )
{
	if( computerDn.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer MAC address attribute";

		LdapDirectory ldapDirectory( m_configuration );
		ldapDirectory.disableFilters();

		const auto  macAddress = ldapDirectory.computerMacAddress( computerDn );

		return reportLdapObjectQueryResults( LdapConfiguration::tr( "computer MAC addresses" ),
											 { LdapConfiguration::tr( "Computer MAC address attribute" ) },
											 macAddress.isEmpty() ? QStringList() : QStringList( macAddress ),
											 ldapDirectory );
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerLocationAttribute( const QString& locationName )
{
	if( locationName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer location attribute for" << locationName;

		LdapDirectory ldapDirectory( m_configuration );

		return reportLdapObjectQueryResults( LdapConfiguration::tr( "computer locations" ),
											 { LdapConfiguration::tr( "Computer location attribute" ) },
											 ldapDirectory.computerLocations( locationName ), ldapDirectory );
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testLocationNameAttribute( const QString& locationName )
{
	if( locationName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing location name attribute for" << locationName;

		LdapDirectory ldapDirectory( m_configuration );

		return reportLdapObjectQueryResults( LdapConfiguration::tr( "computer locations" ),
											 { LdapConfiguration::tr( "Location name attribute" ) },
											 ldapDirectory.computerLocations( locationName ), ldapDirectory );
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testUsersFilter()
{
	vDebug() << "[TEST][LDAP] Testing users filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.users().count();

	return reportLdapFilterTestResult( LdapConfiguration::tr( "users" ), count, ldapDirectory.client().errorDescription() );
}



LdapConfigurationTest::Result LdapConfigurationTest::testUserGroupsFilter()
{
	vDebug() << "[TEST][LDAP] Testing user groups filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.userGroups().count();

	return reportLdapFilterTestResult( LdapConfiguration::tr( "user groups" ), count, ldapDirectory.client().errorDescription() );
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputersFilter()
{
	vDebug() << "[TEST][LDAP] Testing computers filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.computersByHostName().count();

	return reportLdapFilterTestResult( LdapConfiguration::tr( "computers" ), count, ldapDirectory.client().errorDescription() );
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerGroupsFilter()
{
	vDebug() << "[TEST][LDAP] Testing computer groups filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.computerGroups().count();

	return reportLdapFilterTestResult( LdapConfiguration::tr( "computer groups" ), count, ldapDirectory.client().errorDescription() );
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerContainersFilter()
{
	vDebug() << "[TEST][LDAP] Testing computer containers filter";

	LdapDirectory ldapDirectory( m_configuration );
	const auto count = ldapDirectory.computerLocations().count();

	return reportLdapFilterTestResult( LdapConfiguration::tr( "computer containers" ), count, ldapDirectory.client().errorDescription() );
}



LdapConfigurationTest::Result LdapConfigurationTest::testGroupsOfUser( const QString& username )
{
	if( username.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing groups of user" << username;

		LdapDirectory ldapDirectory( m_configuration );

		const auto userObjects = ldapDirectory.users(username);

		if( userObjects.isEmpty() == false )
		{
			return reportLdapObjectQueryResults( LdapConfiguration::tr( "groups of user" ),
												 { LdapConfiguration::tr("User login name attribute"),
												  LdapConfiguration::tr("Group member attribute") },
												 ldapDirectory.groupsOfUser( userObjects.first() ), ldapDirectory );
		}

		return { false,
				LdapConfiguration::tr( "User not found" ),
				LdapConfiguration::tr( "Could not find a user with the name \"%1\". Please check the username "
									   "or the user tree parameter.").arg( username ) };
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testGroupsOfComputer( const QString& computerHostName )
{
	if( computerHostName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing groups of computer for" << computerHostName;

		LdapDirectory ldapDirectory( m_configuration );

		QStringList computerObjects = ldapDirectory.computersByHostName(computerHostName);

		if( computerObjects.isEmpty() == false )
		{
			return reportLdapObjectQueryResults( LdapConfiguration::tr( "groups of computer" ),
												 { LdapConfiguration::tr( "Computer hostname attribute" ),
												  LdapConfiguration::tr( "Group member attribute" ) },
												 ldapDirectory.groupsOfComputer( computerObjects.first() ), ldapDirectory );
		}

		return { false,
				LdapConfiguration::tr( "Computer not found" ),
				LdapConfiguration::tr( "Could not find a computer with the hostname \"%1\". "
									   "Please check the hostname or the computer tree "
									   "parameter.").arg( computerHostName ) };
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testComputerObjectByIpAddress( const QString& computerIpAddress )
{
	if( computerIpAddress.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing computer object resolve by IP address" << computerIpAddress;

		LdapDirectory ldapDirectory( m_configuration );

		QString computerName = ldapDirectory.hostToLdapFormat( computerIpAddress );

		vDebug() << "[TEST][LDAP] Resolved IP address to computer name" << computerName;

		if( computerName.isEmpty() == false )
		{
			return reportLdapObjectQueryResults( LdapConfiguration::tr( "computers" ),
												 { LdapConfiguration::tr( "Computer hostname attribute" ) },
												 ldapDirectory.computersByHostName( computerName ), ldapDirectory );
		}

		return { false,
				LdapConfiguration::tr( "Hostname lookup failed" ),
				LdapConfiguration::tr( "Could not lookup hostname for IP address %1. "
									   "Please check your DNS server settings." ).arg( computerIpAddress ) };
	}

	return invalidTestValueSuppliedResult();
}



LdapConfigurationTest::Result LdapConfigurationTest::testLocationEntries( const QString& locationName )
{
	if( locationName.isEmpty() == false )
	{
		vDebug() << "[TEST][LDAP] Testing location entries for" << locationName;

		LdapDirectory ldapDirectory( m_configuration );
		return reportLdapObjectQueryResults( LdapConfiguration::tr( "location entries" ),
											 { LdapConfiguration::tr( "Computer groups filter" ),
											  LdapConfiguration::tr( "Computer locations identification") },
											 ldapDirectory.computerLocationEntries( locationName ), ldapDirectory );
	}

	return invalidTestValueSuppliedResult();

}



LdapConfigurationTest::Result LdapConfigurationTest::testLocations()
{
	vDebug() << "[TEST][LDAP] Querying all locations";

	LdapDirectory ldapDirectory( m_configuration );
	return reportLdapObjectQueryResults( LdapConfiguration::tr( "location entries" ),
										 { LdapConfiguration::tr( "Filter for computer groups" ),
										  LdapConfiguration::tr( "Computer locations identification") },
										 ldapDirectory.computerLocations(), ldapDirectory );
}



LdapConfigurationTest::Result LdapConfigurationTest::invalidTestValueSuppliedResult()
{
	return { false,
			LdapConfiguration::tr( "Invalid test value"),
			LdapConfiguration::tr("An empty or invalid value has been supplied for this test.") };
}



LdapConfigurationTest::Result LdapConfigurationTest::reportLdapTreeQueryResult( const QString& name, int count,
													  const QString& parameter, const QString& errorDescription )
{
	if( count <= 0 )
	{
		return { false,
				LdapConfiguration::tr( "LDAP %1 test failed").arg( name ),
				LdapConfiguration::tr( "Could not query any entries in configured %1. "
									   "Please check the parameter \"%2\".\n\n"
									   "%3" ).arg( name, parameter, errorDescription ) };
	}

	return { true,
			LdapConfiguration::tr( "LDAP %1 test successful" ).arg( name ),
			LdapConfiguration::tr( "The %1 has been queried successfully and "
								   "%2 entries were found." ).arg( name ).arg( count ) };
}



LdapConfigurationTest::Result LdapConfigurationTest::reportLdapObjectQueryResults( const QString &objectsName,
														 const QStringList& parameterNames,
														 const QStringList& results, const LdapDirectory &directory )
{
	if( results.isEmpty() )
	{
		QStringList parameters;
		parameters.reserve( parameterNames.count() );

		for( const auto& parameterName : parameterNames )
		{
			parameters += QStringLiteral("\"%1\"").arg( parameterName );
		}

		return { false,
				LdapConfiguration::tr( "LDAP test failed"),
				LdapConfiguration::tr( "Could not query any %1. "
									   "Please check the parameter(s) %2 and enter the name of an existing object.\n\n"
									   "%3" ).arg( objectsName, parameters.join( QStringLiteral(" %1 ")
															.arg( LdapConfiguration::tr("and") ) ),
						  directory.client().errorDescription() ) };
	}

	return { true,
			LdapConfiguration::tr( "LDAP test successful" ),
			LdapConfiguration::tr( "%1 %2 have been queried successfully:\n\n%3" ).arg( results.count() )
				.arg( objectsName, formatResultsString( results ) ) };
}





LdapConfigurationTest::Result LdapConfigurationTest::reportLdapFilterTestResult( const QString &filterObjects, int count,
													   const QString &errorDescription )
{
	if( count <= 0 )
	{
		return { false,
				LdapConfiguration::tr( "LDAP filter test failed"),
				LdapConfiguration::tr( "Could not query any %1 using the configured filter. "
									   "Please check the LDAP filter for %1.\n\n"
									   "%2" ).arg( filterObjects, errorDescription ) };
	}

	return { true,
			LdapConfiguration::tr( "LDAP filter test successful" ),
			LdapConfiguration::tr( "%1 %2 have been queried successfully using the configured filter." )
				.arg( count ).arg( filterObjects ) };
}



QString LdapConfigurationTest::formatResultsString( const QStringList &results )
{
	static constexpr auto FirstResult = 0;
	static constexpr auto MaxResults = 3;

	if( results.count() <= MaxResults )
	{
		return results.join(QLatin1Char('\n'));
	}

	return QStringLiteral( "%1\n[...]" ).arg( results.mid( FirstResult, MaxResults ).join( QLatin1Char('\n') ) );
}
