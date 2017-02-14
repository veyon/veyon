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

#include <QTimer>

#include "ComputerManager.h"
#include "FeatureManager.h"
#include "NetworkObject.h"
#include "NetworkObjectModelFactory.h"
#include "NetworkObjectTreeModel.h"
#include "PersonalConfig.h"


ComputerManager::ComputerManager( PersonalConfig& config, QObject* parent ) :
	QObject( parent ),
	m_config( config ),
	m_checkableNetworkObjectProxyModel( new CheckableItemProxyModel( NetworkObjectTreeModel::NetworkObjectUidRole, this ) ),
	m_networkObjectSortProxyModel( new QSortFilterProxyModel( this ) ),
	m_currentMode()
{
	m_networkObjectSortProxyModel->setSourceModel( NetworkObjectModelFactory().create( this ) );
	m_checkableNetworkObjectProxyModel->setSourceModel( m_networkObjectSortProxyModel );

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

	QTimer* computerScreenUpdateTimer = new QTimer( this );
	connect( computerScreenUpdateTimer, &QTimer::timeout, this, &ComputerManager::updateComputerScreens );
	computerScreenUpdateTimer->start( 1000 );		// TODO: replace constant
}



ComputerManager::~ComputerManager()
{
	m_config.setCheckedNetworkObjects( m_checkableNetworkObjectProxyModel->saveStates() );
}



void ComputerManager::updateComputerScreenSize()
{
	for( auto& computer : m_computerList )
	{
		computer.controlInterface().setScaledScreenSize( computerScreenSize() );
	}

	emit computerScreenSizeChanged();
}



void ComputerManager::runFeature( FeatureManager& featureManager, const Feature& feature, QWidget* parent )
{
	ComputerControlInterfaceList computerControlInterfaces;
	for( auto& computer : m_computerList )
	{
		computerControlInterfaces += &computer.controlInterface();
	}

	if( feature.type() == Feature::Mode  )
	{
		featureManager.stopMasterFeature( Feature( m_currentMode ), computerControlInterfaces, parent );

		if( m_currentMode == feature.uid() )
		{
			featureManager.startMasterFeature( featureManager.monitoringModeFeature(), computerControlInterfaces, parent );
			m_currentMode = featureManager.monitoringModeFeature().uid();
		}
		else
		{
			featureManager.startMasterFeature( feature, computerControlInterfaces, parent );
			m_currentMode = feature.uid();
		}
	}
	else
	{
		featureManager.startMasterFeature( feature, computerControlInterfaces, parent );
	}
}



void ComputerManager::reloadComputerList()
{
	emit computerListAboutToBeReset();
	m_computerList = getComputers( QModelIndex() );

	for( auto& computer : m_computerList )
	{
		computer.controlInterface().start( computerScreenSize() );
	}

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
			m_computerList[index].controlInterface().start( computerScreenSize() );
			emit computerInserted();
		}
		else if( index >= m_computerList.count() )
		{
			emit computerAboutToBeInserted( index );
			m_computerList.append( computer );
			m_computerList.last().controlInterface().start( computerScreenSize() );
			emit computerInserted();
		}

		++index;
	}
}



void ComputerManager::updateComputerScreens()
{
	int index = 0;

	for( auto& computer : m_computerList )
	{
		if( computer.controlInterface().hasScreenUpdates() )
		{
			computer.controlInterface().clearScreenUpdateFlag();

			emit computerScreenUpdated( index );
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



QSize ComputerManager::computerScreenSize() const
{
	return QSize( m_config.monitoringScreenSize(),
				  m_config.monitoringScreenSize() * 9 / 16 );
}
