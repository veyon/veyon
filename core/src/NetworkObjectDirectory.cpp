/*
 * NetworkObjectDirectory.cpp - base class for network object directory implementations
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QTimer>

#include "NetworkObjectDirectory.h"


NetworkObjectDirectory::NetworkObjectDirectory( const QString& name, QObject* parent ) :
	QObject( parent ),
	m_name( name ),
	m_updateTimer(new QTimer(this)),
	m_propagateChangedObjectsTimer(new QTimer(this))
{
	connect( m_updateTimer, &QTimer::timeout, this, &NetworkObjectDirectory::update );

	connect(m_propagateChangedObjectsTimer, &QTimer::timeout,
			this, &NetworkObjectDirectory::propagateChildObjectChanges);
	m_propagateChangedObjectsTimer->setInterval(ObjectChangePropagationTimeout);
	m_propagateChangedObjectsTimer->setSingleShot(true);

	// insert root item
	m_objects[rootId()] = {};
}



void NetworkObjectDirectory::setUpdateInterval( int interval )
{
	if( interval >= MinimumUpdateInterval )
	{
		m_updateTimer->start( interval*1000 );
	}
	else
	{
		m_updateTimer->stop();
	}
}




const NetworkObjectList& NetworkObjectDirectory::objects( const NetworkObject& parent ) const
{
	if( parent.type() == NetworkObject::Type::Root || parent.isContainer() )
	{
		const auto it = m_objects.constFind( parent.modelId() );
		if( it != m_objects.end() )
		{
			return it.value();
		}
	}

	return m_defaultObjectList;
}



const NetworkObject& NetworkObjectDirectory::object( NetworkObject::ModelId parent, NetworkObject::ModelId object ) const
{
	if( object == rootId() )
	{
		return m_rootObject;
	}

	const auto it = m_objects.constFind( parent );
	if( it != m_objects.end() )
	{
		for( const auto& entry : *it )
		{
			if( entry.modelId() == object )
			{
				return entry;
			}
		}
	}

	return m_invalidObject;
}



int NetworkObjectDirectory::index( NetworkObject::ModelId parent, NetworkObject::ModelId child ) const
{
	const auto it = m_objects.constFind( parent );
	if( it != m_objects.end() )
	{
		int index = 0;
		for( const auto& entry : *it )
		{
			if( entry.modelId() == child )
			{
				return index;
			}
			++index;
		}
	}

	return -1;
}



int NetworkObjectDirectory::childCount( NetworkObject::ModelId parent ) const
{
	const auto it = m_objects.constFind( parent );
	if( it != m_objects.end() )
	{
		return it->count();
	}

	return 0;
}



NetworkObject::ModelId NetworkObjectDirectory::childId( NetworkObject::ModelId parent, int index ) const
{
	const auto it = m_objects.constFind( parent );
	if( it != m_objects.end() )
	{
		if( index < it->count() )
		{
			return it->at( index ).modelId();
		}
	}

	return 0;
}



NetworkObject::ModelId NetworkObjectDirectory::parentId( NetworkObject::ModelId child ) const
{
	if( child == rootId() )
	{
		return 0;
	}

	for( auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it )
	{
		const auto& objectList = it.value();

		for( const auto& object : objectList )
		{
			if( object.modelId() == child )
			{
				return it.key();
			}
		}
	}

	return 0;
}



NetworkObject::ModelId NetworkObjectDirectory::rootId() const
{
	return m_rootObject.modelId();
}



QVariant NetworkObjectDirectory::queryObjectProperty(NetworkObject::Uid objectUid, NetworkObject::Property property)
{
	for( auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it )
	{
		const auto& objectList = it.value();

		for( const auto& object : objectList )
		{
			if (object.uid() == objectUid)
			{
				return object.property(property);
			}
		}
	}

	return {};
}



NetworkObjectList NetworkObjectDirectory::queryObjects( NetworkObject::Type type,
														NetworkObject::Property property,
														const QVariant& value )
{
	if( hasObjects() == false )
	{
		update();
	}

	NetworkObjectList objects;

	for( auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it )
	{
		const auto& objectList = it.value();

		for( const auto& object : objectList )
		{
			if( ( type == NetworkObject::Type::None || object.type() == type ) &&
				( property == NetworkObject::Property::None ||
				  object.isPropertyValueEqual( property, value, Qt::CaseInsensitive ) ) )
			{
				objects.append( object );
			}
		}
	}

	return objects;
}



NetworkObjectList NetworkObjectDirectory::queryParents( const NetworkObject& child )
{
	if( hasObjects() == false )
	{
		update();
	}

	if( child.type() == NetworkObject::Type::Root )
	{
		return {};
	}

	for( auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it )
	{
		const auto& objectList = it.value();

		for( const auto& object : objectList )
		{
			if( object.uid() == child.parentUid() )
			{
				return queryParents( object ) + NetworkObjectList( { object } );
			}
		}
	}

	return {};
}



void NetworkObjectDirectory::fetchObjects( const NetworkObject& object )
{
	if( object.type() == NetworkObject::Type::Root )
	{
		update();
		m_rootObject.setPopulated();
	}

	setObjectPopulated( object );
}



bool NetworkObjectDirectory::hasObjects() const
{
	return m_objects.size() > 1;
}



void NetworkObjectDirectory::addOrUpdateObject( const NetworkObject& networkObject, const NetworkObject& parent )
{
	if( m_objects.contains( parent.modelId() ) == false )
	{
		vCritical() << "parent" << parent.toJson() << "does not exist for object" << networkObject.toJson();
		return;
	}

	auto completeNetworkObject = networkObject;
	if( completeNetworkObject.parentUid().isNull() )
	{
		completeNetworkObject.setParentUid( parent.uid() );
	}

	auto& objectList = m_objects[parent.modelId()]; // clazy:exclude=detaching-member
	const auto index = objectList.indexOf( completeNetworkObject );

	if( index < 0 )
	{
		Q_EMIT objectsAboutToBeInserted(parent.modelId(), objectList.count(), 1);

		objectList.append( completeNetworkObject );
		if( completeNetworkObject.isContainer() )
		{
			m_objects[completeNetworkObject.modelId()] = {};
		}

		Q_EMIT objectsInserted();

		propagateChildObjectChange(parent.modelId());
	}
	else if( objectList[index].exactMatch( completeNetworkObject ) == false )
	{
		objectList.replace( index, completeNetworkObject );
		propagateChildObjectChange(parent.modelId());
	}
}



void NetworkObjectDirectory::removeObjects( const NetworkObject& parent, const NetworkObjectFilter& removeObjectFilter )
{
	if( m_objects.contains( parent.modelId() ) == false )
	{
		return;
	}

	auto& objectList = m_objects[parent.modelId()]; // clazy:exclude=detaching-member
	int index = 0;
	QList<NetworkObject::ModelId> objectsToRemove;

	for( auto it = objectList.begin(); it != objectList.end(); )
	{
		if( removeObjectFilter( *it ) )
		{
			if( it->isContainer() )
			{
				objectsToRemove.append( it->modelId() );
			}

			Q_EMIT objectsAboutToBeRemoved(parent.modelId(), index, 1);
			it = objectList.erase( it );
			Q_EMIT objectsRemoved();
			propagateChildObjectChange(parent.modelId());
		}
		else
		{
			++it;
			++index;
		}
	}

	for( const auto& groupId : objectsToRemove )
	{
		m_objects.remove( groupId );
	}
}



void NetworkObjectDirectory::replaceObjects( const NetworkObjectList& objects, const NetworkObject& parent )
{
	for( const auto& object : objects )
	{
		addOrUpdateObject( object, parent );
	}

	removeObjects( parent, [&objects]( const NetworkObject& object ) { return objects.contains( object ) == false; } );
}



void NetworkObjectDirectory::setObjectPopulated( const NetworkObject& networkObject )
{
	const auto objectModelId = networkObject.modelId();

	auto it = m_objects.find( parentId( objectModelId ) ); // clazy:exclude=detaching-member
	if( it != m_objects.end() )
	{
		for( auto& entry : *it )
		{
			if( entry.modelId() == objectModelId )
			{
				entry.setPopulated();
				break;
			}
		}
	}
}



void NetworkObjectDirectory::propagateChildObjectChange(NetworkObject::ModelId objectId, int depth)
{
	if (objectId != 0)
	{
		if (m_changedObjectIds.contains(objectId))
		{
			m_changedObjectIds.removeAll(objectId);
		}

		m_changedObjectIds.append(objectId);

		propagateChildObjectChange(parentId(objectId), depth+1);

		if (depth == 0)
		{
			m_propagateChangedObjectsTimer->stop();
			m_propagateChangedObjectsTimer->start();
		}
	}
}



void NetworkObjectDirectory::propagateChildObjectChanges()
{
	for (auto it = m_changedObjectIds.constBegin(), end = m_changedObjectIds.constEnd();
		 it != end; ++it)
	{
		const auto parentObjectId = parentId(*it);
		const auto childIndex = index(parentObjectId, *it);

		Q_EMIT objectChanged(parentObjectId, childIndex);
	}

	m_changedObjectIds.clear();
}
