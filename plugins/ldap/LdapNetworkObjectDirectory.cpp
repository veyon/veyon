/*
 * LdapNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory for LDAP
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "LdapConfiguration.h"
#include "LdapDirectory.h"
#include "LdapNetworkObjectDirectory.h"


LdapNetworkObjectDirectory::LdapNetworkObjectDirectory( const LdapConfiguration& ldapConfiguration,
														QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_ldapDirectory( ldapConfiguration )
{
}



QList<NetworkObject> LdapNetworkObjectDirectory::objects( const NetworkObject& parent )
{
	if( parent.type() == NetworkObject::Root )
	{
		return m_objects.keys();
	}
	else if( parent.type() == NetworkObject::Group &&
			 m_objects.contains( parent ) )
	{
		return qAsConst(m_objects)[parent];
	}

	return QList<NetworkObject>();
}



QList<NetworkObject> LdapNetworkObjectDirectory::queryObjects( NetworkObject::Type type, const QString& name )
{
	switch( type )
	{
	case NetworkObject::Group: return queryGroups( name );
	case NetworkObject::Host: return queryHosts( name );
	default: break;
	}

	return {};
}



NetworkObject LdapNetworkObjectDirectory::queryParent( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Host:
		return NetworkObject( NetworkObject::Group,
							  m_ldapDirectory.computerRoomsOfComputer( object.directoryAddress() ).value( 0 ) );
	case NetworkObject::Group:
		return NetworkObject::Root;
	default:
		break;
	}

	return NetworkObject::None;
}



void LdapNetworkObjectDirectory::update()
{
	const auto computerRooms = m_ldapDirectory.computerRooms();
	const NetworkObject rootObject( NetworkObject::Root );

	for( const auto& computerRoom : qAsConst( computerRooms ) )
	{
		NetworkObject computerRoomObject( NetworkObject::Group, computerRoom );

		if( m_objects.contains( computerRoomObject ) == false )
		{
			emit objectsAboutToBeInserted( rootObject, m_objects.count(), 1 );
			m_objects[computerRoomObject] = QList<NetworkObject>();
			emit objectsInserted();
		}

		updateComputerRoom( computerRoom );
	}

	int index = 0;
	for( auto it = m_objects.begin(); it != m_objects.end(); )	// clazy:exclude=detaching-member
	{
		if( computerRooms.contains( it.key().name() ) == false )
		{
			emit objectsAboutToBeRemoved( rootObject, index, 1 );
			it = m_objects.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++it;
			++index;
		}
	}
}



void LdapNetworkObjectDirectory::updateComputerRoom( const QString& computerRoom )
{
	const NetworkObject computerRoomObject( NetworkObject::Group, computerRoom );
	QList<NetworkObject>& computerRoomObjects = m_objects[computerRoomObject]; // clazy:exclude=detaching-member

	const auto computers = m_ldapDirectory.computerRoomMembers( computerRoom );

	bool hasMacAddressAttribute = ( m_ldapDirectory.configuration().computerMacAddressAttribute().count() > 0 );

	for( const auto& computer : qAsConst( computers ) )
	{
		const auto computerObject = computerToObject( computer, hasMacAddressAttribute );
		if( computerObject.type() != NetworkObject::Host )
		{
			continue;
		}

		const auto index = computerRoomObjects.indexOf( computerObject );
		if( index < 0 )
		{
			emit objectsAboutToBeInserted( computerRoomObject, computerRoomObjects.count(), 1 );
			computerRoomObjects += computerObject; // clazy:exclude=reserve-candidates
			emit objectsInserted();
		}
		else if( computerRoomObjects[index].exactMatch( computerObject ) == false )
		{
			computerRoomObjects.replace( index, computerObject );
			emit objectChanged( computerRoomObject, index );
		}
	}

	int index = 0;
	for( auto it = computerRoomObjects.begin(); it != computerRoomObjects.end(); )
	{
		if( computers.contains( it->directoryAddress() ) == false )
		{
			emit objectsAboutToBeRemoved( computerRoomObject, index, 1 );
			it = computerRoomObjects.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++it;
			++index;
		}
	}
}



QList<NetworkObject> LdapNetworkObjectDirectory::queryGroups( const QString& name )
{
	const auto groups = m_ldapDirectory.computerRooms( name );

	QList<NetworkObject> groupObjects;
	groupObjects.reserve( groups.size() );

	for( const auto& group : groups )
	{
		groupObjects.append( NetworkObject( NetworkObject::Group, group ) );
	}

	return groupObjects;
}



QList<NetworkObject> LdapNetworkObjectDirectory::queryHosts( const QString& name )
{
	const auto computers = m_ldapDirectory.computers( name );

	QList<NetworkObject> hostObjects;
	hostObjects.reserve( computers.size() );

	for( const auto& computer : computers )
	{
		hostObjects.append( computerToObject( computer, false ) );
	}

	return hostObjects;
}



NetworkObject LdapNetworkObjectDirectory::computerToObject( const QString& computerDn, bool populateMacAddres )
{
	const auto computerHostName = m_ldapDirectory.computerHostName( computerDn );
	if( computerHostName.isEmpty() )
	{
		return NetworkObject::None;
	}

	QString computerMacAddress;
	if( populateMacAddres )
	{
		computerMacAddress = m_ldapDirectory.computerMacAddress( computerDn );
	}

	return NetworkObject( NetworkObject::Host,
						  computerHostName,
						  computerHostName,
						  computerMacAddress,
						  computerDn );
}
