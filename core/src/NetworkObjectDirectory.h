/*
 * NetworkObjectDirectory.h - base class for network object directory implementations
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QHash>
#include <QObject>

#include "NetworkObject.h"

class QTimer;

class VEYON_CORE_EXPORT NetworkObjectDirectory : public QObject
{
	Q_OBJECT
public:
	enum {
		MinimumUpdateInterval = 10,
		DefaultUpdateInterval = 60,
		MaximumUpdateInterval = 3600
	};

	explicit NetworkObjectDirectory( QObject* parent );

	void setUpdateInterval( int interval );

	const NetworkObjectList& objects( const NetworkObject& parent ) const;

	const NetworkObject& object( NetworkObject::ModelId parent, NetworkObject::ModelId object ) const;
	int index( NetworkObject::ModelId parent, NetworkObject::ModelId child ) const;
	int childCount( NetworkObject::ModelId parent ) const;
	NetworkObject::ModelId childId( NetworkObject::ModelId parent, int index ) const;
	NetworkObject::ModelId parentId( NetworkObject::ModelId child ) const;

	NetworkObject::ModelId rootId() const;

	virtual NetworkObjectList queryObjects( NetworkObject::Type type,
											NetworkObject::Attribute attribute, const QVariant& value );
	virtual NetworkObjectList queryParents( const NetworkObject& child );

	virtual void update() = 0;
	virtual void fetchObjects( const NetworkObject& object );

protected:
	using NetworkObjectFilter = std::function<bool (const NetworkObject &)>;

	bool hasObjects() const;
	void addOrUpdateObject( const NetworkObject& networkObject, const NetworkObject& parent );
	void removeObjects( const NetworkObject& parent, const NetworkObjectFilter& removeObjectFilter );
	void replaceObjects( const NetworkObjectList& objects, const NetworkObject& parent );
	void setObjectPopulated( const NetworkObject& networkObject );

private:
	QTimer* m_updateTimer;
	QHash<NetworkObject::ModelId, NetworkObjectList> m_objects;
	NetworkObject m_invalidObject;
	NetworkObject m_rootObject;
	NetworkObjectList m_defaultObjectList;

Q_SIGNALS:
	void objectsAboutToBeInserted( const NetworkObject& parent, int index, int count );
	void objectsInserted();
	void objectsAboutToBeRemoved( const NetworkObject& parent, int index, int count );
	void objectsRemoved();
	void objectChanged( const NetworkObject& parent, int index );

};
