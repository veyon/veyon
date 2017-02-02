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
#include "PersonalConfig.h"


ComputerManager::ComputerManager( PersonalConfig& config, QObject* parent ) :
	QObject( parent ),
	m_config( config ),
	m_checkableNetworkObjectProxyModel( new CheckableItemProxyModel( NetworkObjectTreeModel::NetworkObjectUidRole, this ) ),
	m_networkObjectSortProxyModel( new QSortFilterProxyModel( this ) ),
	m_globalMode( Computer::ModeMonitoring )
{
	m_checkableNetworkObjectProxyModel->setSourceModel( NetworkObjectModelFactory().create( this ) );
	m_networkObjectSortProxyModel->setSourceModel( m_checkableNetworkObjectProxyModel );

	connect( networkObjectModel(), &QAbstractItemModel::modelReset,
			 this, &ComputerManager::reloadComputerList );
	connect( networkObjectModel(), &QAbstractItemModel::layoutChanged,
			 this, &ComputerManager::reloadComputerList );

	connect( networkObjectModel(), &QAbstractItemModel::dataChanged,
			 this, &ComputerManager::updateComputerList );
	connect( networkObjectModel(), &QAbstractItemModel::rowsInserted,
			 this, &ComputerManager::updateComputerList );
	connect( networkObjectModel(), &QAbstractItemModel::rowsRemoved,
			 this, &ComputerManager::updateComputerList );

	m_checkableNetworkObjectProxyModel->loadStates( m_config.checkedNetworkObjects() );
}



ComputerManager::~ComputerManager()
{
	m_config.setCheckedNetworkObjects( m_checkableNetworkObjectProxyModel->saveStates() );
}



void ComputerManager::setGlobalMode( Computer::Mode mode )
{
	for( auto computer : m_computerList )
	{
		//computer.setMode( mode )
	}

	m_globalMode = mode;
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
	int rows = networkObjectModel()->rowCount( parent );

	ComputerList computers;

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = networkObjectModel()->index( i, 0, parent );

		if( networkObjectModel()->data( entryIndex, NetworkObjectTreeModel::CheckStateRole ).value<Qt::CheckState>() ==
				Qt::Unchecked )
		{
			continue;
		}

		auto objectType = static_cast<NetworkObject::Type>(
					networkObjectModel()->data( entryIndex, NetworkObjectTreeModel::NetworkObjectTypeRole ).toInt() );

		switch( objectType )
		{
		case NetworkObject::Group:
			computers += getComputers( entryIndex );
			break;
		case NetworkObject::Host:
			computers += Computer( networkObjectModel()->data( entryIndex, NetworkObjectTreeModel::NetworkObjectUidRole ).toUInt(),
								   networkObjectModel()->data( entryIndex, NetworkObjectTreeModel::NetworkObjectNameRole ).toString(),
								   networkObjectModel()->data( entryIndex, NetworkObjectTreeModel::NetworkobjectHostAddressRole ).toString(),
								   networkObjectModel()->data( entryIndex, NetworkObjectTreeModel::NetworkObjectMacAddressRole ).toString() );
			break;
		default: break;
		}
	}

	return computers;
}
