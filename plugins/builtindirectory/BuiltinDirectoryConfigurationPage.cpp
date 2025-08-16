/*
 * BuiltinDirectoryConfigurationPage.cpp - implementation of BuiltinDirectoryConfigurationPage
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

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
	FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	populateLocations();

	ui->locationTableWidget->setCurrentCell( 0, 0 );
}



void BuiltinDirectoryConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void BuiltinDirectoryConfigurationPage::applyConfiguration()
{
}



void BuiltinDirectoryConfigurationPage::addLocation()
{
	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add( NetworkObject( nullptr,
									  NetworkObject::Type::Location, tr( "New location" ),
									  {},
									  QUuid::createUuid() ) );
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
	auto currentRow = ui->locationTableWidget->currentRow();

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.remove( currentLocationObject().uid(), true );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateLocations();

	if( currentRow > 0 )
	{
		ui->locationTableWidget->setCurrentCell( currentRow-1, 0 );
	}
	else if ( ui->locationTableWidget->rowCount() > 0 )
	{
		ui->locationTableWidget->setCurrentCell( currentRow, 0 );
	}
}



void BuiltinDirectoryConfigurationPage::addComputer()
{
	auto currentLocationUid = currentLocationObject().uid();
	if( currentLocationUid.isNull() )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add( NetworkObject( nullptr,
									  NetworkObject::Type::Host, tr( "New computer" ),
									  {},
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

	ui->addComputerButton->setEnabled( false );

	int rowCount = 0;

	const auto networkObjects = m_configuration.networkObjects();
	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject{networkObjectValue.toObject()};
		if( networkObject.type() == NetworkObject::Type::Location )
		{
			auto item = new QTableWidgetItem( networkObject.name() );
			item->setData( NetworkObjectModel::UidRole, networkObject.uid() );
			ui->locationTableWidget->setRowCount( ++rowCount );
			ui->locationTableWidget->setItem( rowCount-1, 0, item );
		}
	}

	ui->locationTableWidget->setUpdatesEnabled( true );

	if( rowCount > 0 )
	{
		ui->addComputerButton->setEnabled( true );
	}
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
		const NetworkObject networkObject{networkObjectValue.toObject()};

		if( networkObject.type() == NetworkObject::Type::Host &&
			networkObject.parentUid() == parentUid )
		{
			auto nameItem = new QTableWidgetItem( networkObject.name() );
			nameItem->setData( NetworkObjectModel::UidRole, networkObject.uid() );
			nameItem->setData( NetworkObjectModel::ParentUidRole, networkObject.parentUid() );

			ui->computerTableWidget->setRowCount( rowCount+1 );
			ui->computerTableWidget->setItem( rowCount, 0, nameItem );
			ui->computerTableWidget->setItem( rowCount, 1, new QTableWidgetItem( networkObject.property( NetworkObject::Property::HostAddress ).toString() ) );
			ui->computerTableWidget->setItem( rowCount, 2, new QTableWidgetItem( networkObject.property( NetworkObject::Property::MacAddress ).toString() ) );
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
		return NetworkObject( nullptr,
							  NetworkObject::Type::Location,
							  selectedLocation->text(),
							  {},
							  selectedLocation->data( NetworkObjectModel::UidRole ).toUuid(),
							  selectedLocation->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}


void BuiltinDirectoryConfigurationPage::importStudents()
{
	const QString filePath = QFileDialog::getOpenFileName(this, tr("Import Student CSV"), QString(), tr("CSV Files (*.csv)"));
	if (filePath.isEmpty())
	{
		return;
	}

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, tr("Error"), tr("Could not open file: %1").arg(file.errorString()));
		return;
	}

	QJsonArray networkObjects = m_configuration.networkObjects();

	QMap<QString, int> hostNameToIndexMap;
	for (int i = 0; i < networkObjects.size(); ++i)
	{
		const QJsonObject obj = networkObjects[i].toObject();
		if (obj.value("type").toInt() == NetworkObject::Type::Host)
		{
			const QString hostAddress = obj.value("properties").toObject().value(NetworkObject::propertyKey(NetworkObject::Property::HostAddress)).toString().trimmed();
			if (!hostAddress.isEmpty())
			{
				hostNameToIndexMap[hostAddress.toLower()] = i;
			}
		}
	}

	QTextStream in(&file);
	int importedCount = 0;
	int lineCount = 0;
	int notFoundCount = 0;

	// Skip header line
	if (!in.atEnd())
	{
		in.readLine();
	}

	while (!in.atEnd())
	{
		lineCount++;
		const QString line = in.readLine();
		const QStringList fields = line.split(',');

		if (fields.size() != 4)
		{
			qWarning() << "Skipping malformed CSV line" << lineCount << ":" << line;
			continue;
		}

		const QString studentName = fields[0].trimmed();
		const QString className = fields[1].trimmed();
		const QString civilId = fields[2].trimmed();
		const QString hostname = fields[3].trimmed();

		if (hostname.isEmpty() || civilId.isEmpty())
		{
			qWarning() << "Skipping CSV line with empty hostname or Civil ID:" << line;
			continue;
		}

		if (hostNameToIndexMap.contains(hostname.toLower()))
		{
			const int index = hostNameToIndexMap.value(hostname.toLower());
			QJsonObject computerObject = networkObjects[index].toObject();

			QJsonObject studentData;
			studentData["name"] = studentName;
			studentData["class"] = className;
			studentData["civilId"] = civilId;

			// Also update computer name to student name for clarity in Veyon Master
			computerObject["name"] = studentName;
			computerObject["studentData"] = studentData;

			networkObjects.replace(index, computerObject);
			importedCount++;
		}
		else
		{
			notFoundCount++;
			qWarning() << "Could not find a computer with hostname:" << hostname;
		}
	}

	file.close();

	m_configuration.setNetworkObjects(networkObjects);

	QMessageBox::information(this, tr("Import Complete"),
		tr("Successfully imported data for %1 students.\n%2 computers were updated.\n%3 hostnames from the CSV were not found in the configuration.")
		.arg(importedCount).arg(importedCount).arg(notFoundCount));

	// Repopulate views to show updated computer names
	populateLocations();
	ui->locationTableWidget->setCurrentCell( 0, 0 );
}



NetworkObject BuiltinDirectoryConfigurationPage::currentComputerObject() const
{
	const int row = ui->computerTableWidget->currentRow();
	if( row >= 0 )
	{
		auto nameItem = ui->computerTableWidget->item( row, 0 );
		auto hostAddressItem = ui->computerTableWidget->item( row, 1 );
		auto macAddressItem = ui->computerTableWidget->item( row, 2 );

		return NetworkObject( nullptr,
							  NetworkObject::Type::Host,
							  nameItem->text(),
							  {
								  { NetworkObject::propertyKey(NetworkObject::Property::HostAddress), hostAddressItem->text().trimmed() },
								  { NetworkObject::propertyKey(NetworkObject::Property::MacAddress), macAddressItem->text().trimmed() }
							  },
							  nameItem->data( NetworkObjectModel::UidRole ).toUuid(),
							  nameItem->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}
