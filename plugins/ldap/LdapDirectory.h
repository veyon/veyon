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
	LdapDirectory( const LdapConfiguration& configuration, const QUrl& url = QUrl(), QObject* parent = nullptr );
	~LdapDirectory();

	const LdapConfiguration& configuration() const
	{
		return m_configuration;
	}

	const LdapClient& client() const
	{
		return m_client;
	}

	void disableAttributes();
	void disableFilters();

	QStringList users( const QString& filterValue = QString() );
	QStringList groups( const QString& filterValue = QString() );
	QStringList userGroups( const QString& filterValue = QString() );
	QStringList computers( const QString& filterValue = QString() );
	QStringList computerGroups( const QString& filterValue = QString() );
	QStringList computerRooms( const QString& filterValue = QString() );

	QStringList groupMembers( const QString& groupDn );
	QStringList groupsOfUser( const QString& userDn );
	QStringList groupsOfComputer( const QString& computerDn );
	QStringList computerRoomsOfComputer( const QString& computerDn );

	QString userLoginName( const QString& userDn );
	QString groupName( const QString& groupDn );
	QString computerHostName( const QString& computerDn );
	QString computerMacAddress( const QString& computerDn );
	QString groupMemberUserIdentification( const QString& userDn );
	QString groupMemberComputerIdentification( const QString& computerDn );

	QStringList computerRoomMembers( const QString& computerRoomName );

	QString hostToLdapFormat( const QString& host );
	QString computerObjectFromHost( const QString& host );


private:
	const LdapConfiguration& m_configuration;
	LdapClient m_client;

	KLDAP::LdapUrl::Scope m_defaultSearchScope = KLDAP::LdapUrl::Base;

	//QString m_baseDn;
	//QString namingContextAttribute;
	QString m_usersDn;
	QString m_groupsDn;
	QString m_computersDn;
	QString m_computerGroupsDn;

	QString m_userLoginAttribute;
	QString m_groupMemberAttribute;
	QString m_computerHostNameAttribute;
	QString m_computerMacAddressAttribute;
	QString m_computerRoomNameAttribute;

	QString m_usersFilter;
	QString m_userGroupsFilter;
	QString m_computersFilter;
	QString m_computerGroupsFilter;
	QString m_computerParentsFilter;

	QString m_computerRoomAttribute;

	bool m_identifyGroupMembersByNameAttribute = false;
	bool m_computerRoomMembersByContainer = false;
	bool m_computerRoomMembersByAttribute = false;
	bool m_computerHostNameAsFQDN = false;

};
