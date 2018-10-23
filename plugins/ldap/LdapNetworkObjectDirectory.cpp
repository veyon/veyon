/*
 * LdapNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory for LDAP
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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



NetworkObjectList LdapNetworkObjectDirectory::queryObjects( NetworkObject::Type type, const QString& name )
{
	switch( type )
	{
	case NetworkObject::Group: return queryGroups( name );
	case NetworkObject::Host: return queryHosts( name );
	default: break;
	}

	return {};
}



NetworkObjectList LdapNetworkObjectDirectory::queryParents( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Host:
		return { NetworkObject( NetworkObject::Group,
								m_ldapDirectory.computerRoomsOfComputer( object.directoryAddress() ).value( 0 ) ) };
	case NetworkObject::Group:
		return { NetworkObject::Root };
	default:
		break;
	}

	return { NetworkObject::None };
}



void LdapNetworkObjectDirectory::update()
{
	const auto groups = m_ldapDirectory.computerRooms();
	const NetworkObject rootObject( NetworkObject::Root );

	for( const auto& computerRoom : qAsConst( groups ) )
	{
		const NetworkObject computerRoomObject( NetworkObject::Group, computerRoom );

		addOrUpdateObject( computerRoomObject, rootObject );

		updateGroup( computerRoomObject );
	}

	removeObjects( NetworkObject::Root, [groups]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Group && groups.contains( object.name() ) == false; } );
}



void LdapNetworkObjectDirectory::updateGroup( const NetworkObject& groupObject )
{
	const auto computers = m_ldapDirectory.computerRoomMembers( groupObject.name() );

	bool hasMacAddressAttribute = ( m_ldapDirectory.configuration().computerMacAddressAttribute().count() > 0 );

	for( const auto& computer : qAsConst( computers ) )
	{
		const auto hostObject = computerToObject( computer, hasMacAddressAttribute );
		if( hostObject.type() == NetworkObject::Host )
		{
			addOrUpdateObject( hostObject, groupObject );
		}
	}

	removeObjects( groupObject, [computers]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Host && computers.contains( object.directoryAddress() ) == false; } );
}



NetworkObjectList LdapNetworkObjectDirectory::queryGroups( const QString& name )
{
	const auto groups = m_ldapDirectory.computerRooms( name );

	NetworkObjectList groupObjects;
	groupObjects.reserve( groups.size() );

	for( const auto& group : groups )
	{
		groupObjects.append( NetworkObject( NetworkObject::Group, group ) );
	}

	return groupObjects;
}



NetworkObjectList LdapNetworkObjectDirectory::queryHosts( const QString& name )
{
	const auto computers = m_ldapDirectory.computers( name );

	NetworkObjectList hostObjects;
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
