/*
 * LocalDataConfigurationPage.cpp - implementation of LocalDataConfigurationPage
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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
#include "LocalDataConfigurationPage.h"
#include "Configuration/UiMapping.h"
#include "NetworkObjectModel.h"

#include "ui_LocalDataConfigurationPage.h"

LocalDataConfigurationPage::LocalDataConfigurationPage( LocalDataConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::LocalDataConfigurationPage),
	m_configuration( configuration )
{
	ui->setupUi(this);

	populateRooms();

	connect( ui->roomTableWidget, &QTableWidget::currentItemChanged,
			 this, &LocalDataConfigurationPage::populateComputers );
}



LocalDataConfigurationPage::~LocalDataConfigurationPage()
{
	delete ui;
}



void LocalDataConfigurationPage::resetWidgets()
{
	populateRooms();

	ui->roomTableWidget->setCurrentCell( 0, 0 );
}



void LocalDataConfigurationPage::connectWidgetsToProperties()
{
}



void LocalDataConfigurationPage::applyConfiguration()
{
}



void LocalDataConfigurationPage::addRoom()
{
	auto networkObjects = m_configuration.networkObjects();

	networkObjects.append( NetworkObject( NetworkObject::Group, tr( "New room" ),
										  QString(), QString(), QString(), QUuid::createUuid() ).
						   toJson() );

	m_configuration.setNetworkObjects( networkObjects );

	populateRooms();

	ui->roomTableWidget->setCurrentCell( ui->roomTableWidget->rowCount()-1, 0 );
}



void LocalDataConfigurationPage::updateRoom()
{
	auto currentRoomIndex = ui->roomTableWidget->currentIndex();
	if( currentRoomIndex.isValid() == false )
	{
		return;
	}

	const auto updatedRoomObject = currentRoomObject();
	auto networkObjects = m_configuration.networkObjects();

	for( auto it = networkObjects.begin(); it != networkObjects.end(); ++it )
	{
		NetworkObject networkObject( it->toObject() );
		if( networkObject.uid() == updatedRoomObject.uid() )
		{
			*it = updatedRoomObject.toJson();
			break;
		}
	}

	m_configuration.setNetworkObjects( networkObjects );

	populateRooms();

	ui->roomTableWidget->setCurrentIndex( currentRoomIndex );
}



void LocalDataConfigurationPage::removeRoom()
{
	const auto currentRoomUid = currentRoomObject().uid();
	auto networkObjects = m_configuration.networkObjects();

	for( auto it = networkObjects.begin(); it != networkObjects.end(); )
	{
		NetworkObject networkObject( it->toObject() );
		if( networkObject.uid() == currentRoomUid )
		{
			it = networkObjects.erase( it );
		}
		else
		{
			++it;
		}
	}

	m_configuration.setNetworkObjects( networkObjects );

	populateRooms();
}



void LocalDataConfigurationPage::addComputer()
{
	auto currentRoomUid = currentRoomObject().uid();
	if( currentRoomUid.isNull() )
	{
		return;
	}

	auto networkObjects = m_configuration.networkObjects();

	NetworkObject networkObject( NetworkObject::Host, tr( "New computer" ),
								 QString(), QString(), QString(),
								 QUuid::createUuid(),
								 currentRoomUid );

	networkObjects.append( networkObject.toJson() );

	m_configuration.setNetworkObjects( networkObjects );

	populateComputers();

	ui->computerTableWidget->setCurrentCell( ui->computerTableWidget->rowCount()-1, 0 );
}



void LocalDataConfigurationPage::updateComputer()
{
	auto currentComputerIndex = ui->computerTableWidget->currentIndex();
	if( currentComputerIndex.isValid() == false )
	{
		return;
	}

	const auto currentComputer = currentComputerObject();
	auto networkObjects = m_configuration.networkObjects();

	for( auto it = networkObjects.begin(); it != networkObjects.end(); ++it )
	{
		NetworkObject networkObject( it->toObject() );
		if( networkObject.uid() == currentComputer.uid() )
		{
			*it = currentComputer.toJson();
			break;
		}
	}

	m_configuration.setNetworkObjects( networkObjects );

	populateComputers();

	ui->computerTableWidget->setCurrentIndex( currentComputerIndex );
}



void LocalDataConfigurationPage::removeComputer()
{
	const auto currentComputerUid = currentComputerObject().uid();
	auto networkObjects = m_configuration.networkObjects();

	for( auto it = networkObjects.begin(); it != networkObjects.end(); )
	{
		NetworkObject networkObject( it->toObject() );
		if( networkObject.uid() == currentComputerUid )
		{
			it = networkObjects.erase( it );
		}
		else
		{
			++it;
		}
	}

	m_configuration.setNetworkObjects( networkObjects );

	populateComputers();
}



void LocalDataConfigurationPage::populateRooms()
{
	ui->roomTableWidget->setUpdatesEnabled( false );
	ui->roomTableWidget->clear();

	int rowCount = 0;

	const auto networkObjects = m_configuration.networkObjects();
	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );
		if( networkObject.type() == NetworkObject::Group )
		{
			auto item = new QTableWidgetItem( networkObject.name() );
			item->setData( NetworkObjectModel::UidRole, networkObject.uid() );
			ui->roomTableWidget->setRowCount( ++rowCount );
			ui->roomTableWidget->setItem( rowCount-1, 0, item );
		}
	}

	ui->roomTableWidget->setUpdatesEnabled( true );
}



void LocalDataConfigurationPage::populateComputers()
{
	auto parentUid = currentRoomObject().uid();

	ui->computerTableWidget->setUpdatesEnabled( false );
	ui->computerTableWidget->setRowCount( 0 );

	int rowCount = 0;

	const auto networkObjects = m_configuration.networkObjects();
	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.type() == NetworkObject::Host &&
				networkObject.parentUid() == parentUid )
		{
			auto nameItem = new QTableWidgetItem( networkObject.name() );
			nameItem->setData( NetworkObjectModel::UidRole, networkObject.uid() );
			nameItem->setData( NetworkObjectModel::ParentUidRole, networkObject.parentUid() );

			ui->computerTableWidget->setRowCount( rowCount+1 );
			ui->computerTableWidget->setItem( rowCount, 0, nameItem );
			ui->computerTableWidget->setItem( rowCount, 1, new QTableWidgetItem( networkObject.hostAddress() ) );
			ui->computerTableWidget->setItem( rowCount, 2, new QTableWidgetItem( networkObject.macAddress() ) );
			++rowCount;
		}
	}

	ui->computerTableWidget->setUpdatesEnabled( true );
}



NetworkObject LocalDataConfigurationPage::currentRoomObject() const
{
	const auto selectedRoom = ui->roomTableWidget->currentItem();
	if( selectedRoom )
	{
		return NetworkObject( NetworkObject::Group,
							  selectedRoom->text(),
							  QString(),
							  QString(),
							  QString(),
							  selectedRoom->data( NetworkObjectModel::UidRole ).toUuid(),
							  selectedRoom->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}



NetworkObject LocalDataConfigurationPage::currentComputerObject() const
{
	const int row = ui->computerTableWidget->currentRow();
	if( row >= 0 )
	{
		auto nameItem = ui->computerTableWidget->item( row, 0 );
		auto hostAddressItem = ui->computerTableWidget->item( row, 1 );
		auto macAddressItem = ui->computerTableWidget->item( row, 2 );

		return NetworkObject( NetworkObject::Host,
							  nameItem->text(),
							  hostAddressItem->text(),
							  macAddressItem->text(),
							  QString(),
							  nameItem->data( NetworkObjectModel::UidRole ).toUuid(),
							  nameItem->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}
