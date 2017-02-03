/*
 * LdapNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory for LDAP
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QTimer>

#include "LdapNetworkObjectDirectory.h"
#include "Ldap/LdapDirectory.h"


LdapNetworkObjectDirectory::LdapNetworkObjectDirectory( QObject* parent ) :
	NetworkObjectDirectory( parent )
{
	/*QTimer*t = new QTimer( this );
	connect( t, &QTimer::timeout, this, &LdapNetworkObjectDirectory::update );
	t->start(2000);*/

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
	const NetworkObject rootObject( NetworkObject::Root );

	LdapDirectory ldapDirectory;

	QStringList computerLabs = ldapDirectory.computerLabs();

	for( auto computerLab : computerLabs )
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
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		if( computerLabs.contains( it.key().name() ) == false )
		{
			emit objectsAboutToBeRemoved( rootObject, index, 1 );
			it = m_objects.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++index;
		}
	}
}



void LdapNetworkObjectDirectory::updateComputerLab( LdapDirectory& ldapDirectory, const QString &computerLab )
{
	const NetworkObject computerLabObject( NetworkObject::Group, computerLab );
	QList<NetworkObject>& computerLabList = m_objects[computerLabObject];

	QStringList computers = ldapDirectory.computerLabMembers( computerLab );

	for( auto& computer : computers )
	{
		QString computerHostName = ldapDirectory.computerHostName( computer );
		if( computerHostName.isEmpty() )
		{
			continue;
		}
		computer = computer.split( ',' ).first();

		const NetworkObject computerObject( NetworkObject::Host, computer, computerHostName );

		if( computerLabList.contains( computerObject ) == false )
		{
			emit objectsAboutToBeInserted( computerLabObject, computerLabList.count(), 1 );
			computerLabList += computerObject;
			emit objectsInserted();
		}
	}

	int index = 0;
	for( auto it = computerLabList.begin(); it != computerLabList.end(); ++it )
	{
		if( computers.contains( it->name() ) == false )
		{
			emit objectsAboutToBeRemoved( computerLabObject, index, 1 );
			it = computerLabList.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++index;
		}
	}
}
