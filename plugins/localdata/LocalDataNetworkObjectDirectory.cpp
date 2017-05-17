/*
 * LocalDataNetworkObjectDirectory.cpp - NetworkObjects from VeyonConfiguration
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

#include "LocalDataConfiguration.h"
#include "LocalDataNetworkObjectDirectory.h"

LocalDataNetworkObjectDirectory::LocalDataNetworkObjectDirectory( const LocalDataConfiguration& configuration, QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_configuration( configuration )
{
	update();
}



QList<NetworkObject> LocalDataNetworkObjectDirectory::objects( const NetworkObject& parent )
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



void LocalDataNetworkObjectDirectory::update()
{
	const auto networkObjects = m_configuration.networkObjects();

	const NetworkObject rootObject( NetworkObject::Root );

	QVector<NetworkObject::Uid> roomUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.type() == NetworkObject::Group )
		{
			roomUids.append( networkObject.uid() );

			if( m_objects.contains( networkObject ) == false )
			{
				emit objectsAboutToBeInserted( rootObject, m_objects.count(), 1 );
				m_objects[networkObject] = QList<NetworkObject>();
				emit objectsInserted();
			}

			updateRoom( networkObject );
		}
	}

	int index = 0;
	for( auto it = m_objects.begin(); it != m_objects.end(); )
	{
		if( it.key().type() == NetworkObject::Group &&
				roomUids.contains( it.key().uid() ) == false )
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



void LocalDataNetworkObjectDirectory::updateRoom( const NetworkObject& roomObject )
{
	const auto networkObjects = m_configuration.networkObjects();

	QList<NetworkObject>& computerObjects = m_objects[roomObject];

	QVector<NetworkObject::Uid> computerUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.parentUid() == roomObject.uid() )
		{
			computerUids.append( networkObject.uid() );

			if( computerObjects.contains( networkObject ) == false )
			{
				emit objectsAboutToBeInserted( roomObject, computerObjects.count(), 1 );
				computerObjects += networkObject;
				emit objectsInserted();
			}
		}
	}

	int index = 0;
	for( auto it = computerObjects.begin(); it != computerObjects.end(); )
	{
		if( computerUids.contains( it->uid() ) == false )
		{
			emit objectsAboutToBeRemoved( roomObject, index, 1 );
			it = computerObjects.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++it;
			++index;
		}
	}
}
