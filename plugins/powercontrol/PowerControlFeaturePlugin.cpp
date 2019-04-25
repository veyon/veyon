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
#include <QProgressBar>
#include <QProgressDialog>
#include <QUdpSocket>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"
#include "PowerControlFeaturePlugin.h"
#include "PowerDownTimeInputDialog.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


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
	m_powerDownNowFeature( QStringLiteral( "PowerDownNow" ),
								   Feature::Action | Feature::AllComponents,
								   Feature::Uid( "a88039f2-6716-40d8-b4e1-9f5cd48e91ed" ),
								   m_powerDownFeature.uid(),
								   tr( "Power down now" ), {}, {} ),
	m_installUpdatesAndPowerDownFeature( QStringLiteral( "InstallUpdatesAndPowerDown" ),
								   Feature::Action | Feature::AllComponents,
								   Feature::Uid( "09bcb3a1-fc11-4d03-8cf1-efd26be8655b" ),
								   m_powerDownFeature.uid(),
								   tr( "Install updates and power down" ), {}, {} ),
	m_powerDownConfirmedFeature( QStringLiteral( "PowerDownConfirmed" ),
								   Feature::Action | Feature::AllComponents,
								   Feature::Uid( "ea2406be-d5c7-42b8-9f04-53469d3cc34c" ),
								   m_powerDownFeature.uid(),
								   tr( "Power down after user confirmation" ), {}, {} ),
	m_powerDownDelayedFeature( QStringLiteral( "PowerDownDelayed" ),
								   Feature::Action | Feature::AllComponents,
								   Feature::Uid( "352de795-7fc4-4850-bc57-525bcb7033f5" ),
								   m_powerDownFeature.uid(),
								   tr( "Power down after timeout" ), {}, {} ),
	m_features( { m_powerOnFeature, m_rebootFeature, m_powerDownFeature, m_powerDownNowFeature,
				m_installUpdatesAndPowerDownFeature, m_powerDownConfirmedFeature, m_powerDownDelayedFeature } )
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
	else if( feature == m_powerDownDelayedFeature )
	{
		PowerDownTimeInputDialog dialog( master.mainWindow() );

		if( dialog.exec() )
		{
			sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ).
								addArgument( ShutdownTimeout, dialog.seconds() ),
								computerControlInterfaces );
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
	Q_UNUSED(messageContext)

	auto& featureWorkerManager = server.featureWorkerManager();

	if( message.featureUid() == m_powerDownFeature.uid() ||
		message.featureUid() == m_powerDownNowFeature.uid() ||
		message.featureUid() == m_installUpdatesAndPowerDownFeature.uid() )
	{
		const auto installUpdates = message.featureUid() == m_installUpdatesAndPowerDownFeature.uid();
		VeyonCore::platform().coreFunctions().powerDown( installUpdates );
	}
	else if( message.featureUid() == m_powerDownConfirmedFeature.uid() )
	{
		if( VeyonCore::platform().userFunctions().loggedOnUsers().isEmpty() )
		{
			VeyonCore::platform().coreFunctions().powerDown( false );
		}
		else
		{
			featureWorkerManager.startWorker( m_powerDownConfirmedFeature, FeatureWorkerManager::ManagedSystemProcess );
			featureWorkerManager.sendMessage( message );
		}
	}
	else if( message.featureUid() == m_powerDownDelayedFeature.uid() )
	{
		featureWorkerManager.startWorker( m_powerDownDelayedFeature, FeatureWorkerManager::ManagedSystemProcess );
		featureWorkerManager.sendMessage( message );
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
	Q_UNUSED(worker)

	if( message.featureUid() == m_powerDownConfirmedFeature.uid() )
	{
		confirmShutdown();
		return true;
	}
	else if( message.featureUid() == m_powerDownDelayedFeature.uid() )
	{
		displayShutdownTimeout( message.argument( ShutdownTimeout ).toInt() );
		return true;
	}

	return false;
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
	else if( feature == m_powerDownFeature ||
			 feature == m_powerDownNowFeature ||
			 feature == m_installUpdatesAndPowerDownFeature ||
			 feature == m_powerDownConfirmedFeature ||
			 feature == m_powerDownDelayedFeature )
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
		vWarning() << "invalid MAC address" << originalMacAddress;
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



void PowerControlFeaturePlugin::confirmShutdown()
{
	QMessageBox m( QMessageBox::Question, tr( "Confirm power down" ),
				   tr( "The computer was remotely requested to power down. Do you want to power down the computer now?" ),
				   QMessageBox::Yes | QMessageBox::No );
	m.show();
	VeyonCore::platform().coreFunctions().raiseWindow( &m );

	if( m.exec() == QMessageBox::Yes )
	{
		VeyonCore::platform().coreFunctions().powerDown( false );
	}
}



static void updateDialog( QProgressDialog* dialog, int newValue )
{
	dialog->setValue( newValue );

	const auto remainingSeconds = dialog->maximum() - newValue;

	dialog->setLabelText( PowerControlFeaturePlugin::tr(
							  "The computer will be powered down in %1 minutes, %2 seconds.\n\n"
							  "Please save your work and close all programs.").
						 arg( remainingSeconds / 60, 2, 10, QLatin1Char('0') ).
						 arg( remainingSeconds % 60, 2, 10, QLatin1Char('0') ) );

	if( remainingSeconds <= 0 )
	{
		VeyonCore::platform().coreFunctions().powerDown( false );
	}

}

void PowerControlFeaturePlugin::displayShutdownTimeout( int shutdownTimeout )
{
	QProgressDialog dialog;
	dialog.setAutoReset( false );
	dialog.setMinimum( 0 );
	dialog.setMaximum( shutdownTimeout );
	dialog.setCancelButton( nullptr );

	auto progressBar = dialog.findChild<QProgressBar *>();
	if( progressBar )
	{
		progressBar->setTextVisible( false );
	}

	updateDialog( &dialog, 0 );

	dialog.show();
	VeyonCore::platform().coreFunctions().raiseWindow( &dialog );

	QTimer powerdownTimer;
	powerdownTimer.start( 1000 );

	connect( &powerdownTimer, &QTimer::timeout, this, [&dialog]() {
		updateDialog( &dialog, dialog.value()+1 );
	} );

	dialog.exec();
}
