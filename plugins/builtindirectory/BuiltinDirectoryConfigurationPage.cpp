/*
 * BuiltinDirectoryConfigurationPage.cpp - implementation of BuiltinDirectoryConfigurationPage
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonObject>

#include "BuiltinDirectoryConfiguration.h"
#include "BuiltinDirectoryConfigurationPage.h"
#include "Configuration/UiMapping.h"
#include "NetworkObjectModel.h"
#include "ObjectManager.h"

#include "ui_BuiltinDirectoryConfigurationPage.h"

BuiltinDirectoryConfigurationPage::BuiltinDirectoryConfigurationPage( BuiltinDirectoryConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::BuiltinDirectoryConfigurationPage),
	m_configuration( configuration )
{
	ui->setupUi(this);

	populateLocations();

	connect( ui->locationTableWidget, &QTableWidget::currentItemChanged,
			 this, &BuiltinDirectoryConfigurationPage::populateComputers );
}



BuiltinDirectoryConfigurationPage::~BuiltinDirectoryConfigurationPage()
{
	delete ui;
}



void BuiltinDirectoryConfigurationPage::resetWidgets()
{
	populateLocations();

	ui->locationTableWidget->setCurrentCell( 0, 0 );
}



void BuiltinDirectoryConfigurationPage::connectWidgetsToProperties()
{
}



void BuiltinDirectoryConfigurationPage::applyConfiguration()
{
}



void BuiltinDirectoryConfigurationPage::addLocation()
{
	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add( NetworkObject( NetworkObject::Location, tr( "New location" ),
									  QString(), QString(), QString(), QUuid::createUuid() ) );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateLocations();

	ui->locationTableWidget->setCurrentCell( ui->locationTableWidget->rowCount()-1, 0 );
}



void BuiltinDirectoryConfigurationPage::updateLocation()
{
	auto currentLocationIndex = ui->locationTableWidget->currentIndex();
	if( currentLocationIndex.isValid() == false )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.update( currentLocationObject() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateLocations();

	ui->locationTableWidget->setCurrentIndex( currentLocationIndex );
}



void BuiltinDirectoryConfigurationPage::removeLocation()
{
	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.remove( currentLocationObject().uid(), true );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateLocations();
}



void BuiltinDirectoryConfigurationPage::addComputer()
{
	auto currentLocationUid = currentLocationObject().uid();
	if( currentLocationUid.isNull() )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add( NetworkObject( NetworkObject::Host, tr( "New computer" ),
									  QString(), QString(), QString(),
									  QUuid::createUuid(),
									  currentLocationUid ) );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateComputers();

	ui->computerTableWidget->setCurrentCell( ui->computerTableWidget->rowCount()-1, 0 );
}



void BuiltinDirectoryConfigurationPage::updateComputer()
{
	auto currentComputerIndex = ui->computerTableWidget->currentIndex();
	if( currentComputerIndex.isValid() == false )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.update( currentComputerObject() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateComputers();

	ui->computerTableWidget->setCurrentIndex( currentComputerIndex );
}



void BuiltinDirectoryConfigurationPage::removeComputer()
{
	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.remove( currentComputerObject().uid() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateComputers();
}



void BuiltinDirectoryConfigurationPage::populateLocations()
{
	ui->locationTableWidget->setUpdatesEnabled( false );
	ui->locationTableWidget->clear();

	int rowCount = 0;

	const auto networkObjects = m_configuration.networkObjects();
	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );
		if( networkObject.type() == NetworkObject::Location )
		{
			auto item = new QTableWidgetItem( networkObject.name() );
			item->setData( NetworkObjectModel::UidRole, networkObject.uid() );
			ui->locationTableWidget->setRowCount( ++rowCount );
			ui->locationTableWidget->setItem( rowCount-1, 0, item );
		}
	}

	ui->locationTableWidget->setUpdatesEnabled( true );
}



void BuiltinDirectoryConfigurationPage::populateComputers()
{
	auto parentUid = currentLocationObject().uid();

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



NetworkObject BuiltinDirectoryConfigurationPage::currentLocationObject() const
{
	const auto selectedLocation = ui->locationTableWidget->currentItem();
	if( selectedLocation )
	{
		return NetworkObject( NetworkObject::Location,
							  selectedLocation->text(),
							  QString(),
							  QString(),
							  QString(),
							  selectedLocation->data( NetworkObjectModel::UidRole ).toUuid(),
							  selectedLocation->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}



NetworkObject BuiltinDirectoryConfigurationPage::currentComputerObject() const
{
	const int row = ui->computerTableWidget->currentRow();
	if( row >= 0 )
	{
		auto nameItem = ui->computerTableWidget->item( row, 0 );
		auto hostAddressItem = ui->computerTableWidget->item( row, 1 );
		auto macAddressItem = ui->computerTableWidget->item( row, 2 );

		return NetworkObject( NetworkObject::Host,
							  nameItem->text(),
							  hostAddressItem->text().trimmed(),
							  macAddressItem->text().trimmed(),
							  QString(),
							  nameItem->data( NetworkObjectModel::UidRole ).toUuid(),
							  nameItem->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}
