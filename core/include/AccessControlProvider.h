/*
 * AccessControlProvider.h - declaration of class AccessControlProvider
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef ACCESS_CONTROL_PROVIDER_H
#define ACCESS_CONTROL_PROVIDER_H

#include "AccessControlRule.h"

class AccessControlDataBackendInterface;

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
	QStringList rooms() const;

	AccessResult checkAccess( QString accessingUser, QString accessingComputer,
							  const QStringList& connectedUsers );

	bool processAuthorizedGroups( const QString& accessingUser );

	AccessControlRule::Action processAccessControlRules( const QString& accessingUser,
														 const QString& accessingComputer,
														 const QString& localUser,
														 const QString& localComputer,
														 const QStringList& connectedUsers );

	bool isAccessDeniedByLocalState() const;


private:
	bool isMemberOfUserGroup( const QString& user, const QString& groupName ) const;
	bool isLocatedInRoom( const QString& computer, const QString& roomName ) const;
	bool hasGroupsInCommon( const QString& userOne, const QString& userTwo ) const;
	bool isLocatedInSameRoom( const QString& computerOne, const QString& computerTwo ) const;
	bool isLocalHost( const QString& accessingComputer ) const;
	bool isLocalUser( const QString& accessingUser, const QString& localUser ) const;

	QString lookupSubject( AccessControlRule::Subject subject,
						   const QString& accessingUser, const QString& accessingComputer,
						   const QString& localUser, const QString& localComputer ) const;

	bool matchConditions( const AccessControlRule& rule,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer,
						  const QStringList& connectedUsers ) const;

	QList<AccessControlRule> m_accessControlRules;
	AccessControlDataBackendInterface* m_dataBackend;

} ;

#endif
