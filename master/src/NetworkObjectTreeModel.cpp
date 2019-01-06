/*
 * NetworkObjectTreeModel.cpp - data model returning hierarchically grouped network objects
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

#include "NetworkObjectDirectory.h"
#include "NetworkObjectTreeModel.h"


NetworkObjectTreeModel::NetworkObjectTreeModel( NetworkObjectDirectory* directory, QObject* parent ) :
	NetworkObjectModel( parent ),
	m_directory( directory )
{
	connect( m_directory, &NetworkObjectDirectory::objectsAboutToBeInserted,
			 this, &NetworkObjectTreeModel::beginInsertObjects );
	connect( m_directory, &NetworkObjectDirectory::objectsInserted,
			 this, &NetworkObjectTreeModel::endInsertObjects );

	connect( m_directory, &NetworkObjectDirectory::objectsAboutToBeRemoved,
			 this, &NetworkObjectTreeModel::beginRemoveObjects );
	connect( m_directory, &NetworkObjectDirectory::objectsRemoved,
			 this, &NetworkObjectTreeModel::endRemoveObjects );

	connect( m_directory, &NetworkObjectDirectory::objectChanged,
			 this, &NetworkObjectTreeModel::updateObject );
}



QModelIndex NetworkObjectTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if( parent.isValid() && parent.column() != 0 )
	{
		return QModelIndex();
	}

	int parentId = 0;
	if( parent.isValid() && parent.internalId() == 0 )
	{
		parentId = parent.row() + 1;
	}

	return createIndex( row, column, static_cast<quintptr>(parentId) );
}



QModelIndex NetworkObjectTreeModel::parent( const QModelIndex& index ) const
{
	// parent ID set?
	if( index.internalId() > 0 )
	{
		// use it as row index
		return createIndex( static_cast<int>(index.internalId()) - 1, 0 );
	}

	return QModelIndex();
}



int NetworkObjectTreeModel::rowCount( const QModelIndex& parent ) const
{
	if( !parent.isValid() )
	{
		return m_directory->objects( NetworkObject( NetworkObject::Root ) ).count();
	}

	if( parent.internalId() == 0 )
	{
		const auto rootObjects = m_directory->objects( NetworkObject( NetworkObject::Root ) );
		const NetworkObject groupObject = rootObjects[parent.row()];

		return m_directory->objects( groupObject ).count();
	}

	return 0;
}



int NetworkObjectTreeModel::columnCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent)

	return 1;
}



QVariant NetworkObjectTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole )
	{
		return tr( "Room/Computer" );
	}

	return QVariant();
}



QVariant NetworkObjectTreeModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() == false )
	{
		return QVariant();
	}

	NetworkObject networkObject;

	const auto rootObjects = m_directory->objects( NetworkObject( NetworkObject::Root ) );

	if( index.internalId() > 0 )
	{
		const auto& groupObject = rootObjects[static_cast<int>(index.internalId())-1];
		const auto groupObjects = m_directory->objects( groupObject );

		networkObject = groupObjects[index.row()];
	}
	else
	{
		networkObject = rootObjects[index.row()];
	}

	switch( role )
	{
	case UidRole: return networkObject.uid();
	case NameRole: return networkObject.name();
	case TypeRole: return networkObject.type();
	case HostAddressRole: return networkObject.hostAddress();
	case MacAddressRole: return networkObject.macAddress();
	case DirectoryAddressRole: return networkObject.directoryAddress();
	default: break;
	}

	return QVariant();
}



void NetworkObjectTreeModel::beginInsertObjects( const NetworkObject& parent, int index, int count )
{
	if( parent.type() == NetworkObject::Root )
	{
		beginInsertRows( createIndex( -1, -1 ), index, index+count-1 );
	}
	else if( parent.type() == NetworkObject::Group )
	{
		int groupIndex = 0;
		const auto rootObjects = m_directory->objects( NetworkObject( NetworkObject::Root ) );
		for( const auto& groupObject : rootObjects )
		{
			if( groupObject == parent )
			{
				beginInsertRows( createIndex( groupIndex, 0 ), index, index+count-1 );
				break;
			}
			++groupIndex;
		}
	}
}



void NetworkObjectTreeModel::endInsertObjects()
{
	endInsertRows();
}



void NetworkObjectTreeModel::beginRemoveObjects( const NetworkObject& parent, int index, int count )
{
	if( parent.type() == NetworkObject::Root )
	{
		beginRemoveRows( createIndex( -1, -1 ), index, index+count-1 );
	}
	else if( parent.type() == NetworkObject::Group )
	{
		int groupIndex = 0;
		const auto rootObjects = m_directory->objects( NetworkObject( NetworkObject::Root ) );
		for( const auto& groupObject : rootObjects )
		{
			if( groupObject == parent )
			{
				beginRemoveRows( createIndex( groupIndex, 0 ), index, index+count-1 );
				break;
			}
			++groupIndex;
		}
	}
}



void NetworkObjectTreeModel::endRemoveObjects()
{
	endRemoveRows();
}



void NetworkObjectTreeModel::updateObject( const NetworkObject& parent, int row )
{
	const auto index = objectIndex( parent, row );

	emit dataChanged( index, index );
}



QModelIndex NetworkObjectTreeModel::objectIndex( const NetworkObject& parent, int row ) const
{
	if( parent.type() == NetworkObject::Root )
	{
		return index( row, 0 );
	}
	else if( parent.type() == NetworkObject::Group )
	{
		int groupIndex = 0;
		const auto rootObjects = m_directory->objects( NetworkObject( NetworkObject::Root ) );
		for( const auto& groupObject : rootObjects )
		{
			if( groupObject == parent )
			{
				auto parentIndex = createIndex( groupIndex, 0 );
				return index( row, 0, parentIndex );
			}
			++groupIndex;
		}
	}

	return QModelIndex();
}
