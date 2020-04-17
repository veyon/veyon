/*
 * LdapConfigurationTest.h - header for the LdapConfigurationTest class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QObject>

#include "LdapCommon.h"

class LdapConfiguration;
class LdapDirectory;

class LDAP_COMMON_EXPORT LdapConfigurationTest
{
public:
	explicit LdapConfigurationTest( LdapConfiguration& configuration );

	struct Result
	{
		bool success{false};
		QString title;
		QString message;
		operator bool() const
		{
			return success;
		}
	};

public:
	Result testBind();
	Result testBaseDn();
	Result testNamingContext();
	Result testUserTree();
	Result testGroupTree();
	Result testComputerTree();
	Result testComputerGroupTree();
	Result testUserLoginNameAttribute( const QString& userFilter );
	Result testGroupMemberAttribute( const QString& groupFilter );
	Result testComputerDisplayNameAttribute( const QString& computerName );
	Result testComputerHostNameAttribute( const QString& computerName );
	Result testComputerMacAddressAttribute( const QString& computerDn );
	Result testComputerLocationAttribute( const QString& locationName );
	Result testLocationNameAttribute( const QString& locationName );
	Result testUsersFilter();
	Result testUserGroupsFilter();
	Result testComputersFilter();
	Result testComputerGroupsFilter();
	Result testComputerContainersFilter();
	Result testGroupsOfUser( const QString& username );
	Result testGroupsOfComputer( const QString& computerHostName );
	Result testComputerObjectByIpAddress( const QString& computerIpAddress );
	Result testLocationEntries( const QString& locationName );
	Result testLocations();

	static Result invalidTestValueSuppliedResult();

	Result reportLdapTreeQueryResult( const QString& name, int count, const QString& parameter,
								   const QString& errorDescription );
	Result reportLdapObjectQueryResults( const QString &objectsName, const QStringList& parameterNames,
									  const QStringList &results, const LdapDirectory& directory );
	Result reportLdapFilterTestResult( const QString &filterObjects, int count, const QString &errorDescription );

	static QString formatResultsString( const QStringList& results );

	LdapConfiguration& m_configuration;

};
