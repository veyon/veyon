// Copyright (c) 2019-2024 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "LdapClient.h"
#include "LdapCommon.h"
#include "VeyonCore.h"

class LdapConfiguration;
class LdapClient;

class LDAP_COMMON_EXPORT LdapDirectory : public QObject
{
	Q_OBJECT
public:
	explicit LdapDirectory( const LdapConfiguration& configuration, QObject* parent = nullptr );
	~LdapDirectory() override = default;

	const QString& configInstanceId() const;

	const LdapClient& client() const
	{
		return m_client;
	}

	LdapClient& client()
	{
		return m_client;
	}

	QString usersDn();
	QString groupsDn();
	QString computersDn();
	QString computerGroupsDn();

	void disableAttributes();
	void disableFilters();

	QStringList users( const QString& filterValue = {} );
	QStringList groups( const QString& filterValue = {} );
	QStringList userGroups( const QString& filterValue = {} );
	QStringList computersByDisplayName( const QString& filterValue = {} );
	QStringList computersByHostName( const QString& filterValue = {} );
	QStringList computerGroups( const QString& filterValue = {} );
	QStringList computerLocations( const QString& filterValue = {} );

	QStringList groupMembers( const QString& groupDn );
	QStringList groupsOfUser( const QString& userDn );
	QStringList groupsOfComputer( const QString& computerDn );
	QStringList locationsOfComputer( const QString& computerDn );

	QString userLoginName( const QString& userDn );
	QString computerDisplayName( const QString& computerDn );
	QString computerHostName( const QString& computerDn );
	QString computerMacAddress( const QString& computerDn );
	QString groupMemberUserIdentification( const QString& userDn );
	QString groupMemberComputerIdentification( const QString& computerDn );

	QStringList computerLocationEntries( const QString& locationName );

	QString hostToLdapFormat( const QString& host );
	QString computerObjectFromHost( const QString& host );

	const QString& computersFilter() const
	{
		return m_computersFilter;
	}

	const QString& computerContainersFilter() const
	{
		return m_computerContainersFilter;
	}

	const QString& locationNameAttribute() const
	{
		return m_locationNameAttribute;
	}

	const QString& computerDisplayNameAttribute() const
	{
		return m_computerDisplayNameAttribute;
	}

	const QString& computerHostNameAttribute() const
	{
		return m_computerHostNameAttribute;
	}

	const QString& computerMacAddressAttribute() const
	{
		return m_computerMacAddressAttribute;
	}

	bool computerLocationsByContainer() const
	{
		return m_computerLocationsByContainer;
	}

private:
	LdapClient::Scope computerSearchScope() const;

	const LdapConfiguration& m_configuration;
	LdapClient m_client;

	LdapClient::Scope m_defaultSearchScope = LdapClient::Scope::Base;

	QString m_usersDn;
	QString m_groupsDn;
	QString m_computersDn;
	QString m_computerGroupsDn;

	QString m_userLoginNameAttribute;
	QString m_groupMemberAttribute;
	QString m_groupMemberFilterAttribute;
	QString m_computerDisplayNameAttribute;
	QString m_computerHostNameAttribute;
	QString m_computerMacAddressAttribute;
	QString m_locationNameAttribute;

	QString m_usersFilter;
	QString m_userGroupsFilter;
	QString m_computersFilter;
	QString m_computerGroupsFilter;
	QString m_computerContainersFilter;

	QString m_computerLocationAttribute;

	bool m_identifyGroupMembersByNameAttribute = false;
	bool m_computerLocationsByContainer = false;
	bool m_computerLocationsByAttribute = false;
	bool m_computerHostNameAsFQDN = false;

};
