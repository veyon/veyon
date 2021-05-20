/*
 * LdapDirectory.cpp - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "HostAddress.h"
#include "LdapConfiguration.h"
#include "LdapDirectory.h"


LdapDirectory::LdapDirectory( const LdapConfiguration& configuration, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration ),
	m_client( configuration, QUrl(), this )
{
	if( m_configuration.recursiveSearchOperations() )
	{
		m_defaultSearchScope = LdapClient::Scope::Sub;
	}
	else
	{
		m_defaultSearchScope = LdapClient::Scope::One;
	}

	m_userLoginNameAttribute = m_configuration.userLoginNameAttribute();
	m_groupMemberAttribute = m_configuration.groupMemberAttribute();

	if( m_configuration.queryNestedUserGroups() &&
		m_groupMemberAttribute.contains( QLatin1Char(':') ) == false )
	{
		m_groupMemberAttribute.append( QLatin1String(":1.2.840.113556.1.4.1941:") );
	}

	m_computerDisplayNameAttribute = m_configuration.computerDisplayNameAttribute();
	m_computerHostNameAttribute = m_configuration.computerHostNameAttribute();
	m_computerHostNameAsFQDN = m_configuration.computerHostNameAsFQDN();
	m_computerMacAddressAttribute = m_configuration.computerMacAddressAttribute();
	m_locationNameAttribute = m_configuration.locationNameAttribute();

	if( m_computerDisplayNameAttribute.isEmpty() )
	{
		m_computerDisplayNameAttribute = QStringLiteral("cn");
	}

	if( m_locationNameAttribute.isEmpty() )
	{
		m_locationNameAttribute = QStringLiteral("cn");
	}

	m_usersFilter = m_configuration.usersFilter();
	m_userGroupsFilter = m_configuration.userGroupsFilter();
	m_computersFilter = m_configuration.computersFilter();
	m_computerGroupsFilter = m_configuration.computerGroupsFilter();
	m_computerContainersFilter = m_configuration.computerContainersFilter();

	m_identifyGroupMembersByNameAttribute = m_configuration.identifyGroupMembersByNameAttribute();

	m_computerLocationsByContainer = m_configuration.computerLocationsByContainer();
	m_computerLocationsByAttribute = m_configuration.computerLocationsByAttribute();
	m_computerLocationAttribute = m_configuration.computerLocationAttribute();
}



const QString& LdapDirectory::configInstanceId() const
{
	return m_configuration.instanceId();
}



QString LdapDirectory::usersDn()
{
	if( m_usersDn.isEmpty() )
	{
		m_usersDn = LdapClient::constructSubDn( m_configuration.userTree(), m_client.baseDn() );
	}

	return m_usersDn;
}



QString LdapDirectory::groupsDn()
{
	if( m_groupsDn.isEmpty() )
	{
		m_groupsDn = LdapClient::constructSubDn( m_configuration.groupTree(), m_client.baseDn() );
	}

	return m_groupsDn;
}



QString LdapDirectory::computersDn()
{
	if( m_computersDn.isEmpty() )
	{
		m_computersDn = LdapClient::constructSubDn( m_configuration.computerTree(), m_client.baseDn() );
	}

	return m_computersDn;
}



QString LdapDirectory::computerGroupsDn()
{
	if( m_computerGroupsDn.isEmpty() )
	{
		const auto computerGroupTree = m_configuration.computerGroupTree();

		if( computerGroupTree.isEmpty() == false )
		{
			m_computerGroupsDn = LdapClient::constructSubDn( computerGroupTree, m_client.baseDn() );
		}
		else
		{
			m_computerGroupsDn = groupsDn();
		}
	}

	return m_computerGroupsDn;
}



/*!
 * \brief Disables any configured attributes which is required for some test scenarious
 */
void LdapDirectory::disableAttributes()
{
	m_userLoginNameAttribute.clear();
	m_computerDisplayNameAttribute.clear();
	m_computerHostNameAttribute.clear();
	m_computerMacAddressAttribute.clear();
}



/*!
 * \brief Disables any configured filters which is required for some test scenarious
 */
void LdapDirectory::disableFilters()
{
	m_usersFilter.clear();
	m_userGroupsFilter.clear();
	m_computersFilter.clear();
	m_computerGroupsFilter.clear();
	m_computerContainersFilter.clear();
}




QStringList LdapDirectory::users( const QString& filterValue )
{
	return m_client.queryDistinguishedNames( usersDn(),
											 LdapClient::constructQueryFilter( m_userLoginNameAttribute, filterValue, m_usersFilter ),
											 m_defaultSearchScope );
}



QStringList LdapDirectory::groups( const QString& filterValue )
{
	return m_client.queryDistinguishedNames( groupsDn(),
											 LdapClient::constructQueryFilter( QStringLiteral( "cn" ), filterValue ),
											 m_defaultSearchScope );
}



QStringList LdapDirectory::userGroups( const QString& filterValue )
{
	return m_client.queryDistinguishedNames( groupsDn(),
											 LdapClient::constructQueryFilter( QStringLiteral( "cn" ), filterValue, m_userGroupsFilter ),
											 m_defaultSearchScope );
}



QStringList LdapDirectory::computersByDisplayName( const QString& filterValue )
{
	return m_client.queryDistinguishedNames( computersDn(),
											 LdapClient::constructQueryFilter( m_computerDisplayNameAttribute, filterValue, m_computersFilter ),
											 computerSearchScope() );
}



/*!
 * \brief Returns list of computer object names matching the given hostname filter
 * \param filterValue A filter value which is used to query the hostname attribute
 * \return List of DNs of all matching computer objects
 */
QStringList LdapDirectory::computersByHostName( const QString& filterValue )
{
	return m_client.queryDistinguishedNames( computersDn(),
											 LdapClient::constructQueryFilter( m_computerHostNameAttribute, filterValue, m_computersFilter ),
											 computerSearchScope() );
}



QStringList LdapDirectory::computerGroups( const QString& filterValue )
{
	return m_client.queryDistinguishedNames( computerGroupsDn(),
											 LdapClient::constructQueryFilter( m_locationNameAttribute, filterValue, m_computerGroupsFilter ) ,
											 m_defaultSearchScope );
}



QStringList LdapDirectory::computerLocations( const QString& filterValue )
{
	QStringList locations;

	if( m_computerLocationsByAttribute )
	{
		locations = m_client.queryAttributeValues( computersDn(),
												   m_computerLocationAttribute,
												   LdapClient::constructQueryFilter( m_computerLocationAttribute, filterValue, m_computersFilter ),
												   m_defaultSearchScope );
	}
	else if( m_computerLocationsByContainer )
	{
		locations = m_client.queryAttributeValues( computersDn(),
												   m_locationNameAttribute,
												   LdapClient::constructQueryFilter( m_locationNameAttribute, filterValue, m_computerContainersFilter ) ,
												   m_defaultSearchScope );
	}
	else
	{
		locations = m_client.queryAttributeValues( computerGroupsDn(),
												   m_locationNameAttribute,
												   LdapClient::constructQueryFilter( m_locationNameAttribute, filterValue, m_computerGroupsFilter ) ,
												   m_defaultSearchScope );
	}

	locations.removeDuplicates();

	std::sort( locations.begin(), locations.end() );

	return locations;
}



QStringList LdapDirectory::groupMembers( const QString& groupDn )
{
	return m_client.queryAttributeValues( groupDn, m_groupMemberAttribute );
}



QStringList LdapDirectory::groupsOfUser( const QString& userDn )
{
	const auto userId = groupMemberUserIdentification( userDn );
	if( m_groupMemberAttribute.isEmpty() || userId.isEmpty() )
	{
		return {};
	}

	return m_client.queryDistinguishedNames( groupsDn(),
											 LdapClient::constructQueryFilter( m_groupMemberAttribute, userId, m_userGroupsFilter ),
											 m_defaultSearchScope );
}



QStringList LdapDirectory::groupsOfComputer( const QString& computerDn )
{
	const auto computerId = groupMemberComputerIdentification( computerDn );
	if( m_groupMemberAttribute.isEmpty() || computerId.isEmpty() )
	{
		return {};
	}

	return m_client.queryDistinguishedNames( computerGroupsDn(),
											 LdapClient::constructQueryFilter( m_groupMemberAttribute, computerId, m_computerGroupsFilter ),
											 m_defaultSearchScope );
}



QStringList LdapDirectory::locationsOfComputer( const QString& computerDn )
{
	if( m_computerLocationsByAttribute )
	{
		return m_client.queryAttributeValues( computerDn, m_computerLocationAttribute );
	}
	else if( m_computerLocationsByContainer )
	{
		return m_client.queryAttributeValues( LdapClient::parentDn( computerDn ), m_locationNameAttribute );
	}

	const auto computerId = groupMemberComputerIdentification( computerDn );
	if( m_groupMemberAttribute.isEmpty() || computerId.isEmpty() )
	{
		return {};
	}

	return m_client.queryAttributeValues( computerGroupsDn(),
										  m_locationNameAttribute,
										  LdapClient::constructQueryFilter( m_groupMemberAttribute, computerId, m_computerGroupsFilter ),
										  m_defaultSearchScope );
}



QString LdapDirectory::userLoginName( const QString& userDn )
{
	return m_client.queryAttributeValues( userDn, m_userLoginNameAttribute ).value( 0 );
}



QString LdapDirectory::computerDisplayName( const QString& computerDn )
{
	return m_client.queryAttributeValues( computerDn, m_computerDisplayNameAttribute ).value( 0 );

}



QString LdapDirectory::computerHostName( const QString& computerDn )
{
	if( computerDn.isEmpty() )
	{
		return {};
	}

	return m_client.queryAttributeValues( computerDn, m_computerHostNameAttribute ).value( 0 );
}



QString LdapDirectory::computerMacAddress( const QString& computerDn )
{
	if( computerDn.isEmpty() )
	{
		return {};
	}

	return m_client.queryAttributeValues( computerDn, m_computerMacAddressAttribute ).value( 0 );
}



QString LdapDirectory::groupMemberUserIdentification( const QString& userDn )
{
	if( m_identifyGroupMembersByNameAttribute )
	{
		return userLoginName( userDn );
	}

	return userDn;
}



QString LdapDirectory::groupMemberComputerIdentification( const QString& computerDn )
{
	if( m_identifyGroupMembersByNameAttribute )
	{
		return computerHostName( computerDn );
	}

	return computerDn;
}



QStringList LdapDirectory::computerLocationEntries( const QString& locationName )
{
	if( m_computerLocationsByAttribute )
	{
		return m_client.queryDistinguishedNames( computersDn(),
												 LdapClient::constructQueryFilter( m_computerLocationAttribute, locationName, m_computersFilter ),
												 m_defaultSearchScope );
	}
	else if( m_computerLocationsByContainer )
	{
		const auto locationDnFilter = LdapClient::constructQueryFilter( m_locationNameAttribute, locationName, m_computerContainersFilter );
		const auto locationDns = m_client.queryDistinguishedNames( computersDn(), locationDnFilter, m_defaultSearchScope );

		return m_client.queryDistinguishedNames( locationDns.value( 0 ),
												 LdapClient::constructQueryFilter( {}, {}, m_computersFilter ),
												 m_defaultSearchScope );
	}

	const auto groups = computerGroups( locationName );
	if( groups.size() != 1 )
	{
		vWarning() << "location" << locationName << "does not resolve to exactly one computer group:" << groups;
	}

	if( groups.isEmpty() )
	{
		return {};
	}

	auto memberComputers = groupMembers( groups.value( 0 ) );

	// computer filter configured?
	if( m_computersFilter.isEmpty() == false )
	{
		const auto computerHostNames = computersByHostName();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		auto memberComputersSet = QSet<QString>( memberComputers.begin(), memberComputers.end() );
		const auto computerHostNameSet = QSet<QString>( computerHostNames.begin(), computerHostNames.end() );
#else
		auto memberComputersSet = memberComputers.toSet();
		const auto computerHostNameSet = computersByHostName().toSet();
#endif

		// then return intersection of filtered computer list and group members
		const auto computerIntersection = memberComputersSet.intersect( computerHostNameSet );
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		return { computerIntersection.begin(), computerIntersection.end() };
#else
		return computerIntersection.toList();
#endif
	}

	return memberComputers;
}



QString LdapDirectory::hostToLdapFormat( const QString& host )
{
	if( m_computerHostNameAsFQDN )
	{
		return HostAddress( host ).convert( HostAddress::Type::FullyQualifiedDomainName );
	}

	return HostAddress( host ).convert( HostAddress::Type::HostName );
}



QString LdapDirectory::computerObjectFromHost( const QString& host )
{
	const auto hostName = hostToLdapFormat( host );
	if( hostName.isEmpty() )
	{
		vWarning() << "could not resolve hostname, returning empty computer object";
		return {};
	}

	const auto computerObjects = computersByHostName( hostName );
	if( computerObjects.count() == 1 )
	{
		return computerObjects.first();
	}

	// return empty result if not exactly one object was found
	vWarning() << "more than one computer object found, returning empty computer object!";
	return {};
}



LdapClient::Scope LdapDirectory::computerSearchScope() const
{
	// when using containers/OUs as locations computer objects are not located directly below the configured computer DN
	if( m_computerLocationsByContainer )
	{
		return LdapClient::Scope::Sub;
	}

	return m_defaultSearchScope;
}
