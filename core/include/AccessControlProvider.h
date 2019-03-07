/*
 * AccessControlProvider.h - declaration of class AccessControlProvider
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

#include "AccessControlRule.h"
#include "NetworkObject.h"

class UserGroupsBackendInterface;
class NetworkObjectDirectory;

class VEYON_CORE_EXPORT AccessControlProvider
{
public:
	typedef enum AccessResults {
		AccessDeny,
		AccessAllow,
		AccessToBeConfirmed,
		AccessResultCount
	} AccessResult;

	AccessControlProvider();

	QStringList userGroups() const;
	QStringList locations() const;
	QStringList locationsOfComputer( const QString& computer ) const;

	AccessResult checkAccess( const QString& accessingUser, const QString& accessingComputer,
							  const QStringList& connectedUsers );

	bool processAuthorizedGroups( const QString& accessingUser );

	AccessControlRule::Action processAccessControlRules( const QString& accessingUser,
														 const QString& accessingComputer,
														 const QString& localUser,
														 const QString& localComputer,
														 const QStringList& connectedUsers );

	bool isAccessToLocalComputerDenied() const;

private:
	bool isMemberOfUserGroup( const QString& user, const QString& groupName ) const;
	bool isLocatedAt( const QString& computer, const QString& locationName ) const;
	bool haveGroupsInCommon( const QString& userOne, const QString& userTwo ) const;
	bool haveSameLocations( const QString& computerOne, const QString& computerTwo ) const;
	bool isLocalHost( const QString& accessingComputer ) const;
	bool isLocalUser( const QString& accessingUser, const QString& localUser ) const;
	bool isNoUserLoggedOn() const;

	QString lookupSubject( AccessControlRule::Subject subject,
						   const QString& accessingUser, const QString& accessingComputer,
						   const QString& localUser, const QString& localComputer ) const;

	bool matchConditions( const AccessControlRule& rule,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer,
						  const QStringList& connectedUsers ) const;

	static QStringList objectNames( const NetworkObjectList& objects );

	QList<AccessControlRule> m_accessControlRules;
	UserGroupsBackendInterface* m_userGroupsBackend;
	NetworkObjectDirectory* m_networkObjectDirectory;
	bool m_queryDomainGroups;

} ;
