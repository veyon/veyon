/*
 * AccessControlProvider.cpp - implementation of the AccessControlProvider class
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

#include <QDebug>
#include <QHostInfo>

#include "AccessControlProvider.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"


AccessControlProvider::AccessControlProvider() :
	m_ldapDirectory()
{
	for( auto encodedRule : ItalcCore::config->accessControlRules() )
	{
		m_accessControlRules.append( AccessControlRule( encodedRule ) );
	}
}



QStringList AccessControlProvider::userGroups()
{
	QStringList groups;

	if( m_ldapDirectory.isEnabled() )
	{
		if( m_ldapDirectory.isBound() )
		{
			groups = m_ldapDirectory.userGroups();
		}
	}
	else
	{
		groups = LocalSystem::userGroups();
	}

	qSort( groups );

	return groups;
}



QStringList AccessControlProvider::computerGroups()
{
	QStringList groups;

	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		groups = m_ldapDirectory.computerGroups();
	}

	qSort( groups );

	return groups;
}



QStringList AccessControlProvider::computerPools()
{
	QStringList compoterPoolList;

	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		compoterPoolList = m_ldapDirectory.computerPools();
	}

	qSort( compoterPoolList );

	return compoterPoolList;
}



AccessControlProvider::AccessResult AccessControlProvider::checkAccess( const QString& accessingUser, const QString& accessingComputer )
{
	if( ItalcCore::config->isAccessRestrictedToUserGroups() )
	{
		if( processAuthorizedGroups( accessingUser ) )
		{
			return AccessAllow;
		}
	}
	else if( ItalcCore::config->isAccessControlRulesProcessingEnabled() )
	{
		if( processAccessControlRules( accessingUser,
									   accessingComputer,
									   LocalSystem::User::loggedOnUser().name(),
									   QHostInfo::localHostName() ) == AccessControlRule::ActionAllow )
		{
			return AccessAllow;
		}
	}
	else
	{
		// no access control method configured, therefore grant access
		return AccessAllow;
	}

	// configured access control method did not succeed, therefore deny access
	return AccessDeny;
}



bool AccessControlProvider::processAuthorizedGroups(const QString &accessingUser)
{
	qDebug() << "AccessControlProvider::processAuthorizedGroups(): processing for user" << accessingUser;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	return groupsOfUser( accessingUser ).toSet().intersects( ItalcCore::config->logonGroups().toSet() );
#else
	return groupsOfUser( accessingUser ).toSet().intersect( ItalcCore::config->logonGroups().toSet() ).isEmpty() == false;
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
				return m_ldapDirectory.groupsOfUser( userDn );
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
	return entityType == AccessControlRule::EntityTypeUser &&
			groupsOfUser( entity ).contains( groupName );
}



bool AccessControlProvider::isMemberOfComputerPool( AccessControlRule::EntityType entityType,
													const QString &entity,
													const QString &computerPoolName )
{
	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		QStringList computerPoolMembers = m_ldapDirectory.computerPoolMembers( computerPoolName );

		if( computerPoolMembers.isEmpty() )
		{
			qWarning() << "AccessControlProvider::isMemberOfComputerPool(): empty computer pool!";
			return false;
		}

		const QString objectDn = ldapObjectOfEntity( entityType, entity );

		return objectDn.isEmpty() == false && computerPoolMembers.contains( objectDn );
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



bool AccessControlProvider::hasComputerPoolsInCommon( AccessControlRule::EntityType entityOneType, const QString &entityOne,
													  AccessControlRule::EntityType entityTwoType, const QString &entityTwo )
{
	if( m_ldapDirectory.isEnabled() && m_ldapDirectory.isBound() )
	{
		QStringList objectOneComputerPools = ldapComputerPoolsOfEntity( entityOneType, entityOne );
		QStringList objectTwoComputerPools = ldapComputerPoolsOfEntity( entityTwoType, entityTwo );

		return objectOneComputerPools.toSet().intersect( objectTwoComputerPools.toSet() ).isEmpty() == false;
	}

	return false;
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

	switch( entityType )
	{
	case AccessControlRule::EntityTypeUser:
		objects = m_ldapDirectory.users( entity );
		break;
	case AccessControlRule::EntityTypeComputer:
		objects = m_ldapDirectory.computers( m_ldapDirectory.hostToLdapFormat( entity ) );
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



QStringList AccessControlProvider::ldapComputerPoolsOfEntity( AccessControlRule::EntityType entityType, const QString &entity )
{
	const QString objectDn = ldapObjectOfEntity( entityType, entity );

	if( objectDn.isEmpty() )
	{
		return QStringList();
	}

	if( entityType == AccessControlRule::EntityTypeUser )
	{
		return m_ldapDirectory.computerPoolsOfUser( objectDn );
	}
	else if( entityType == AccessControlRule::EntityTypeComputer )
	{
		return m_ldapDirectory.computerPoolsOfComputer( objectDn );
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

	qDebug() << "AccessControlProvider::matchConditions():" << rule.name() << rule.entity() << ruleEntityValue << rule.conditions();

	if( rule.hasCondition( AccessControlRule::ConditionMemberOfGroup ) &&
			isMemberOfGroup( ruleEntityType,
							 ruleEntityValue,
							 rule.conditionArgument( AccessControlRule::ConditionMemberOfGroup ).toString() ) == false )
	{
		return false;
	}

	if( rule.hasCondition( AccessControlRule::ConditionMemberOfComputerPool ) &&
			isMemberOfComputerPool( ruleEntityType,
									ruleEntityValue,
									rule.conditionArgument( AccessControlRule::ConditionMemberOfComputerPool ).toString() ) == false )
	{
		return false;
	}

	if( rule.hasCondition( AccessControlRule::ConditionGroupsInCommon ) )
	{
		const AccessControlRule::Entity secondEntity =
				rule.conditionArgument( AccessControlRule::ConditionGroupsInCommon ).value<AccessControlRule::Entity>();

		const AccessControlRule::EntityType secondEntityType = AccessControlRule::entityType( secondEntity );
		const QString secondEntityValue = lookupEntity( secondEntity, accessingUser, accessingComputer, localUser, localComputer );

		if( hasGroupsInCommon( ruleEntityType, ruleEntityValue,
							   secondEntityType, secondEntityValue ) == false )
		{
			return false;
		}
	}

	if( rule.hasCondition( AccessControlRule::ConditionComputerPoolsInCommon ) )
	{
		const AccessControlRule::Entity secondEntity =
				rule.conditionArgument( AccessControlRule::ConditionComputerPoolsInCommon ).value<AccessControlRule::Entity>();

		const AccessControlRule::EntityType secondEntityType = AccessControlRule::entityType( secondEntity );
		const QString secondEntityValue = lookupEntity( secondEntity, accessingUser, accessingComputer, localUser, localComputer );

		if( hasComputerPoolsInCommon( ruleEntityType, ruleEntityValue,
							   secondEntityType, secondEntityValue ) == false )
		{
			return false;
		}
	}

	return true;
}
