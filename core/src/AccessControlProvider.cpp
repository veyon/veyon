/*
 * AccessControlProvider.cpp - implementation of the AccessControlProvider class
 *
 * Copyright (c) 2016-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QNetworkInterface>
#include <QRegularExpression>

#include "UserGroupsBackendManager.h"
#include "AccessControlProvider.h"
#include "HostAddress.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "PlatformPluginInterface.h"
#include "PlatformSessionFunctions.h"
#include "PlatformUserFunctions.h"


AccessControlProvider::AccessControlProvider() :
	m_accessControlRules(),
	m_userGroupsBackend(VeyonCore::userGroupsBackendManager().configuredBackend()),
	m_networkObjectDirectory(VeyonCore::networkObjectDirectoryManager().configuredDirectory()),
	m_useDomainUserGroups(VeyonCore::config().useDomainUserGroups())
{
	const QJsonArray accessControlRules = VeyonCore::config().accessControlRules();

	m_accessControlRules.reserve( accessControlRules.size() );

	for( const auto& accessControlRule : accessControlRules )
	{
		m_accessControlRules.append( AccessControlRule( accessControlRule ) );
	}
}



QStringList AccessControlProvider::userGroups() const
{
	auto userGroupList = m_userGroupsBackend->userGroups(m_useDomainUserGroups);

	std::sort( userGroupList.begin(), userGroupList.end() );

	return userGroupList;
}



QStringList AccessControlProvider::locations() const
{
	auto locationList = objectNames( m_networkObjectDirectory->queryObjects( NetworkObject::Type::Location,
																			 NetworkObject::Property::None, {} ) );

	std::sort( locationList.begin(), locationList.end() );

	return locationList;
}



QStringList AccessControlProvider::locationsOfComputer( const QString& computer ) const
{
	const auto fqdn = HostAddress( computer ).convert( HostAddress::Type::FullyQualifiedDomainName );

	vDebug() << "Searching for locations of computer" << computer << "via FQDN" << fqdn;

	if( fqdn.isEmpty() )
	{
		vWarning() << "Empty FQDN - returning empty location list";
		return {};
	}

	const auto computers = m_networkObjectDirectory->queryObjects( NetworkObject::Type::Host,
																   NetworkObject::Property::HostAddress, fqdn );
	if( computers.isEmpty() )
	{
		vWarning() << "Could not query any network objects for host" << fqdn;
		return {};
	}

	QStringList locationList;
	locationList.reserve( computers.size()*3 );

	for( const auto& computer : computers )
	{
		const auto parents = m_networkObjectDirectory->queryParents( computer );
		for( const auto& parent : parents )
		{
			locationList.append( parent.name() );
		}
	}

	std::sort( locationList.begin(), locationList.end() );

	vDebug() << "Found locations:" << locationList;

	return locationList;
}



AccessControlProvider::Access AccessControlProvider::checkAccess( const QString& accessingUser,
																  const QString& accessingComputer,
																  const QStringList& connectedUsers,
																  Plugin::Uid authMethodUid )
{
	if( VeyonCore::config().isAccessRestrictedToUserGroups() )
	{
		if( processAuthorizedGroups( accessingUser ) )
		{
			return Access::Allow;
		}
	}
	else if( VeyonCore::config().isAccessControlRulesProcessingEnabled() )
	{
		auto action = processAccessControlRules( accessingUser,
												 accessingComputer,
												 VeyonCore::platform().userFunctions().currentUser(),
												 HostAddress::localFQDN(),
												 connectedUsers,
												 authMethodUid );
		switch( action )
		{
		case AccessControlRule::Action::Allow:
			return Access::Allow;
		case AccessControlRule::Action::AskForPermission:
			return Access::ToBeConfirmed;
		default: break;
		}
	}
	else
	{
		vDebug() << "no access control method configured, allowing access.";

		// no access control method configured, therefore grant access
		return Access::Allow;
	}

	vDebug() << "configured access control method did not succeed, denying access.";

	// configured access control method did not succeed, therefore deny access
	return Access::Deny;
}



bool AccessControlProvider::processAuthorizedGroups( const QString& accessingUser )
{
	vDebug() << "processing for user" << accessingUser;

	const auto groupsOfAccessingUser = m_userGroupsBackend->groupsOfUser( accessingUser, m_useDomainUserGroups );
	const auto authorizedUserGroups = VeyonCore::config().authorizedUserGroups();

	vDebug() << groupsOfAccessingUser << authorizedUserGroups;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	const auto groupsOfAccessingUserSet = QSet<QString>{ groupsOfAccessingUser.begin(), groupsOfAccessingUser.end() };
	const auto authorizedUserGroupSet = QSet<QString>{ authorizedUserGroups.begin(), authorizedUserGroups.end() };
#else
	const auto groupsOfAccessingUserSet = groupsOfAccessingUser.toSet();
	const auto authorizedUserGroupSet = authorizedUserGroups.toSet();
#endif

	return groupsOfAccessingUserSet.intersects( authorizedUserGroupSet );
}



AccessControlRule::Action AccessControlProvider::processAccessControlRules( const QString& accessingUser,
																			const QString& accessingComputer,
																			const QString& localUser,
																			const QString& localComputer,
																			const QStringList& connectedUsers,
																			Plugin::Uid authMethodUid )
{
	vDebug() << "processing rules for" << accessingUser << accessingComputer << localUser << localComputer << connectedUsers << authMethodUid;

	for( const auto& rule : qAsConst( m_accessControlRules ) )
	{
		// rule disabled?
		if( rule.action() == AccessControlRule::Action::None )
		{
			// then continue with next rule
			continue;
		}

		if( rule.areConditionsIgnored() ||
			matchConditions( rule, accessingUser, accessingComputer, localUser, localComputer, connectedUsers, authMethodUid ) )
		{
			vDebug() << "rule" << rule.name() << "matched with action" << rule.action();
			return rule.action();
		}
	}

	vDebug() << "no matching rule, denying access";

	return AccessControlRule::Action::Deny;
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
		if( matchConditions( rule, {}, {},
							 VeyonCore::platform().userFunctions().currentUser(), HostAddress::localFQDN(), {}, {} ) )
		{
			switch( rule.action() )
			{
			case AccessControlRule::Action::Deny:
				return true;
			case AccessControlRule::Action::Allow:
			case AccessControlRule::Action::AskForPermission:
				return false;
			default:
				break;
			}
		}
	}

	return false;
}



bool AccessControlProvider::isMemberOfUserGroup( const QString &user,
												 const QString &groupName ) const
{
	const QRegularExpression groupNameRX( groupName );

	if( groupNameRX.isValid() )
	{
		return m_userGroupsBackend->groupsOfUser( user, m_useDomainUserGroups ).indexOf( groupNameRX ) >= 0;
	}

	return m_userGroupsBackend->groupsOfUser( user, m_useDomainUserGroups ).contains( groupName );
}



bool AccessControlProvider::isLocatedAt( const QString &computer, const QString &locationName ) const
{
	return locationsOfComputer( computer ).contains( locationName );
}



bool AccessControlProvider::haveGroupsInCommon( const QString &userOne, const QString &userTwo ) const
{
	const auto userOneGroups = m_userGroupsBackend->groupsOfUser( userOne, m_useDomainUserGroups );
	const auto userTwoGroups = m_userGroupsBackend->groupsOfUser( userTwo, m_useDomainUserGroups );

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	const auto userOneGroupSet = QSet<QString>{ userOneGroups.begin(), userOneGroups.end() };
	const auto userTwoGroupSet = QSet<QString>{ userTwoGroups.begin(), userTwoGroups.end() };
#else
	const auto userOneGroupSet = userOneGroups.toSet();
	const auto userTwoGroupSet = userTwoGroups.toSet();
#endif

	return userOneGroupSet.intersects( userTwoGroupSet );
}



bool AccessControlProvider::haveSameLocations( const QString &computerOne, const QString &computerTwo ) const
{
	const auto computerOneLocations = locationsOfComputer( computerOne );
	const auto computerTwoLocations = locationsOfComputer( computerTwo );

	return computerOneLocations.isEmpty() == false &&
			computerOneLocations == computerTwoLocations;
}



bool AccessControlProvider::isLocalHost( const QString &accessingComputer ) const
{
	return HostAddress( accessingComputer ).isLocalHost();
}



bool AccessControlProvider::isLocalUser( const QString &accessingUser, const QString &localUser ) const
{
	return accessingUser.isEmpty() == false &&
			accessingUser == localUser;
}



bool AccessControlProvider::isNoUserLoggedInLocally() const
{
	return VeyonCore::platform().userFunctions().isAnyUserLoggedInLocally() == false;
}



bool AccessControlProvider::isNoUserLoggedInRemotely() const
{
	return VeyonCore::platform().userFunctions().isAnyUserLoggedInRemotely() == false;
}



QString AccessControlProvider::lookupSubject( AccessControlRule::Subject subject,
											  const QString &accessingUser, const QString &accessingComputer,
											  const QString &localUser, const QString &localComputer ) const
{
	switch( subject )
	{
	case AccessControlRule::Subject::AccessingUser: return accessingUser;
	case AccessControlRule::Subject::AccessingComputer: return accessingComputer;
	case AccessControlRule::Subject::LocalUser: return localUser;
	case AccessControlRule::Subject::LocalComputer: return localComputer;
	default: break;
	}

	return {};
}



bool AccessControlProvider::matchConditions( const AccessControlRule &rule,
											 const QString& accessingUser, const QString& accessingComputer,
											 const QString& localUser, const QString& localComputer,
											 const QStringList& connectedUsers, Plugin::Uid authMethodUid ) const
{
	vDebug() << rule.toJson();

	AccessControlRule::Condition condition{AccessControlRule::Condition::None};

	if( rule.isConditionEnabled( AccessControlRule::Condition::AuthenticationMethod ) )
	{
		condition = AccessControlRule::Condition::AuthenticationMethod;

		const auto allowedAuthMethod = Plugin::Uid( rule.argument( AccessControlRule::Condition::AuthenticationMethod ) );
		if( authMethodUid.isNull() ||
			allowedAuthMethod.isNull() ||
			( authMethodUid == allowedAuthMethod ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::MemberOfGroup ) )
	{
		condition = AccessControlRule::Condition::MemberOfGroup;

		const auto condition = AccessControlRule::Condition::MemberOfGroup;
		const auto user = lookupSubject( rule.subject( condition ), accessingUser, {}, localUser, {} );
		const auto group = rule.argument( condition );

		if( user.isEmpty() || group.isEmpty() ||
			isMemberOfUserGroup( user, group ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::GroupsInCommon ) )
	{
		condition = AccessControlRule::Condition::GroupsInCommon;

		if( accessingUser.isEmpty() || localUser.isEmpty() ||
			haveGroupsInCommon( accessingUser, localUser ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::LocatedAt ) )
	{
		condition = AccessControlRule::Condition::LocatedAt;

		const auto computer = lookupSubject( rule.subject( condition ), {}, accessingComputer, {}, localComputer );
		const auto location = rule.argument( condition );

		if( computer.isEmpty() || location.isEmpty() ||
			isLocatedAt( computer, location ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::LocationsInCommon ) )
	{
		condition = AccessControlRule::Condition::LocationsInCommon;

		if( accessingComputer.isEmpty() || localComputer.isEmpty() ||
			haveSameLocations( accessingComputer, localComputer ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromLocalHost ) )
	{
		condition = AccessControlRule::Condition::AccessFromLocalHost;

		if( isLocalHost( accessingComputer ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromSameUser ) )
	{
		condition = AccessControlRule::Condition::AccessFromSameUser;

		if( isLocalUser( accessingUser, localUser ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromAlreadyConnectedUser ) )
	{
		condition = AccessControlRule::Condition::AccessFromAlreadyConnectedUser;

		if( connectedUsers.contains( accessingUser ) == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessedUserLoggedInLocally ) )
	{
		condition = AccessControlRule::Condition::AccessedUserLoggedInLocally;

		if( VeyonCore::platform().sessionFunctions().currentSessionIsRemote() == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::NoUserLoggedInLocally ) )
	{
		condition = AccessControlRule::Condition::NoUserLoggedInLocally;

		if( isNoUserLoggedInLocally() == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::NoUserLoggedInRemotely ) )
	{
		condition = AccessControlRule::Condition::NoUserLoggedInRemotely;

		if( isNoUserLoggedInRemotely() == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::UserSession ) )
	{
		condition = AccessControlRule::Condition::UserSession;

		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() == rule.isConditionInverted(condition) )
		{
			return false;
		}
	}

	// do not match the rule if no conditions are set at all
	if( condition == AccessControlRule::Condition::None )
	{
		return false;
	}

	return true;
}



QStringList AccessControlProvider::objectNames( const NetworkObjectList& objects )
{
	QStringList nameList;
	nameList.reserve( objects.size() );

	for( const auto& object : objects )
	{
		nameList.append( object.name() );
	}

	return nameList;
}
