/*
 * BuiltinDirectory.cpp - NetworkObjects from VeyonConfiguration
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

#include "BuiltinDirectoryConfiguration.h"
#include "BuiltinDirectory.h"


BuiltinDirectory::BuiltinDirectory( BuiltinDirectoryConfiguration& configuration, QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_configuration( configuration )
{
}



QList<NetworkObject> BuiltinDirectory::objects( const NetworkObject& parent )
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



QList<NetworkObject> BuiltinDirectory::queryObjects( NetworkObject::Type type, const QString& name )
{
	const auto networkObjects = m_configuration.networkObjects();

	QList<NetworkObject> objects;

	// search for corresponding group whose UID matches parent UID of computer object
	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( ( type == NetworkObject::None || networkObject.type() == type ) &&
				( name.isEmpty() || networkObject.name().compare( name, Qt::CaseInsensitive ) == 0 ) )
		{
			objects.append( networkObject );
		}
	}

	return objects;
}



NetworkObject BuiltinDirectory::queryParent( const NetworkObject& object )
{
	const auto networkObjects = m_configuration.networkObjects();
	const auto parentUid = object.parentUid();

	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.uid() == parentUid )
		{
			return networkObject;
		}
	}

	return NetworkObject();
}



void BuiltinDirectory::update()
{
	m_configuration.reloadFromStore();

	const auto networkObjects = m_configuration.networkObjects();

	const NetworkObject rootObject( NetworkObject::Root );

	QVector<NetworkObject::Uid> roomUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.type() == NetworkObject::Group )
		{
			roomUids.append( networkObject.uid() ); // clazy:exclude=reserve-candidates

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
	for( auto it = m_objects.begin(); it != m_objects.end(); ) // clazy:exclude=detaching-member
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



void BuiltinDirectory::updateRoom( const NetworkObject& roomObject )
{
	const auto networkObjects = m_configuration.networkObjects();

	QList<NetworkObject>& computerObjects = m_objects[roomObject]; // clazy:exclude=detaching-member

	QVector<NetworkObject::Uid> computerUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.parentUid() == roomObject.uid() )
		{
			computerUids.append( networkObject.uid() ); // clazy:exclude=reserve-candidates

			int index = computerObjects.indexOf( networkObject );
			if( index < 0 )
			{
				emit objectsAboutToBeInserted( roomObject, computerObjects.count(), 1 );
				computerObjects += networkObject; // clazy:exclude=reserve-candidates
				emit objectsInserted();
			}
			else if( computerObjects[index].exactMatch( networkObject ) == false )
			{
				computerObjects.replace( index, networkObject );
				emit objectChanged( roomObject, index );
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
