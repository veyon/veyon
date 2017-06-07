/*
 * LdapNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory for LDAP
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "LdapNetworkObjectDirectory.h"
#include "LdapConfiguration.h"
#include "LdapDirectory.h"


LdapNetworkObjectDirectory::LdapNetworkObjectDirectory( const LdapConfiguration& configuration, QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_configuration( configuration )
{
	update();
}



QList<NetworkObject> LdapNetworkObjectDirectory::objects(const NetworkObject &parent)
{
	if( parent.type() == NetworkObject::Root )
	{
		return m_objects.keys();
	}
	else if( parent.type() == NetworkObject::Group &&
			 m_objects.contains( parent ) )
	{
		return m_objects[parent];
	}

	return QList<NetworkObject>();
}



void LdapNetworkObjectDirectory::update()
{
	LdapDirectory ldapDirectory( m_configuration );

	const auto computerRooms = ldapDirectory.computerRooms();
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

		updateComputerRoom( ldapDirectory, computerRoom );
	}

	int index = 0;
	for( auto it = m_objects.begin(); it != m_objects.end(); )
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



void LdapNetworkObjectDirectory::updateComputerRoom( LdapDirectory& ldapDirectory, const QString &computerRoom )
{
	const NetworkObject computerRoomObject( NetworkObject::Group, computerRoom );
	QList<NetworkObject>& computerRoomObjects = m_objects[computerRoomObject];

	QStringList computers = ldapDirectory.computerRoomMembers( computerRoom );

	bool hasMacAddressAttribute = ( m_configuration.ldapComputerMacAddressAttribute().count() > 0 );

	for( const auto& computer : qAsConst( computers ) )
	{
		QString computerHostName = ldapDirectory.computerHostName( computer );
		if( computerHostName.isEmpty() )
		{
			continue;
		}

		QString computerMacAddress;
		if( hasMacAddressAttribute )
		{
			computerMacAddress = ldapDirectory.computerMacAddress( computer );
		}

		const NetworkObject computerObject( NetworkObject::Host,
											computerHostName,
											computerHostName,
											computerMacAddress,
											computer );

		if( computerRoomObjects.contains( computerObject ) == false )
		{
			emit objectsAboutToBeInserted( computerRoomObject, computerRoomObjects.count(), 1 );
			computerRoomObjects += computerObject;
			emit objectsInserted();
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
