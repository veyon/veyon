/*
 * AccessControlProvider.h - declaration of class AccessControlProvider
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

class AccessControlProvider
{
public:
	AccessControlProvider();

	QStringList userGroups();
	QStringList computerGroups();
	QStringList computerPools();

	bool processAuthorizedGroups( const QString& accessingUser );

	AccessControlRule::Action processAccessControlRules( const QString& accessingUser,
														 const QString& accessingComputer,
														 const QString& localUser,
														 const QString& localComputer );


private:
	QStringList groupsOfUser( const QString& userName );

	bool isMemberOfGroup( AccessControlRule::EntityType entityType, const QString& entity, const QString& groupName );
	bool isMemberOfComputerPool( AccessControlRule::EntityType entityType, const QString& entity, const QString& computerPoolName );
	bool hasGroupsInCommon( AccessControlRule::EntityType entityOneType, const QString& entityOne,
							AccessControlRule::EntityType entityTwoType, const QString& entityTwo );
	bool hasComputerPoolsInCommon( AccessControlRule::EntityType entityOneType, const QString& entityOne,
								   AccessControlRule::EntityType entityTwoType, const QString& entityTwo );

	QString lookupEntity( AccessControlRule::Entity entity,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer );

	QString ldapObjectOfEntity( AccessControlRule::EntityType entityType, const QString& entity );
	QStringList ldapGroupsOfEntity( AccessControlRule::EntityType entityType, const QString& entity );
	QStringList ldapComputerPoolsOfEntity( AccessControlRule::EntityType entityType, const QString& entity );

	bool matchConditions( const AccessControlRule& rule,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer);

	LdapDirectory m_ldapDirectory;
	QList<AccessControlRule> m_accessControlRules;

} ;

#endif
