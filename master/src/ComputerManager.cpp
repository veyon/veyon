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

#include <QHostAddress>
#include <QHostInfo>
#include <QMessageBox>
#include <QTimer>

#include "ComputerManager.h"
#include "ItalcConfiguration.h"
#include "NetworkObject.h"
#include "NetworkObjectModelFactory.h"
#include "NetworkObjectOverlayDataModel.h"
#include "NetworkObjectTreeModel.h"
#include "StringListFilterProxyModel.h"
#include "UserConfig.h"


ComputerManager::ComputerManager( UserConfig& config, QObject* parent ) :
	QObject( parent ),
	m_config( config ),
	m_networkObjectModel( NetworkObjectModelFactory().create( this ) ),
	m_networkObjectOverlayDataModel( new NetworkObjectOverlayDataModel( 1, Qt::DisplayRole, tr( "User" ), this ) ),
	m_computerTreeModel( new CheckableItemProxyModel( NetworkObjectModel::UidRole, this ) ),
	m_networkObjectSortFilterProxyModel( new StringListFilterProxyModel( this ) )
{
	m_networkObjectOverlayDataModel->setSourceModel( m_networkObjectModel );
	m_networkObjectSortFilterProxyModel->setSourceModel( m_networkObjectOverlayDataModel );
	m_computerTreeModel->setSourceModel( m_networkObjectSortFilterProxyModel );

	connect( computerTreeModel(), &QAbstractItemModel::modelReset,
			 this, &ComputerManager::reloadComputerList );
	connect( computerTreeModel(), &QAbstractItemModel::layoutChanged,
			 this, &ComputerManager::reloadComputerList );

	connect( computerTreeModel(), &QAbstractItemModel::dataChanged,
			 this, &ComputerManager::updateComputerList );
	connect( computerTreeModel(), &QAbstractItemModel::rowsInserted,
			 this, &ComputerManager::updateComputerList );
	connect( computerTreeModel(), &QAbstractItemModel::rowsRemoved,
			 this, &ComputerManager::updateComputerList );

	m_computerTreeModel->loadStates( m_config.checkedNetworkObjects() );

	QTimer* computerScreenUpdateTimer = new QTimer( this );
	connect( computerScreenUpdateTimer, &QTimer::timeout, this, &ComputerManager::updateComputerScreens );
	computerScreenUpdateTimer->start( 1000 );		// TODO: replace constant

	initRoomFilterList();
}



ComputerManager::~ComputerManager()
{
	m_config.setCheckedNetworkObjects( m_computerTreeModel->saveStates() );
}



ComputerControlInterfaceList ComputerManager::computerControlInterfaces()
{
	ComputerControlInterfaceList interfaces;

	for( auto& computer : m_computerList )
	{
		interfaces += &computer.controlInterface();
	}

	return interfaces;
}



void ComputerManager::updateComputerScreenSize()
{
	for( auto& computer : m_computerList )
	{
		computer.controlInterface().setScaledScreenSize( computerScreenSize() );
	}

	emit computerScreenSizeChanged();
}



void ComputerManager::addRoom( const QString& room )
{
	m_roomFilterList.append( room );

	updateRoomFilterList();
}



void ComputerManager::reloadComputerList()
{
	emit computerListAboutToBeReset();
	m_computerList = getComputers( QModelIndex() );

	for( auto& computer : m_computerList )
	{
		startComputerControlInterface( computer );
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
			startComputerControlInterface( m_computerList[index] );
			emit computerInserted();
		}
		else if( index >= m_computerList.count() )
		{
			emit computerAboutToBeInserted( index );
			m_computerList.append( computer );
			startComputerControlInterface( m_computerList.last() );
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



void ComputerManager::initRoomFilterList()
{
	if( ItalcCore::config->onlyCurrentRoomVisible() )
	{
		for( auto hostAddress : QHostInfo::fromName( QHostInfo::localHostName() ).addresses() )
		{
			qDebug() << "ComputerManager::initRoomFilterList(): adding room for" << hostAddress;
			m_roomFilterList.append( findRoomOfComputer( hostAddress, QModelIndex() ) );
		}

		if( m_roomFilterList.isEmpty() )
		{
			QMessageBox::warning( nullptr,
								  tr( "Room detection failed" ),
								  tr( "Could not determine the room which this computer belongs to. "
									  "This indicates a problem with the system configuration. "
									  "All rooms will be shown in the computer management instead." ) );
			qWarning( "ComputerManager::initRoomFilterList(): room detection failed" );
		}

		updateRoomFilterList();
	}
}



void ComputerManager::updateRoomFilterList()
{
	if( ItalcCore::config->onlyCurrentRoomVisible() )
	{
		m_networkObjectSortFilterProxyModel->setStringList( m_roomFilterList );
	}
}



QString ComputerManager::findRoomOfComputer( const QHostAddress& hostAddress, const QModelIndex& parent )
{
	QAbstractItemModel* model = networkObjectModel();

	int rows = model->rowCount( parent );

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		auto objectType = static_cast<NetworkObject::Type>( model->data( entryIndex, NetworkObjectModel::TypeRole ).toInt() );

		if( objectType == NetworkObject::Group )
		{
			QString room = findRoomOfComputer( hostAddress, entryIndex );
			if( room.isEmpty() == false )
			{
				return room;
			}
		}
		else if( objectType == NetworkObject::Host )
		{
			QString currentHostAddress = model->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString();

			if ( QHostAddress( currentHostAddress ) == hostAddress ||
				 QHostInfo::fromName( currentHostAddress ).addresses().contains( hostAddress ) )
			{
				return model->data( parent, NetworkObjectModel::NameRole ).toString();
			}
		}
	}

	return QString();
}



ComputerList ComputerManager::getComputers(const QModelIndex &parent)
{
	int rows = computerTreeModel()->rowCount( parent );

	ComputerList computers;

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = computerTreeModel()->index( i, 0, parent );

		if( computerTreeModel()->data( entryIndex, NetworkObjectModel::CheckStateRole ).value<Qt::CheckState>() ==
				Qt::Unchecked )
		{
			continue;
		}

		auto objectType = static_cast<NetworkObject::Type>(
					computerTreeModel()->data( entryIndex, NetworkObjectModel::TypeRole ).toInt() );

		switch( objectType )
		{
		case NetworkObject::Group:
			computers += getComputers( entryIndex );
			break;
		case NetworkObject::Host:
			computers += Computer( computerTreeModel()->data( entryIndex, NetworkObjectModel::UidRole ).value<NetworkObject::Uid>(),
								   computerTreeModel()->data( entryIndex, NetworkObjectModel::NameRole ).toString(),
								   computerTreeModel()->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString(),
								   computerTreeModel()->data( entryIndex, NetworkObjectModel::MacAddressRole ).toString() );
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



void ComputerManager::startComputerControlInterface( Computer& computer )
{
	computer.controlInterface().start( computerScreenSize() );

	connect( &computer.controlInterface(), &ComputerControlInterface::userChanged,
			 [&] () { updateUser( computer ); } );
}



void ComputerManager::updateUser( Computer& computer )
{
	QModelIndex networkObjectIndex = findNetworkObject( computer.networkObjectUid() );

	if( networkObjectIndex.isValid() )
	{
		networkObjectIndex = m_networkObjectOverlayDataModel->index( networkObjectIndex.row(), 1, networkObjectIndex.parent() );
		m_networkObjectOverlayDataModel->setData( networkObjectIndex,
												  computer.controlInterface().user(),
												  Qt::DisplayRole );
	}
}



QModelIndex ComputerManager::findNetworkObject( const NetworkObject::Uid& networkObjectUid, const QModelIndex& parent )
{
	QAbstractItemModel* model = networkObjectModel();

	int rows = model->rowCount( parent );

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		auto objectType = static_cast<NetworkObject::Type>( model->data( entryIndex, NetworkObjectModel::TypeRole ).toInt() );

		if( objectType == NetworkObject::Group )
		{
			QModelIndex index = findNetworkObject( networkObjectUid, entryIndex );
			if( index.isValid() )
			{
				return index;
			}
		}
		else if( objectType == NetworkObject::Host )
		{
			if( model->data( entryIndex, NetworkObjectModel::UidRole ).value<NetworkObject::Uid>() == networkObjectUid )
			{
				return entryIndex;
			}
		}
	}

	return QModelIndex();
}
