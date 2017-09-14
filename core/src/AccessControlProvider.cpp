/*
 * AccessControlProvider.cpp - implementation of the AccessControlProvider class
 *
 * Copyright (c) 2016-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QDebug>
#include <QHostInfo>

#include "AccessControlDataBackendManager.h"
#include "AccessControlProvider.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "LocalSystem.h"
#include "PlatformPluginInterface.h"
#include "PlatformUserInfoFunctions.h"


AccessControlProvider::AccessControlProvider() :
	m_accessControlRules(),
	m_dataBackend( VeyonCore::accessControlDataBackendManager().configuredBackend() )
{
	const QJsonArray accessControlRules = VeyonCore::config().accessControlRules();

	m_accessControlRules.reserve( accessControlRules.size() );

	for( const auto& accessControlRule : accessControlRules )
	{
		m_accessControlRules.append( accessControlRule );
	}
}



QStringList AccessControlProvider::userGroups() const
{
	auto userGroupList = m_dataBackend->userGroups();

	std::sort( userGroupList.begin(), userGroupList.end() );

	return userGroupList;
}



QStringList AccessControlProvider::rooms() const
{
	auto roomList = m_dataBackend->allRooms();

	std::sort( roomList.begin(), roomList.end() );

	return roomList;
}



AccessControlProvider::AccessResult AccessControlProvider::checkAccess( const QString& accessingUser,
																		const QString& accessingComputer,
																		const QStringList& connectedUsers )
{
	if( VeyonCore::config().isAccessRestrictedToUserGroups() )
	{
		if( processAuthorizedGroups( accessingUser ) )
		{
			return AccessAllow;
		}
	}
	else if( VeyonCore::config().isAccessControlRulesProcessingEnabled() )
	{
		auto action = processAccessControlRules( accessingUser,
												 accessingComputer,
												 LocalSystem::User::loggedOnUser().name(),
												 QHostInfo::localHostName(),
												 connectedUsers );
		switch( action )
		{
		case AccessControlRule::ActionAllow:
			return AccessAllow;
		case AccessControlRule::ActionAskForPermission:
			return AccessToBeConfirmed;
		default: break;
		}
	}
	else
	{
		qDebug( "AccessControlProvider::checkAccess(): "
				"no access control method configured, allowing access." );

		// no access control method configured, therefore grant access
		return AccessAllow;
	}

	qDebug( "AccessControlProvider::checkAccess(): "
			"configured access control method did not succeed, denying access." );

	// configured access control method did not succeed, therefore deny access
	return AccessDeny;
}



bool AccessControlProvider::processAuthorizedGroups(const QString &accessingUser)
{
	qDebug() << "AccessControlProvider::processAuthorizedGroups(): processing for user" << accessingUser;

	return intersects( m_dataBackend->groupsOfUser( accessingUser ).toSet(),
						VeyonCore::config().authorizedUserGroups().toSet() );
}



AccessControlRule::Action AccessControlProvider::processAccessControlRules( const QString& accessingUser,
																			const QString& accessingComputer,
																			const QString& localUser,
																			const QString& localComputer,
																			const QStringList& connectedUsers )
{
	qDebug() << "AccessControlProvider::processAccessControlRules(): processing rules for"
			 << accessingUser << accessingComputer << localUser << localComputer << connectedUsers;

	for( const auto& rule : qAsConst( m_accessControlRules ) )
	{
		// rule disabled?
		if( rule.action() == AccessControlRule::ActionNone )
		{
			// then continue with next rule
			continue;
		}

		if( rule.areConditionsIgnored() ||
				matchConditions( rule, accessingUser, accessingComputer, localUser, localComputer, connectedUsers ) )
		{
			qDebug() << "AccessControlProvider::processAccessControlRules(): rule"
					 << rule.name() << "matched with action" << rule.action();
			return rule.action();
		}
	}

	qDebug() << "AccessControlProvider::processAccessControlRules(): no matching rule, denying access";

	return AccessControlRule::ActionDeny;
}


/*!
 * \brief Returns whether any incoming access requests would be denied due to a deny rule matching the local state (e.g. teacher logged on)
 */
bool AccessControlProvider::isAccessToLocalComputerDenied() const
{
	if( VeyonCore::config().isAccessControlRulesProcessingEnabled() == false )
	{
		return false;
	}

	for( const auto& rule : qAsConst( m_accessControlRules ) )
	{
		if( rule.action() == AccessControlRule::ActionDeny &&
				matchConditions( rule, QString(), QString(),
								 LocalSystem::User::loggedOnUser().name(),
								 QHostInfo::localHostName(),
								 QStringList() ) )
		{
			return true;
		}
	}

	return false;
}



bool AccessControlProvider::isMemberOfUserGroup( const QString &user,
												 const QString &groupName ) const
{
	QRegExp groupNameRX( groupName );

	if( groupNameRX.isValid() )
	{
		return m_dataBackend->groupsOfUser( user ).indexOf( QRegExp( groupName ) ) >= 0;
	}

	return m_dataBackend->groupsOfUser( user ).contains( groupName );
}



bool AccessControlProvider::isLocatedInRoom( const QString &computer, const QString &roomName ) const
{
	return m_dataBackend->roomsOfComputer( computer ).contains( roomName );
}



bool AccessControlProvider::hasGroupsInCommon( const QString &userOne, const QString &userTwo ) const
{
	const auto userOneGroups = m_dataBackend->groupsOfUser( userOne );
	const auto userTwoGroups = m_dataBackend->groupsOfUser( userOne );

	return intersects( userOneGroups.toSet(), userTwoGroups.toSet() );
}



bool AccessControlProvider::isLocatedInSameRoom( const QString &computerOne, const QString &computerTwo ) const
{
	const auto computerOneRooms = m_dataBackend->roomsOfComputer( computerOne );
	const auto computerTwoRooms = m_dataBackend->roomsOfComputer( computerTwo );

	return intersects( computerOneRooms.toSet(), computerTwoRooms.toSet() );
}



bool AccessControlProvider::isLocalHost( const QString &accessingComputer ) const
{
	return QHostAddress( accessingComputer ).isLoopback();
}



bool AccessControlProvider::isLocalUser( const QString &accessingUser, const QString &localUser ) const
{
	return accessingUser == localUser;
}



bool AccessControlProvider::isNoUserLoggedOn() const
{
	return VeyonCore::platform().userInfoFunctions()->loggedOnUsers().isEmpty();
}



QString AccessControlProvider::lookupSubject( AccessControlRule::Subject subject,
											  const QString &accessingUser, const QString &accessingComputer,
											  const QString &localUser, const QString &localComputer ) const
{
	switch( subject )
	{
	case AccessControlRule::SubjectAccessingUser: return accessingUser;
	case AccessControlRule::SubjectAccessingComputer: return accessingComputer;
	case AccessControlRule::SubjectLocalUser: return localUser;
	case AccessControlRule::SubjectLocalComputer: return localComputer;
	default: break;
	}

	return QString();
}



bool AccessControlProvider::matchConditions( const AccessControlRule &rule,
											 const QString& accessingUser, const QString& accessingComputer,
											 const QString& localUser, const QString& localComputer,
											 const QStringList& connectedUsers ) const
{
	bool hasConditions = false;

	// normally all selected conditions have to match in order to make the whole rule match
	// if conditions should be inverted (i.e. "is member of" is to be interpreted as "is NOT member of")
	// we have to check against the opposite boolean value
	bool matchResult = rule.areConditionsInverted() == false;

	qDebug() << "AccessControlProvider::matchConditions():" << rule.toJson() << matchResult;

	auto condition = AccessControlRule::ConditionMemberOfUserGroup;

	if( rule.isConditionEnabled( condition ) )
	{
		hasConditions = true;

		const auto user = lookupSubject( rule.subject( condition ), accessingUser, QString(), localUser, QString() );
		const auto group = rule.argument( condition );

		if( user.isEmpty() || group.isEmpty() ||
				isMemberOfUserGroup( user, group ) != matchResult )
		{
			return false;
		}
	}

	condition = AccessControlRule::ConditionGroupsInCommon;

	if( rule.isConditionEnabled( condition ) )
	{
		hasConditions = true;

		if( accessingUser.isEmpty() || localUser.isEmpty() ||
				hasGroupsInCommon( accessingUser, localUser ) != matchResult )
		{
			return false;
		}
	}

	condition = AccessControlRule::ConditionLocatedInRoom;

	if( rule.isConditionEnabled( condition ) )
	{
		hasConditions = true;

		const auto computer = lookupSubject( rule.subject( condition ), QString(), accessingComputer, QString(), localComputer );
		const auto room = rule.argument( condition );

		if( computer.isEmpty() || room.isEmpty() ||
				isLocatedInRoom( computer, room ) != matchResult )
		{
			return false;
		}
	}


	condition = AccessControlRule::ConditionLocatedInSameRoom;

	if( rule.isConditionEnabled( condition ) )
	{
		hasConditions = true;

		if( accessingComputer.isEmpty() || localComputer.isEmpty() ||
				isLocatedInSameRoom( accessingComputer, localComputer ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromLocalHost ) )
	{
		hasConditions = true;

		if( isLocalHost( accessingComputer ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromLocalUser ) )
	{
		hasConditions = true;

		if( isLocalUser( accessingUser, localUser ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::ConditionAccessFromAlreadyConnectedUser ) )
	{
		hasConditions = true;

		if( connectedUsers.contains( accessingUser ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::ConditionNoUserLoggedOn ) )
	{
		hasConditions = true;

		if( isNoUserLoggedOn() != matchResult )
		{
			return false;
		}
	}

	// do not match the rule if no conditions are set at all
	if( hasConditions == false )
	{
		return false;
	}

	return true;
}
