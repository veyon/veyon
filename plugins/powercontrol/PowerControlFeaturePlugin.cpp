/*
 * PowerControlFeaturePlugin.cpp - implementation of PowerControlFeaturePlugin class
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

#include <QMessageBox>
#include <QNetworkInterface>
#include <QUdpSocket>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "PlatformCoreFunctions.h"
#include "PowerControlFeaturePlugin.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"


PowerControlFeaturePlugin::PowerControlFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_powerOnFeature( Feature::Action | Feature::AllComponents,
					  Feature::Uid( "f483c659-b5e7-4dbc-bd91-2c9403e70ebd" ),
					  Feature::Uid(),
					  tr( "Power on" ), QString(),
					  tr( "Click this button to power on all computers. "
						  "This way you do not have to power on each computer by hand." ),
					  QStringLiteral(":/powercontrol/preferences-system-power-management.png") ),
	m_rebootFeature( Feature::Action | Feature::AllComponents,
					 Feature::Uid( "4f7d98f0-395a-4fff-b968-e49b8d0f748c" ),
					 Feature::Uid(),
					 tr( "Reboot" ), QString(),
					 tr( "Click this button to reboot all computers." ),
					 QStringLiteral(":/powercontrol/system-reboot.png") ),
	m_powerDownFeature( Feature::Action | Feature::AllComponents,
						Feature::Uid( "6f5a27a0-0e2f-496e-afcc-7aae62eede10" ),
						Feature::Uid(),
						tr( "Power down" ), QString(),
						tr( "Click this button to power down all computers. "
							"This way you do not have to power down each computer by hand." ),
						QStringLiteral(":/powercontrol/system-shutdown.png") ),
	m_features( { m_powerOnFeature, m_rebootFeature, m_powerDownFeature } )
{
}



const FeatureList &PowerControlFeaturePlugin::featureList() const
{
	return m_features;
}



bool PowerControlFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( m_features.contains( feature ) == false )
	{
		return false;
	}

	if( feature == m_powerOnFeature )
	{
		for( const auto& controlInterface : computerControlInterfaces )
		{
			broadcastWOLPacket( controlInterface->computer().macAddress() );
		}
	}
	else
	{
		if( confirmFeatureExecution( feature, master.mainWindow() ) == false )
		{
			return false;
		}

		sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ), computerControlInterfaces );
	}

	return true;
}



bool PowerControlFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master);
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);

	return false;
}



bool PowerControlFeaturePlugin::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
													  ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master);
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool PowerControlFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message )
{
	Q_UNUSED(server);

	if( message.featureUid() == m_powerDownFeature.uid() )
	{
		VeyonCore::platform().coreFunctions().powerDown();
	}
	else if( message.featureUid() == m_rebootFeature.uid() )
	{
		VeyonCore::platform().coreFunctions().reboot();
	}
	else
	{
		return false;
	}

	return true;
}



bool PowerControlFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);
	Q_UNUSED(message);

	return false;
}



bool PowerControlFeaturePlugin::confirmFeatureExecution( const Feature& feature, QWidget* parent )
{
	if( VeyonCore::config().confirmDangerousActions() == false )
	{
		return true;
	}

	if( feature == m_rebootFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm reboot" ),
									  tr( "Do you really want to reboot the selected computers?" ) ) ==
				QMessageBox::Yes;
	}
	else if( feature == m_powerDownFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm power down" ),
									  tr( "Do you really want to power down the selected computer?" ) ) ==
				QMessageBox::Yes;
	}

	return false;
}



void PowerControlFeaturePlugin::broadcastWOLPacket( QString macAddress )
{
	const int MAC_SIZE = 6;
	unsigned int mac[MAC_SIZE];  // Flawfinder: ignore

	if( macAddress.isEmpty() )
	{
		return;
	}

	const auto originalMacAddress = macAddress;

	// remove all possible delimiters
	macAddress.replace( ':', QString() );
	macAddress.replace( '-', QString() );
	macAddress.replace( '.', QString() );

	if( sscanf( macAddress.toUtf8().constData(),
				"%2x%2x%2x%2x%2x%2x",
				&mac[0],
				&mac[1],
				&mac[2],
				&mac[3],
				&mac[4],
				&mac[5] ) != MAC_SIZE )
	{
		qWarning() << "PowerControlFeaturePlugin::broadcastWOLPacket(): invalid MAC address" << originalMacAddress;
		return;
	}

	QByteArray datagram( MAC_SIZE*17, static_cast<char>( 0xff ) );

	for( int i = 1; i < 17; ++i )
	{
		for(int j = 0; j < MAC_SIZE; ++j )
		{
			datagram[i*MAC_SIZE+j] = static_cast<char>( mac[j] );
		}
	}

	QUdpSocket udpSocket;

	udpSocket.writeDatagram( datagram, QHostAddress::Broadcast, 9 );

	const auto networkInterfaces = QNetworkInterface::allInterfaces();
	for( const auto& networkInterface : networkInterfaces )
	{
		const auto addressEntries = networkInterface.addressEntries();
		for( const auto& addressEntry : addressEntries )
		{
			if( addressEntry.broadcast().isNull() == false )
			{
				udpSocket.writeDatagram( datagram, addressEntry.broadcast(), 9 );
			}
		}
	}
}
