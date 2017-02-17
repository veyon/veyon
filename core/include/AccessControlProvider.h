/*
 * AccessControlProvider.h - declaration of class AccessControlProvider
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef ACCESS_CONTROL_PROVIDER_H
#define ACCESS_CONTROL_PROVIDER_H

#include "Ldap/LdapDirectory.h"
#include "AccessControlRule.h"

class ITALC_CORE_EXPORT AccessControlProvider
{
public:
	typedef enum AccessResults {
		AccessDeny,
		AccessAllow,
		AccessToBeConfirmed,
		AccessResultCount
	} AccessResult;

	AccessControlProvider();

	QStringList userGroups();
	QStringList computerGroups();
	QStringList computerLabs();

	AccessResult checkAccess( QString accessingUser, QString accessingComputer );

	bool processAuthorizedGroups( const QString& accessingUser );

	AccessControlRule::Action processAccessControlRules( const QString& accessingUser,
														 const QString& accessingComputer,
														 const QString& localUser,
														 const QString& localComputer );

	bool isAccessDeniedByLocalState();


private:
	QStringList groupsOfUser( const QString& userName );

	bool isMemberOfGroup( AccessControlRule::EntityType entityType, const QString& entity, const QString& groupName );
	bool isLocatedInComputerLab( AccessControlRule::EntityType entityType, const QString& entity, const QString& computerLabName );
	bool hasGroupsInCommon( AccessControlRule::EntityType entityOneType, const QString& entityOne,
							AccessControlRule::EntityType entityTwoType, const QString& entityTwo );
	bool isLocatedInSameComputerLab( AccessControlRule::EntityType entityOneType, const QString& entityOne,
									 AccessControlRule::EntityType entityTwoType, const QString& entityTwo );
	bool isLocalHost( const QString& accessingComputer ) const;
	bool isLocalUser( const QString& accessingUser, const QString& localUser ) const;

	QString lookupEntity( AccessControlRule::Entity entity,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer );

	QString ldapObjectOfEntity( AccessControlRule::EntityType entityType, const QString& entity );
	QStringList ldapGroupsOfEntity( AccessControlRule::EntityType entityType, const QString& entity );
	QStringList ldapComputerLabsOfEntity( AccessControlRule::EntityType entityType, const QString& entity );

	bool matchConditions( const AccessControlRule& rule,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer);

	LdapDirectory m_ldapDirectory;
	QList<AccessControlRule> m_accessControlRules;

} ;

#endif
