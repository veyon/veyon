/*
 * ComputerManager.cpp - maintains and provides a computer object list
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

#include <QCoreApplication>
#include <QFile>
#include <QHostAddress>
#include <QHostInfo>
#include <QMessageBox>
#include <QTimer>

#include "BuiltinFeatures.h"
#include "ComputerManager.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"
#include "NetworkObject.h"
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectFilterProxyModel.h"
#include "NetworkObjectOverlayDataModel.h"
#include "NetworkObjectTreeModel.h"
#include "UserConfig.h"
#include "UserSessionControl.h"


ComputerManager::ComputerManager( UserConfig& config,
								  FeatureManager& featureManager,
								  BuiltinFeatures& builtinFeatures,
								  QObject* parent ) :
	QObject( parent ),
	m_config( config ),
	m_featureManager( featureManager ),
	m_builtinFeatures( builtinFeatures ),
	m_networkObjectDirectoryManager( new NetworkObjectDirectoryManager() ),
	m_networkObjectDirectory( m_networkObjectDirectoryManager->createDirectory( this ) ),
	m_networkObjectModel( new NetworkObjectTreeModel( m_networkObjectDirectory ) ),
	m_networkObjectOverlayDataModel( new NetworkObjectOverlayDataModel( 1, Qt::DisplayRole, tr( "User" ), this ) ),
	m_computerTreeModel( new CheckableItemProxyModel( NetworkObjectModel::UidRole, this ) ),
	m_networkObjectFilterProxyModel( new NetworkObjectFilterProxyModel( this ) )
{
	if( m_networkObjectDirectory == nullptr )
	{
		QMessageBox::critical( nullptr,
							   tr( "Missing network object directory plugin" ),
							   tr( "No default network object directory plugin was found. "
								   "Please check your installation or configure a different "
								   "network object directory backend via %1 Configurator." ).
							   arg( VeyonCore::applicationName() ) );
		qFatal( "ComputerManager: missing network object directory plugin!" );
	}

	initNetworkObjectLayer();
	initRooms();
	initComputerTreeModel();
	updateComputerList();

	auto computerScreenUpdateTimer = new QTimer( this );
	connect( computerScreenUpdateTimer, &QTimer::timeout, this, &ComputerManager::updateComputerScreens );
	computerScreenUpdateTimer->start( 1000 );		// TODO: replace constant
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



void ComputerManager::removeRoom( const QString& room )
{
	if( m_currentRooms.contains( room ) == false )
	{
		m_roomFilterList.removeAll( room );

		updateRoomFilterList();
	}
}



bool ComputerManager::saveComputerAndUsersList( const QString& fileName )
{
	QStringList lines( tr( "Computer name;Host name;User" ) );

	for( auto& computer : m_computerList )
	{
		QModelIndex networkObjectIndex = findNetworkObject( computer.networkObjectUid() );
		if( networkObjectIndex.isValid() )
		{
			// create index for user column
			networkObjectIndex = m_networkObjectOverlayDataModel->
					index( networkObjectIndex.row(), 1, networkObjectIndex.parent() );
			// fetch user
			QString user = m_networkObjectOverlayDataModel->data( networkObjectIndex ).toString();
			// create new line with computer and user
			lines += computer.name() + ";" + computer.hostAddress() + ";" + user;
		}
	}

	// append empty string to generate final newline at end of file
	lines += QString();

	QFile outputFile( fileName );
	if( outputFile.open( QFile::WriteOnly | QFile::Truncate ) == false )
	{
		return false;
	}

	outputFile.write( lines.join( "\r\n" ).toUtf8() );

	return true;
}



void ComputerManager::reloadComputerList()
{
	emit computerListAboutToBeReset();
	m_computerList = getCheckedComputers( QModelIndex() );

	for( auto& computer : m_computerList )
	{
		startComputerControlInterface( computer );
	}

	emit computerListReset();
}



void ComputerManager::updateComputerList()
{
	ComputerList newComputerList = getCheckedComputers( QModelIndex() );

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



void ComputerManager::initRooms()
{
	QString localHostName = QHostInfo::localHostName();
	const auto localHostAddresses = QHostInfo::fromName( localHostName ).addresses();

	for( const auto& address : localHostAddresses )
	{
		qDebug() << "ComputerManager::initRooms(): initializing rooms for"
				 << QString( "%1 (%2)" ).arg( localHostName, address.toString() );
	}
	m_currentRooms.append( findRoomOfComputer( localHostName, localHostAddresses, QModelIndex() ) );

	qDebug() << "ComputerManager::initRooms(): found local rooms" << m_currentRooms;

	if( VeyonCore::config().onlyCurrentRoomVisible() )
	{
		if( m_currentRooms.isEmpty() )
		{
			QMessageBox::warning( nullptr,
								  tr( "Room detection failed" ),
								  tr( "Could not determine the room which this computer belongs to. "
									  "This indicates a problem with the system configuration. "
									  "All rooms will be shown in the computer management instead." ) );
			qWarning( "ComputerManager::initRoomFilterList(): room detection failed" );
		}

		m_roomFilterList = m_currentRooms;
		updateRoomFilterList();
	}
}



void ComputerManager::initNetworkObjectLayer()
{
	m_networkObjectOverlayDataModel->setSourceModel( m_networkObjectModel );
	m_networkObjectFilterProxyModel->setSourceModel( m_networkObjectOverlayDataModel );
	m_computerTreeModel->setSourceModel( m_networkObjectFilterProxyModel );

	if( VeyonCore::config().localComputerHidden() )
	{
		m_networkObjectFilterProxyModel->setComputerExcludeFilter( QStringList( QHostInfo::localHostName() ) );
	}

	m_networkObjectFilterProxyModel->setEmptyGroupsExcluded( VeyonCore::config().emptyRoomsHidden() );
}



void ComputerManager::initComputerTreeModel()
{
	QJsonArray checkedNetworkObjects;
	if( VeyonCore::config().autoSwitchToCurrentRoom() )
	{
		for( const auto& room : qAsConst( m_currentRooms ) )
		{
			const auto computersInRoom = getComputersInRoom( room );
			for( const auto& computer : computersInRoom )
			{
				checkedNetworkObjects += computer.networkObjectUid().toString();
			}
		}
	}
	else
	{
		checkedNetworkObjects = m_config.checkedNetworkObjects();
	}

	m_computerTreeModel->loadStates( checkedNetworkObjects );

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
}



void ComputerManager::updateRoomFilterList()
{
	if( VeyonCore::config().onlyCurrentRoomVisible() )
	{
		m_networkObjectFilterProxyModel->setGroupFilter( m_roomFilterList );
	}
}



QString ComputerManager::findRoomOfComputer( const QString& hostName, const QList<QHostAddress>& hostAddresses, const QModelIndex& parent )
{
	QAbstractItemModel* model = networkObjectModel();

	int rows = model->rowCount( parent );

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		auto objectType = static_cast<NetworkObject::Type>( model->data( entryIndex, NetworkObjectModel::TypeRole ).toInt() );

		if( objectType == NetworkObject::Group )
		{
			QString room = findRoomOfComputer( hostName, hostAddresses, entryIndex );
			if( room.isEmpty() == false )
			{
				return room;
			}
		}
		else if( objectType == NetworkObject::Host )
		{
			QString currentHost = model->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString();
			QHostAddress currentHostAddress;

			if( hostName.toLower() == currentHost.toLower() ||
					( currentHostAddress.setAddress( currentHost ) && hostAddresses.contains( currentHostAddress ) ) )
			{
				return model->data( parent, NetworkObjectModel::NameRole ).toString();
			}
		}
	}

	return QString();
}



ComputerList ComputerManager::getComputersInRoom( const QString& roomName, const QModelIndex& parent )
{
	QAbstractItemModel* model = computerTreeModel();

	int rows = model->rowCount( parent );

	ComputerList computers;

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		auto objectType = static_cast<NetworkObject::Type>( model->data( entryIndex, NetworkObjectModel::TypeRole ).toInt() );

		switch( objectType )
		{
		case NetworkObject::Group:
			if( model->data( entryIndex, NetworkObjectModel::NameRole ).toString() == roomName )
			{
				computers += getComputersInRoom( roomName, entryIndex );
			}
			break;
		case NetworkObject::Host:
			computers += Computer( model->data( entryIndex, NetworkObjectModel::UidRole ).toUuid(),
								   model->data( entryIndex, NetworkObjectModel::NameRole ).toString(),
								   model->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString(),
								   model->data( entryIndex, NetworkObjectModel::MacAddressRole ).toString() );
			break;
		default: break;
		}
	}

	return computers;
}



ComputerList ComputerManager::getCheckedComputers(const QModelIndex &parent)
{
	QAbstractItemModel* model = computerTreeModel();

	int rows = model->rowCount( parent );

	ComputerList computers;

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		if( model->data( entryIndex, NetworkObjectModel::CheckStateRole ).value<Qt::CheckState>() == Qt::Unchecked )
		{
			continue;
		}

		auto objectType = static_cast<NetworkObject::Type>( model->data( entryIndex, NetworkObjectModel::TypeRole ).toInt() );

		switch( objectType )
		{
		case NetworkObject::Group:
			computers += getCheckedComputers( entryIndex );
			break;
		case NetworkObject::Host:
			computers += Computer( model->data( entryIndex, NetworkObjectModel::UidRole ).toUuid(),
								   model->data( entryIndex, NetworkObjectModel::NameRole ).toString(),
								   model->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString(),
								   model->data( entryIndex, NetworkObjectModel::MacAddressRole ).toString(),
								   model->data( parent, NetworkObjectModel::NameRole ).toString() );
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
	computer.controlInterface().start( computerScreenSize(),
									   &m_builtinFeatures.userSessionControl() );

	connect( &computer.controlInterface(), &ComputerControlInterface::featureMessageReceived,
			 &m_featureManager, &FeatureManager::handleMasterFeatureMessage );

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
			if( model->data( entryIndex, NetworkObjectModel::UidRole ).toUuid() == networkObjectUid )
			{
				return entryIndex;
			}
		}
	}

	return QModelIndex();
}
