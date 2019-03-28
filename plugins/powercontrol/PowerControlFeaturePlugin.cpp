/*
 * PowerControlFeaturePlugin.cpp - implementation of PowerControlFeaturePlugin class
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
	m_commands( {
{ QStringLiteral("on"), tr( "Power on a computer via Wake-on-LAN (WOL)" ) },
				} ),
	m_powerOnFeature( QStringLiteral( "PowerOn" ),
					  Feature::Action | Feature::AllComponents,
					  Feature::Uid( "f483c659-b5e7-4dbc-bd91-2c9403e70ebd" ),
					  Feature::Uid(),
					  tr( "Power on" ), QString(),
					  tr( "Click this button to power on all computers. "
						  "This way you do not have to power on each computer by hand." ),
					  QStringLiteral(":/powercontrol/preferences-system-power-management.png") ),
	m_rebootFeature( QStringLiteral( "Reboot" ),
					 Feature::Action | Feature::AllComponents,
					 Feature::Uid( "4f7d98f0-395a-4fff-b968-e49b8d0f748c" ),
					 Feature::Uid(),
					 tr( "Reboot" ), QString(),
					 tr( "Click this button to reboot all computers." ),
					 QStringLiteral(":/powercontrol/system-reboot.png") ),
	m_powerDownFeature( QStringLiteral( "PowerDown" ),
						Feature::Action | Feature::AllComponents,
						Feature::Uid( "6f5a27a0-0e2f-496e-afcc-7aae62eede10" ),
						Feature::Uid(),
						tr( "Power down" ), QString(),
						tr( "Click this button to power down all computers. "
							"This way you do not have to power down each computer by hand." ),
						QStringLiteral(":/powercontrol/system-shutdown.png") ),
	m_features( { m_powerOnFeature, m_rebootFeature, m_powerDownFeature } )
{
}



QStringList PowerControlFeaturePlugin::commands() const
{
	return m_commands.keys();
}



QString PowerControlFeaturePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
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



bool PowerControlFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													  const MessageContext& messageContext,
													  const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)

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



CommandLinePluginInterface::RunResult PowerControlFeaturePlugin::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );

	const QMap<QString, QStringList> commands = {
		{ QStringLiteral("on"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("MAC ADDRESS") ),
						 tr( "This command broadcasts a Wake-on-LAN (WOL) packet to the network in order to power on the computer with the given MAC address." ) } ) },
	};

	if( commands.contains( command ) )
	{
		const auto& helpString = commands[command];
		print( QStringLiteral("\n%1 %2 %3\n\n%4\n\n").arg( commandLineModuleName(), command, helpString[0], helpString[1] ) );

		return NoResult;
	}

	print( tr("Please specify the command to display help for!") );

	return Unknown;
}



CommandLinePluginInterface::RunResult PowerControlFeaturePlugin::handle_on( const QStringList& arguments )
{
	if( arguments.size() < 1 )
	{
		return NotEnoughArguments;
	}

	return broadcastWOLPacket( arguments.first() ) ? Successful : Failed;
}



bool PowerControlFeaturePlugin::confirmFeatureExecution( const Feature& feature, QWidget* parent )
{
	if( VeyonCore::config().confirmUnsafeActions() == false )
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



bool PowerControlFeaturePlugin::broadcastWOLPacket( QString macAddress )
{
	const int MAC_SIZE = 6;
	unsigned int mac[MAC_SIZE];  // Flawfinder: ignore

	if( macAddress.isEmpty() )
	{
		return false;
	}

	const auto originalMacAddress = macAddress;

	// remove all possible delimiters
	macAddress.replace( QLatin1Char(':'), QString() );
	macAddress.replace( QLatin1Char('-'), QString() );
	macAddress.replace( QLatin1Char('.'), QString() );

	if( sscanf( macAddress.toUtf8().constData(),
				"%2x%2x%2x%2x%2x%2x",
				&mac[0],
				&mac[1],
				&mac[2],
				&mac[3],
				&mac[4],
				&mac[5] ) != MAC_SIZE )
	{
		CommandLineIO::error( tr( "Invalid MAC address specified!" ) );
		qWarning() << "PowerControlFeaturePlugin::broadcastWOLPacket(): invalid MAC address" << originalMacAddress;
		return false;
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

	bool success = ( udpSocket.writeDatagram( datagram, QHostAddress::Broadcast, 9 ) == datagram.size() );

	const auto networkInterfaces = QNetworkInterface::allInterfaces();
	for( const auto& networkInterface : networkInterfaces )
	{
		const auto addressEntries = networkInterface.addressEntries();
		for( const auto& addressEntry : addressEntries )
		{
			if( addressEntry.broadcast().isNull() == false )
			{
				success &= ( udpSocket.writeDatagram( datagram, addressEntry.broadcast(), 9 ) == datagram.size() );
			}
		}
	}

	return success;
}
