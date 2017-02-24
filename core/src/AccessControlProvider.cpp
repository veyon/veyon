/*
 * AccessControlProvider.cpp - implementation of the AccessControlProvider class
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

#include <QDebug>
#include <QHostInfo>

#include "AccessControlProvider.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"


AccessControlProvider::AccessControlProvider() :
	m_ldapDirectory()
{
	for( auto accessControlRule : ItalcCore::config->accessControlRules() )
	{
		m_accessControlRules.append( accessControlRule );
	}
}



QStringList AccessControlProvider::userGroups()
{
	QStringList groups;

	if( m_ldapDirectory.isEnabled() )
	{
		if( m_ldapDirectory.isBound() )
		{
			groups = m_ldapDirectory.toRelativeDnList( m_ldapDirectory.userGroups() );
		}
	}
	else
	{
		groups = LocalSystem::userGroups();
	}

	std::sort( groups.begin(), groups.end() );

	return groups;
}



QStringList AccessControlProvider::computerGroups()
{
	QStringList groups;

	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		groups = m_ldapDirectory.toRelativeDnList( m_ldapDirectory.computerGroups() );
	}

	std::sort( groups.begin(), groups.end() );

	return groups;
}



QStringList AccessControlProvider::computerLabs()
{
	QStringList computerLabList;

	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		computerLabList = m_ldapDirectory.computerLabs();
	}

	std::sort( computerLabList.begin(), computerLabList.end() );

	return computerLabList;
}



AccessControlProvider::AccessResult AccessControlProvider::checkAccess( QString accessingUser, QString accessingComputer )
{
	// remove the domain part of the accessing user (e.g. "EXAMPLE.COM\Teacher" -> "TEACHER")
	int domainSeparator = accessingUser.indexOf( '\\' );
	if( domainSeparator >= 0 )
	{
		accessingUser = accessingUser.mid( domainSeparator + 1 );
	}

	if( ItalcCore::config->isAccessRestrictedToUserGroups() )
	{
		if( processAuthorizedGroups( accessingUser ) )
		{
			return AccessAllow;
		}
	}
	else if( ItalcCore::config->isAccessControlRulesProcessingEnabled() )
	{
		auto action = processAccessControlRules( accessingUser,
												 accessingComputer,
												 LocalSystem::User::loggedOnUser().name(),
												 QHostInfo::localHostName() );
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	return groupsOfUser( accessingUser ).toSet().intersects( ItalcCore::config->authorizedUserGroups().toSet() );
#else
	return groupsOfUser( accessingUser ).toSet().intersect( ItalcCore::config->authorizedUserGroups().toSet() ).isEmpty() == false;
#endif
}



AccessControlRule::Action AccessControlProvider::processAccessControlRules(const QString &accessingUser,
																		   const QString &accessingComputer,
																		   const QString &localUser,
																		   const QString &localComputer)
{
	qDebug() << "AccessControlProvider::processAccessControlRules(): processing rules for"
			 << accessingUser << accessingComputer << localUser << localComputer;

	for( auto rule : m_accessControlRules )
	{
		// rule disabled?
		if( rule.action() == AccessControlRule::ActionNone )
		{
			// then continue with next rule
			continue;
		}

		if( matchConditions( rule, accessingUser, accessingComputer, localUser, localComputer ) )
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
bool AccessControlProvider::isAccessDeniedByLocalState()
{
	if( ItalcCore::config->isAccessControlRulesProcessingEnabled() == false )
	{
		return false;
	}

	for( auto rule : m_accessControlRules )
	{
		if( rule.action() == AccessControlRule::ActionDeny &&
				matchConditions( rule, QString(), QString(),
								 LocalSystem::User::loggedOnUser().name(),
								 QHostInfo::localHostName() ) )
		{
			return true;
		}
	}

	return false;
}



QStringList AccessControlProvider::groupsOfUser( const QString &userName )
{
	if( m_ldapDirectory.isEnabled() )
	{
		if( m_ldapDirectory.isBound() )
		{
			const QString userDn = m_ldapDirectory.users( userName ).value( 0 );

			if( userDn.isEmpty() == false )
			{
				return m_ldapDirectory.toRelativeDnList( m_ldapDirectory.groupsOfUser( userDn ) );
			}
		}
	}
	else
	{
		return LocalSystem::groupsOfUser( userName );
	}

	return QStringList();
}



bool AccessControlProvider::isMemberOfGroup( AccessControlRule::EntityType entityType,
											 const QString &entity,
											 const QString &groupName )
{
	if( entityType != AccessControlRule::EntityTypeUser )
	{
		return false;
	}

	QRegExp groupNameRX( groupName );

	if( groupNameRX.isValid() )
	{
		return groupsOfUser( entity ).indexOf( QRegExp( groupName ) ) >= 0;
	}

	return groupsOfUser( entity ).contains( groupName );
}



bool AccessControlProvider::isLocatedInComputerLab( AccessControlRule::EntityType entityType,
													const QString &entity,
													const QString &computerLabName )
{
	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		QStringList computerLabMembers = m_ldapDirectory.computerLabMembers( computerLabName );

		if( computerLabMembers.isEmpty() )
		{
			qWarning() << "AccessControlProvider::isMemberOfComputerLab(): empty computer lab!";
			return false;
		}

		const QString objectDn = ldapObjectOfEntity( entityType, entity );

		return objectDn.isEmpty() == false && computerLabMembers.contains( objectDn );
	}

	return false;
}



bool AccessControlProvider::hasGroupsInCommon( AccessControlRule::EntityType entityOneType, const QString &entityOne,
											   AccessControlRule::EntityType entityTwoType, const QString &entityTwo )
{
	if( m_ldapDirectory.isEnabled() )
	{
		if( m_ldapDirectory.isBound() )
		{
			QStringList objectOneGroups = ldapGroupsOfEntity( entityOneType, entityOne );
			QStringList objectTwoGroups = ldapGroupsOfEntity( entityTwoType, entityTwo );

			return objectOneGroups.toSet().intersect( objectTwoGroups.toSet() ).isEmpty() == false;
		}
	}
	else if( entityOneType == AccessControlRule::EntityTypeUser &&
			 entityTwoType == AccessControlRule::EntityTypeUser )
	{
		return LocalSystem::groupsOfUser( entityOne ).toSet().
				intersect( LocalSystem::groupsOfUser( entityTwo ).toSet() ).isEmpty() == false;
	}

	return false;
}



bool AccessControlProvider::isLocatedInSameComputerLab( AccessControlRule::EntityType entityOneType, const QString &entityOne,
														AccessControlRule::EntityType entityTwoType, const QString &entityTwo )
{
	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		QStringList objectOneComputerLabs = ldapComputerLabsOfEntity( entityOneType, entityOne );
		QStringList objectTwoComputerLabs = ldapComputerLabsOfEntity( entityTwoType, entityTwo );

		return objectOneComputerLabs.toSet().intersect( objectTwoComputerLabs.toSet() ).isEmpty() == false;
	}

	return false;
}



bool AccessControlProvider::isLocalHost( const QString &accessingComputer ) const
{
	return QHostAddress( accessingComputer ).isLoopback();
}



bool AccessControlProvider::isLocalUser(const QString &accessingUser, const QString &localUser) const
{
	return accessingUser == localUser;
}



QString AccessControlProvider::lookupEntity( AccessControlRule::Entity entity,
											 const QString &accessingUser, const QString &accessingComputer,
											 const QString &localUser, const QString &localComputer )
{
	switch( entity )
	{
	case AccessControlRule::EntityAccessingUser: return accessingUser;
	case AccessControlRule::EntityAccessingComputer: return accessingComputer;
	case AccessControlRule::EntityLocalUser: return localUser;
	case AccessControlRule::EntityLocalComputer: return localComputer;
	default: break;
	}

	return QString();
}



QString AccessControlProvider::ldapObjectOfEntity( AccessControlRule::EntityType entityType, const QString &entity )
{
	QStringList objects;

	// do not query objects without a filter value
	if( entity.isEmpty() )
	{
		qWarning() << "AccessControlProvider::getLdapObjectForEntity(): empty entity of type" << entityType;
		return QString();
	}

	switch( entityType )
	{
	case AccessControlRule::EntityTypeUser:
		objects = m_ldapDirectory.users( entity );
		break;
	case AccessControlRule::EntityTypeComputer:
		return m_ldapDirectory.computerObjectFromHost( entity );
		break;
	default: break;
	}

	if( objects.isEmpty() )
	{
		return QString();
	}

	if( objects.count() > 1 )
	{
		qWarning() << "AccessControlProvider::getLdapObjectForEntity(): more than one entity object resolved from LDAP!";

		// return empty result in order to prevent undesired effects leading to possible security issues
		return QString();
	}

	return objects.first();
}



QStringList AccessControlProvider::ldapGroupsOfEntity( AccessControlRule::EntityType entityType, const QString &entity )
{
	const QString objectDn = ldapObjectOfEntity( entityType, entity );

	if( objectDn.isEmpty() )
	{
		return QStringList();
	}

	if( entityType == AccessControlRule::EntityTypeUser )
	{
		return m_ldapDirectory.groupsOfUser( objectDn );
	}
	else if( entityType == AccessControlRule::EntityTypeComputer )
	{
		// use LdapDirectory::groupsOfComputer() as it automatically uses the correct group
		// member attribute and resolves the required attribute of the computer object
		return m_ldapDirectory.groupsOfComputer( objectDn );
	}

	return QStringList();
}



QStringList AccessControlProvider::ldapComputerLabsOfEntity( AccessControlRule::EntityType entityType, const QString &entity )
{
	const QString objectDn = ldapObjectOfEntity( entityType, entity );

	if( objectDn.isEmpty() )
	{
		return QStringList();
	}

	if( entityType == AccessControlRule::EntityTypeUser )
	{
		return m_ldapDirectory.computerLabsOfUser( objectDn );
	}
	else if( entityType == AccessControlRule::EntityTypeComputer )
	{
		return m_ldapDirectory.computerLabsOfComputer( objectDn );
	}

	return QStringList();
}



bool AccessControlProvider::matchConditions( const AccessControlRule &rule,
											 const QString& accessingUser, const QString& accessingComputer,
											 const QString& localUser, const QString& localComputer)
{
	const AccessControlRule::Entity ruleEntity = rule.entity();
	const AccessControlRule::EntityType ruleEntityType = AccessControlRule::entityType( ruleEntity );

	const QString ruleEntityValue = lookupEntity( ruleEntity, accessingUser, accessingComputer, localUser, localComputer );

	// we can't match empty entity values therefore ignore this rule
	if( ruleEntityValue.isEmpty() )
	{
		return false;
	}

	bool hasConditions = false;

	// normally all selected conditions have to match in order to make the whole rule match
	// if conditions should be inverted (i.e. "is member of" is to be interpreted as "is NOT member of")
	// we have to check against the opposite boolean value
	bool matchResult = rule.areConditionsInverted();

	qDebug() << "AccessControlProvider::matchConditions():" << rule.name() << rule.entity() << ruleEntityValue << matchResult << rule.conditions();

	if( rule.hasCondition( AccessControlRule::ConditionMemberOfGroup ) )
	{
		hasConditions = true;

		if( isMemberOfGroup( ruleEntityType,
							 ruleEntityValue,
							 rule.conditionArgument( AccessControlRule::ConditionMemberOfGroup ).toString() ) == matchResult )
		{
			return false;
		}
	}


	if( rule.hasCondition( AccessControlRule::ConditionGroupsInCommon ) )
	{
		hasConditions = true;

		const AccessControlRule::Entity secondEntity =
				rule.conditionArgument( AccessControlRule::ConditionGroupsInCommon ).value<AccessControlRule::Entity>();

		const AccessControlRule::EntityType secondEntityType = AccessControlRule::entityType( secondEntity );
		const QString secondEntityValue = lookupEntity( secondEntity, accessingUser, accessingComputer, localUser, localComputer );

		if( hasGroupsInCommon( ruleEntityType, ruleEntityValue,
							   secondEntityType, secondEntityValue ) == matchResult )
		{
			return false;
		}
	}


	if( rule.hasCondition( AccessControlRule::ConditionLocatedInComputerLab ) )
	{
		hasConditions = true;

		if( isLocatedInComputerLab( ruleEntityType,
									ruleEntityValue,
									rule.conditionArgument( AccessControlRule::ConditionLocatedInComputerLab ).toString() ) == matchResult )
		{
			return false;
		}
	}


	if( rule.hasCondition( AccessControlRule::ConditionLocatedInSameComputerLab ) )
	{
		hasConditions = true;

		const AccessControlRule::Entity secondEntity =
				rule.conditionArgument( AccessControlRule::ConditionLocatedInSameComputerLab ).value<AccessControlRule::Entity>();

		const AccessControlRule::EntityType secondEntityType = AccessControlRule::entityType( secondEntity );
		const QString secondEntityValue = lookupEntity( secondEntity, accessingUser, accessingComputer, localUser, localComputer );

		if( isLocatedInSameComputerLab( ruleEntityType, ruleEntityValue,
										secondEntityType, secondEntityValue ) == matchResult )
		{
			return false;
		}
	}

	if( rule.hasCondition( AccessControlRule::ConditionAccessFromLocalHost ) )
	{
		hasConditions = true;

		if( isLocalHost( accessingComputer ) == matchResult )
		{
			return false;
		}
	}

	if( rule.hasCondition( AccessControlRule::ConditionAccessFromLocalUser ) )
	{
		hasConditions = true;

		if( isLocalUser( accessingUser, localUser ) == matchResult )
		{
			return false;
		}
	}

	if( rule.hasCondition( AccessControlRule::ConditionAccessFromAlreadyConnectedUser ) )
	{
		hasConditions = true;

		// TODO: implement connection list check logic
		return false;
	}

	// do not match the rule if no conditions are set at all
	if( hasConditions == false )
	{
		return false;
	}

	return true;
}
