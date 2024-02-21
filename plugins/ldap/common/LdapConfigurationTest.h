// Copyright (c) 2019-2024 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

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
