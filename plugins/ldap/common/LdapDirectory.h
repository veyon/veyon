/*
 * LdapDirectory.h - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "LdapClient.h"
#include "VeyonCore.h"

class LdapConfiguration;
class LdapClient;

class LdapDirectory : public QObject
{
	Q_OBJECT
public:
	LdapDirectory( const LdapConfiguration& configuration, QObject* parent = nullptr );
	~LdapDirectory() = default;

	const QString& configInstanceId() const;

	const LdapClient& client() const
	{
		return m_client;
	}

	LdapClient& client()
	{
		return m_client;
	}

	void disableAttributes();
	void disableFilters();

	QStringList users( const QString& filterValue = QString() );
	QStringList groups( const QString& filterValue = QString() );
	QStringList userGroups( const QString& filterValue = QString() );
	QStringList computersByDisplayName( const QString& filterValue = QString() );
	QStringList computersByHostName( const QString& filterValue = QString() );
	QStringList computerGroups( const QString& filterValue = QString() );
	QStringList computerLocations( const QString& filterValue = QString() );

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

	const QString& computersDn() const
	{
		return m_computersDn;
	}

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
