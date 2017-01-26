/*
 * ComputerManager.cpp - maintains and provides a computer object list
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

#include "ComputerManager.h"
#include "NetworkObject.h"
#include "NetworkObjectModelFactory.h"
#include "NetworkObjectTreeModel.h"


ComputerManager::ComputerManager( QObject* parent ) :
	QObject( parent ),
	m_networkObjectModel( NetworkObjectModelFactory().create( this ) ),
	m_globalMode( Computer::ModeMonitoring )
{
	connect( m_networkObjectModel, &QAbstractItemModel::dataChanged,
			 this, &ComputerManager::updateComputerData );

	connect( m_networkObjectModel, &QAbstractItemModel::modelReset,
			 this, &ComputerManager::reloadComputerList );

	connect( m_networkObjectModel, &QAbstractItemModel::layoutChanged,
			 this, &ComputerManager::updateComputerList );
	connect( m_networkObjectModel, &QAbstractItemModel::rowsInserted,
			 this, &ComputerManager::updateComputerList );
	connect( m_networkObjectModel, &QAbstractItemModel::rowsRemoved,
			 this, &ComputerManager::updateComputerList );
}



void ComputerManager::setGlobalMode( Computer::Mode mode )
{
	for( auto computer : m_computerList )
	{
		//computer.setMode( mode )
	}

	m_globalMode = mode;
}



void ComputerManager::updateComputerData()
{
	emit dataChanged();
}



void ComputerManager::reloadComputerList()
{
	emit computerListAboutToBeReset();
	m_computerList = getComputers( QModelIndex() );
	emit computerListReset();
}



void ComputerManager::updateComputerList()
{
	ComputerList newComputerList = getComputers( QModelIndex() );

	int index = 0;

	for( auto computerIt = m_computerList.begin(); computerIt != m_computerList.end(); )
	{
		if( newComputerList.contains( *computerIt ) == false )
		{
			emit computerAboutToBeRemoved( index );
			computerIt = m_computerList.erase( computerIt );
			emit computerRemoved();
		}
		else
		{
			++computerIt;
			++index;
		}
	}

	index = 0;

	for( const auto& computer : newComputerList )
	{
		if( index < m_computerList.count() && m_computerList[index] != computer )
		{
			emit computerAboutToBeInserted( index );
			m_computerList.insert( index, computer );
			emit computerInserted();
		}
		else if( index >= m_computerList.count() )
		{
			emit computerAboutToBeInserted( index );
			m_computerList.append( computer );
			emit computerInserted();
		}
		++index;
	}
}



ComputerList ComputerManager::getComputers(const QModelIndex &parent)
{
	if( m_networkObjectModel == Q_NULLPTR )
	{
		return ComputerList();
	}

	int rows = m_networkObjectModel->rowCount( parent );

	ComputerList computers;

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = m_networkObjectModel->index( i, 0, parent );

		if( m_networkObjectModel->data( entryIndex, NetworkObjectTreeModel::CheckStateRole ).value<Qt::CheckState>() ==
				Qt::Unchecked )
		{
			continue;
		}

		auto objectType = static_cast<NetworkObject::Type>(
					m_networkObjectModel->data( entryIndex, NetworkObjectTreeModel::NetworkObjectTypeRole ).toInt() );

		switch( objectType )
		{
		case NetworkObject::Group:
			computers += getComputers( entryIndex );
			break;
		case NetworkObject::Host:
			computers += Computer( m_networkObjectModel->data( entryIndex, NetworkObjectTreeModel::NetworkObjectUidRole ).toUInt(),
								   m_networkObjectModel->data( entryIndex, NetworkObjectTreeModel::NetworkObjectNameRole ).toString(),
								   m_networkObjectModel->data( entryIndex, NetworkObjectTreeModel::NetworkobjectHostAddressRole ).toString(),
								   m_networkObjectModel->data( entryIndex, NetworkObjectTreeModel::NetworkObjectMacAddressRole ).toString() );
			break;
		default: break;
		}
	}

	return computers;
}
