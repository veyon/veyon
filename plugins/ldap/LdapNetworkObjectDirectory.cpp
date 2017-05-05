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

	const auto computerLabs = ldapDirectory.computerLabs();
	const NetworkObject rootObject( NetworkObject::Root );

	for( const auto& computerLab : qAsConst( computerLabs ) )
	{
		NetworkObject computerLabObject( NetworkObject::Group, computerLab );

		if( m_objects.contains( computerLabObject ) == false )
		{
			emit objectsAboutToBeInserted( rootObject, m_objects.count(), 1 );
			m_objects[computerLabObject] = QList<NetworkObject>();
			emit objectsInserted();
		}

		updateComputerLab( ldapDirectory, computerLab );
	}

	int index = 0;
	for( auto it = m_objects.begin(); it != m_objects.end(); )
	{
		if( computerLabs.contains( it.key().name() ) == false )
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



void LdapNetworkObjectDirectory::updateComputerLab( LdapDirectory& ldapDirectory, const QString &computerLab )
{
	const NetworkObject computerLabObject( NetworkObject::Group, computerLab );
	QList<NetworkObject>& computerLabObjects = m_objects[computerLabObject];

	QStringList computers = ldapDirectory.computerLabMembers( computerLab );

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

		if( computerLabObjects.contains( computerObject ) == false )
		{
			emit objectsAboutToBeInserted( computerLabObject, computerLabObjects.count(), 1 );
			computerLabObjects += computerObject;
			emit objectsInserted();
		}
	}

	int index = 0;
	for( auto it = computerLabObjects.begin(); it != computerLabObjects.end(); )
	{
		if( computers.contains( it->directoryAddress() ) == false )
		{
			emit objectsAboutToBeRemoved( computerLabObject, index, 1 );
			it = computerLabObjects.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++it;
			++index;
		}
	}
}
