/*
 * BuiltinDirectoryConfigurationPage.cpp - implementation of BuiltinDirectoryConfigurationPage
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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
#include "NetworkDiscoveryDialog.h"
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h>
#include <lm.h>
#else
#include <QProcess>
#endif

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
	objectManager.add(NetworkObject(NetworkObject::Type::Location,
									objectManager.generateUniqueName(tr("New location")),
									{}, {}, {}, QUuid::createUuid()));
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



void BuiltinDirectoryConfigurationPage::moveLocationUp()
{
	const int row = ui->locationTableWidget->currentRow();

	if( row <= 0 )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.moveUp( currentLocationObject().uid() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateLocations();
	ui->locationTableWidget->setCurrentCell( row - 1, 0 );
}



void BuiltinDirectoryConfigurationPage::moveLocationDown()
{
	const int row = ui->locationTableWidget->currentRow();

	if( row < 0 || row >= ui->locationTableWidget->rowCount() - 1 )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.moveDown( currentLocationObject().uid() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateLocations();
	ui->locationTableWidget->setCurrentCell( row + 1, 0 );
}



void BuiltinDirectoryConfigurationPage::addComputer()
{
	auto currentLocationUid = currentLocationObject().uid();
	if( currentLocationUid.isNull() )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add(NetworkObject(NetworkObject::Type::Host,
									objectManager.generateUniqueName(tr("New computer")),
									{}, {}, {},
									QUuid::createUuid(),
									currentLocationUid));
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



void BuiltinDirectoryConfigurationPage::moveComputerUp()
{
	const int row = ui->computerTableWidget->currentRow();

	if( row <= 0 )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.moveUp( currentComputerObject().uid() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateComputers();
	ui->computerTableWidget->setCurrentCell( row - 1, 0 );
}



void BuiltinDirectoryConfigurationPage::moveComputerDown()
{
	const int row = ui->computerTableWidget->currentRow();

	if( row < 0 || row >= ui->computerTableWidget->rowCount() - 1 )
	{
		return;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.moveDown( currentComputerObject().uid() );
	m_configuration.setNetworkObjects( objectManager.objects() );

	populateComputers();
	ui->computerTableWidget->setCurrentCell( row + 1, 0 );
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
		if( networkObject.type() == NetworkObject::Type::Location )
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

		if( networkObject.type() == NetworkObject::Type::Host &&
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
		return NetworkObject( NetworkObject::Type::Location,
							  selectedLocation->text(),
							  {},
							  {},
							  {},
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

		return NetworkObject( NetworkObject::Type::Host,
							  nameItem->text(),
							  hostAddressItem->text().trimmed(),
							  macAddressItem->text().trimmed(),
							  {},
							  nameItem->data( NetworkObjectModel::UidRole ).toUuid(),
							  nameItem->data( NetworkObjectModel::ParentUidRole ).toUuid() );
	}

	return NetworkObject();
}

void BuiltinDirectoryConfigurationPage::scanWorkgroup()
{
	auto currentLocationUid = currentLocationObject().uid();
	if( currentLocationUid.isNull() )
	{
		QMessageBox::warning(this, tr("Warning"), tr("Please select a location first."));
		return;
	}

	QStringList discoveredComputers;

#ifdef Q_OS_WIN
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;
	DWORD resumeHandle = 0;
	SERVER_INFO_100* serverInfo = nullptr;

	NET_API_STATUS status = NetServerEnum(NULL, 100, (LPBYTE*)&serverInfo, MAX_PREFERRED_LENGTH,
										  &entriesRead, &totalEntries, SV_TYPE_WORKSTATION | SV_TYPE_SERVER,
										  NULL, &resumeHandle);

	if( (status == NERR_Success || status == ERROR_MORE_DATA) && serverInfo != nullptr )
	{
		for( DWORD i = 0; i < entriesRead; i++ )
		{
			discoveredComputers.append( QString::fromWCharArray((wchar_t*)serverInfo[i].sv100_name) );
		}
		NetApiBufferFree( serverInfo );
	}
#else
	QProcess process;
	process.start( QStringLiteral("nmblookup"), QStringList() << QStringLiteral("-S") << QStringLiteral("WORKGROUP") );
	if( process.waitForFinished( 10000 ) )
	{
		QByteArray output = process.readAllStandardOutput();
		QString outputStr = QString::fromUtf8( output );
		QStringList lines = outputStr.split( QLatin1Char('\n'), Qt::SkipEmptyParts );
		for( const QString& line : lines )
		{
			if( line.contains(QStringLiteral("<ACTIVE>")) && !line.contains(QStringLiteral("<GROUP>")) )
			{
				QString name = line.section( QLatin1Char('<'), 0, 0 ).trimmed();
				if( !name.isEmpty() && !discoveredComputers.contains(name, Qt::CaseInsensitive) )
				{
					discoveredComputers.append( name );
				}
			}
		}
	}
#endif

	if( discoveredComputers.isEmpty() )
	{
		QMessageBox::information(this, tr("Information"), tr("No computers found in the workgroup."));
		return;
	}

	NetworkDiscoveryDialog dialog( discoveredComputers, this );
	if( dialog.exec() == QDialog::Accepted )
	{
		QStringList selected = dialog.selectedComputers();
		if( selected.isEmpty() )
		{
			return;
		}

		ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

		for( const QString& pcName : selected )
		{
			bool exists = false;
			for( const auto& objValue : objectManager.objects() )
			{
				NetworkObject obj( objValue.toObject() );
				if( obj.type() == NetworkObject::Type::Host &&
					obj.parentUid() == currentLocationUid &&
					obj.name().compare(pcName, Qt::CaseInsensitive) == 0 )
				{
					exists = true;
					break;
				}
			}

			if( !exists )
			{
				objectManager.add( NetworkObject( NetworkObject::Type::Host, pcName, pcName, {}, {}, QUuid::createUuid(), currentLocationUid ) );
			}
		}

		m_configuration.setNetworkObjects( objectManager.objects() );
		populateComputers();
	}
}
