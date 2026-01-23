/*
 * AccessControlProvider.cpp - implementation of the AccessControlProvider class
 *
 * Copyright (c) 2016-2026 Tobias Junghans <tobydox@veyon.io>
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
#include "PlatformUserFunctions.h"


AccessControlProvider::AccessControlProvider() :
	m_userGroupsBackend(VeyonCore::userGroupsBackendManager().configuredBackend()),
	m_networkObjectDirectory(VeyonCore::networkObjectDirectoryManager().configuredDirectory()),
	m_useDomainUserGroups(VeyonCore::config().useDomainUserGroups()),
	m_accessControlFeature(QLatin1String(staticMetaObject.className()),
						   Feature::Flag::Meta | Feature::Flag::Builtin,
						   Feature::Uid{QStringLiteral("1815941e-eaef-43ab-b9cd-5403dca3f749")}, {}, {}, {}, {}, {})
{
	const QJsonArray accessControlRules = VeyonCore::config().accessControlRules();

	m_accessControlRules.reserve( accessControlRules.size() );

	for (const auto& accessControlRule : accessControlRules)
	{
		m_accessControlRules.append(AccessControlRule::Pointer::create(accessControlRule));
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
																			 NetworkObject::Attribute::None, {} ) );

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
																   NetworkObject::Attribute::HostAddress, fqdn );
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



AccessControlProvider::CheckResult AccessControlProvider::checkAccess(const QString& accessingUser,
																	  const QString& accessingComputer,
																	  const QStringList& connectedUsers)
{
	CheckResult denyAccessCheckResult{Access::Deny};
	if (VeyonCore::config().isAccessRestrictedToUserGroups())
	{
		if (processAuthorizedGroups(accessingUser))
		{
			return {Access::Allow, Reason::UserInAuthorizedUserGroups};
		}
		denyAccessCheckResult.reason = Reason::UserNotInAuthorizedUserGroups;
	}
	else if (VeyonCore::config().isAccessControlRulesProcessingEnabled())
	{
		const auto rule = processAccessControlRules(accessingUser,
													accessingComputer,
													VeyonCore::platform().userFunctions().currentUser(),
													HostAddress::localFQDN(),
													connectedUsers);
		if (rule)
		{
			switch(rule->action())
			{
			case AccessControlRule::Action::Allow:
				return {Access::Allow, Reason::AccessControlRuleMatched};
			case AccessControlRule::Action::AskForPermission:
				return {Access::ToBeConfirmed};
			default: break;
			}
			denyAccessCheckResult.reason = Reason::AccessControlRuleMatched;
			denyAccessCheckResult.matchedRule = rule;
		}
		else
		{
			denyAccessCheckResult.reason = Reason::NoAccessControlRuleMatched;
		}
	}
	else
	{
		vDebug() << "no access control method configured, allowing access.";

		// no access control method configured, therefore grant access
		return {Access::Allow};
	}

	vDebug() << "configured access control method did not succeed, denying access.";

	// configured access control method did not succeed, therefore deny access
	return denyAccessCheckResult;
}



bool AccessControlProvider::processAuthorizedGroups( const QString& accessingUser )
{
	vDebug() << "processing for user" << accessingUser;

	const auto groupsOfAccessingUser = m_userGroupsBackend->groupsOfUser( accessingUser, m_useDomainUserGroups );
	const auto authorizedUserGroups = VeyonCore::config().authorizedUserGroups();

	vDebug() << groupsOfAccessingUser << authorizedUserGroups;

	const auto groupsOfAccessingUserSet = QSet<QString>{ groupsOfAccessingUser.begin(), groupsOfAccessingUser.end() };
	const auto authorizedUserGroupSet = QSet<QString>{ authorizedUserGroups.begin(), authorizedUserGroups.end() };

	return groupsOfAccessingUserSet.intersects( authorizedUserGroupSet );
}



AccessControlRule::Pointer AccessControlProvider::processAccessControlRules(const QString& accessingUser,
																			const QString& accessingComputer,
																			const QString& localUser,
																			const QString& localComputer,
																			const QStringList& connectedUsers)
{
	vDebug() << "processing rules for" << accessingUser << accessingComputer << localUser << localComputer << connectedUsers;

	for (const auto& rule : std::as_const(m_accessControlRules))
	{
		// rule disabled?
		if (rule->action() == AccessControlRule::Action::None)
		{
			// then continue with next rule
			continue;
		}

		if (rule->areConditionsIgnored() ||
			matchConditions(*rule, accessingUser, accessingComputer, localUser, localComputer, connectedUsers))
		{
			vDebug() << "rule" << rule->name() << "matched with action" << rule->action();
			return rule;
		}
	}

	vDebug() << "no matching rule, denying access";

	return nullptr;
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

	for (const auto& rule : std::as_const(m_accessControlRules))
	{
		if (matchConditions(*rule, {}, {},
							VeyonCore::platform().userFunctions().currentUser(), HostAddress::localFQDN(), {}))
		{
			switch (rule->action())
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



bool AccessControlProvider::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface, const FeatureMessage& message)
{
	if (message.featureUid() == m_accessControlFeature.uid())
	{
		computerControlInterface->setAccessControlFailed(message.argument(Argument::Details).toString());
		return true;
	}

	return false;
}



void AccessControlProvider::sendDetails(QIODevice* ioDevice, const QString& details)
{
	FeatureMessage{m_accessControlFeature.uid()}.addArgument(
				AccessControlProvider::Argument::Details, details)
			.sendAsRfbMessage(ioDevice);
}



bool AccessControlProvider::isMemberOfUserGroup( const QString &user,
												 const QString &groupName ) const
{
	return matchList(m_userGroupsBackend->groupsOfUser(user, m_useDomainUserGroups), groupName);
}



bool AccessControlProvider::isLocatedAt( const QString &computer, const QString &locationName ) const
{
	return matchList(locationsOfComputer(computer), locationName);
}



bool AccessControlProvider::haveGroupsInCommon( const QString &userOne, const QString &userTwo ) const
{
	const auto userOneGroups = m_userGroupsBackend->groupsOfUser( userOne, m_useDomainUserGroups );
	const auto userTwoGroups = m_userGroupsBackend->groupsOfUser( userTwo, m_useDomainUserGroups );

	const auto userOneGroupSet = QSet<QString>{ userOneGroups.begin(), userOneGroups.end() };
	const auto userTwoGroupSet = QSet<QString>{ userTwoGroups.begin(), userTwoGroups.end() };

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



bool AccessControlProvider::isNoUserLoggedOn() const
{
	return VeyonCore::platform().userFunctions().isAnyUserLoggedOn() == false;
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
											 const QStringList& connectedUsers ) const
{
	bool hasConditions = false;

	// normally all selected conditions have to match in order to make the whole rule match
	// if conditions should be inverted (i.e. "is member of" is to be interpreted as "is NOT member of")
	// we have to check against the opposite boolean value
	bool matchResult = rule.areConditionsInverted() == false;

	vDebug() << rule.toJson() << matchResult;

	if( rule.isConditionEnabled( AccessControlRule::Condition::MemberOfUserGroup ) )
	{
		hasConditions = true;

		const auto condition = AccessControlRule::Condition::MemberOfUserGroup;
		const auto user = lookupSubject( rule.subject( condition ), accessingUser, {}, localUser, {} );
		const auto group = rule.argument( condition );

		if( user.isEmpty() || group.isEmpty() ||
			isMemberOfUserGroup( user, group ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::GroupsInCommon ) )
	{
		hasConditions = true;

		if( accessingUser.isEmpty() || localUser.isEmpty() ||
			haveGroupsInCommon( accessingUser, localUser ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::LocatedAt ) )
	{
		hasConditions = true;

		const auto condition = AccessControlRule::Condition::LocatedAt;
		const auto computer = lookupSubject( rule.subject( condition ), {}, accessingComputer, {}, localComputer );
		const auto location = rule.argument( condition );

		if( computer.isEmpty() || location.isEmpty() ||
			isLocatedAt( computer, location ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::SameLocation ) )
	{
		hasConditions = true;

		if( accessingComputer.isEmpty() || localComputer.isEmpty() ||
			haveSameLocations( accessingComputer, localComputer ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromLocalHost ) )
	{
		hasConditions = true;

		if( isLocalHost( accessingComputer ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromLocalUser ) )
	{
		hasConditions = true;

		if( isLocalUser( accessingUser, localUser ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::AccessFromAlreadyConnectedUser ) )
	{
		hasConditions = true;

		if( connectedUsers.contains( accessingUser ) != matchResult )
		{
			return false;
		}
	}

	if( rule.isConditionEnabled( AccessControlRule::Condition::NoUserLoggedOn ) )
	{
		hasConditions = true;

		if( isNoUserLoggedOn() != matchResult )
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::ComputerAlreadyBeingAccessed))
	{
		hasConditions = true;

		if (!connectedUsers.isEmpty() != matchResult)
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



bool AccessControlProvider::matchList(const QStringList& list, const QString& pattern)
{
	if (pattern.startsWith(QLatin1Char('/')) && pattern.endsWith(QLatin1Char('/')) &&
		pattern.length() > 2)
	{
		return list.indexOf(QRegularExpression(pattern.mid(1, pattern.length() - 2))) >= 0;
	}

	if (pattern.endsWith(QLatin1Char('*')))
	{
		const QRegularExpression rx(pattern);
		if (rx.isValid())
		{
			return list.indexOf(rx) >= 0;
		}
	}

	return list.contains(pattern);
}
