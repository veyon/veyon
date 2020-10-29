/*
 * DemoFeaturePlugin.cpp - implementation of DemoFeaturePlugin class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QMessageBox>

#include "AuthenticationCredentials.h"
#include "Computer.h"
#include "DemoClient.h"
#include "DemoConfigurationPage.h"
#include "DemoFeaturePlugin.h"
#include "DemoServer.h"
#include "FeatureWorkerManager.h"
#include "HostAddress.h"
#include "Logger.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


DemoFeaturePlugin::DemoFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	DemoAuthentication( uid() ),
	m_demoFeature( QStringLiteral( "Demo" ),
				   Feature::Mode | Feature::AllComponents,
				   Feature::Uid( "6f4cd922-b63e-40bf-9451-536065c7cdf9" ),
				   Feature::Uid(),
				   tr( "Demo" ), tr( "Stop demo" ),
				   tr( "Share your screen or allow a user to share his screen with other users." ),
				   QStringLiteral(":/demo/demo.png") ),
	m_demoClientFullScreenFeature( QStringLiteral( "FullScreenDemo" ),
								   Feature::Internal | Feature::AllComponents,
								   Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" ),
								   {}, tr("Full screen demo"), {}, {} ),
	m_demoClientWindowFeature( QStringLiteral( "WindowDemo" ),
							   Feature::Internal | Feature::AllComponents,
							   Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" ),
							   {}, tr("Window demo"), {}, {} ),
	m_shareOwnScreenFullScreenFeature( QStringLiteral( "ShareOwnScreenFullScreen" ),
									   Feature::Mode | Feature::AllComponents,
									   Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" ),
									   m_demoFeature.uid(),
									   tr( "Share your own screen in fullscreen mode" ), {},
									   tr( "In this mode your screen is being displayed in "
										   "full screen mode on all computers while the input "
										   "devices of the users are locked." ),
									   QStringLiteral(":/demo/presentation-fullscreen.png") ),
	m_shareOwnScreenWindowFeature( QStringLiteral( "ShareOwnScreenWindow" ),
								   Feature::Mode | Feature::AllComponents,
								   Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" ),
								   m_demoFeature.uid(),
								   tr( "Share your own screen in a window" ), {},
								   tr( "In this mode your screen being displayed in a "
									   "window on all computers. The users are "
									   "able to switch to other windows as needed." ),
								   QStringLiteral(":/demo/presentation-window.png") ),
	m_shareUserScreenFullScreenFeature( QStringLiteral( "ShareUserScreenFullScreen" ),
										Feature::Mode | Feature::AllComponents,
										Feature::Uid( "b4e542e2-1deb-48ac-910a-bbf8ac9a0bde" ),
										m_demoFeature.uid(),
										tr( "Share selected user's screen in fullscreen mode" ), {},
										tr( "In this mode the screen of the selected user is being displayed "
											"in full screen mode on all computers while the input "
											"devices of the users are locked." ),
										QStringLiteral(":/demo/presentation-fullscreen.png") ),
	m_shareUserScreenWindowFeature( QStringLiteral( "ShareUserScreenWindow" ),
									Feature::Mode | Feature::AllComponents,
									Feature::Uid( "ebfc5ec4-f725-4bfc-a93a-c6d4864c6806" ),
									m_demoFeature.uid(),
									tr( "Share selected user's screen in a window" ), {},
									tr( "In this mode the screen of the selected user being displayed "
										"in a window on all computers. The users are "
										"able to switch to other windows as needed." ),
									QStringLiteral(":/demo/presentation-window.png") ),
	m_demoServerFeature( QStringLiteral( "DemoServer" ),
						 Feature::Session | Feature::Service | Feature::Worker,
						 Feature::Uid( "e4b6e743-1f5b-491d-9364-e091086200f4" ),
						 m_demoFeature.uid(),
						 {}, {}, {} ),
	m_features( {
		m_demoFeature, m_demoServerFeature,
		m_demoClientFullScreenFeature, m_demoClientWindowFeature,
		m_shareOwnScreenFullScreenFeature, m_shareOwnScreenWindowFeature,
		m_shareUserScreenFullScreenFeature, m_shareUserScreenWindowFeature
	} ),
	m_configuration( &VeyonCore::config() )
{
}



bool DemoFeaturePlugin::controlFeature( Feature::Uid featureUid,
									   Operation operation,
									   const QVariantMap& arguments,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( featureUid == m_demoServerFeature.uid() )
	{
		return controlDemoServer( operation, arguments, computerControlInterfaces );
	}

	if( featureUid == m_demoClientFullScreenFeature.uid() || featureUid == m_demoClientWindowFeature.uid() )
	{
		return controlDemoClient( featureUid, operation, arguments, computerControlInterfaces );
	}

	return false;
}



bool DemoFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_shareOwnScreenWindowFeature || feature == m_shareOwnScreenFullScreenFeature )
	{
		// start demo server
		controlFeature( m_demoServerFeature.uid(), Operation::Start, {},
						{ master.localSessionControlInterface().weakPointer() } );

		// start demo clients
		controlFeature( feature == m_shareOwnScreenFullScreenFeature ? m_demoClientFullScreenFeature.uid()
																	 : m_demoClientWindowFeature.uid(),
						Operation::Start, {}, computerControlInterfaces );

		return true;
	}

	if( feature == m_shareUserScreenWindowFeature || feature == m_shareUserScreenFullScreenFeature )
	{
		if( master.selectedComputerControlInterfaces().size() < 1 )
		{
			QMessageBox::critical( master.mainWindow(), feature.name(),
								   tr( "Please select a user screen to share.") );
			return true;
		}

		if( master.selectedComputerControlInterfaces().size() > 1 )
		{
			QMessageBox::critical( master.mainWindow(), feature.name(),
								   tr( "Please select only one user screen to share.") );
			return true;
		}

		auto vncServerPort = VeyonCore::config().vncServerPort();
		auto demoServerPort = VeyonCore::config().demoServerPort();

		const auto demoServerInterface = master.selectedComputerControlInterfaces().first();
		const auto demoServerHost = demoServerInterface->computer().hostAddress();
		const auto primaryServerPort = HostAddress::parsePortNumber( demoServerHost );

		if( primaryServerPort > 0 )
		{
			const auto sessionId = primaryServerPort - VeyonCore::config().veyonServerPort();
			vncServerPort += sessionId;
			demoServerPort += sessionId;
		}

		// start demo server
		controlFeature( m_demoServerFeature.uid(), Operation::Start,
						{
							{ a2s(Argument::VncServerPort), vncServerPort },
							{ a2s(Argument::DemoServerPort), demoServerPort },
						},
						master.selectedComputerControlInterfaces() );

		// start demo clients
		auto userDemoControlInterfaces = computerControlInterfaces;
		userDemoControlInterfaces.removeAll( demoServerInterface );

		const QVariantMap demoClientArgs{
			{ a2s(Argument::DemoServerHost), HostAddress::parseHost(demoServerHost) },
			{ a2s(Argument::DemoServerPort), demoServerPort },
		};

		controlFeature( feature == m_shareUserScreenFullScreenFeature ? m_demoClientFullScreenFeature.uid()
																	  : m_demoClientWindowFeature.uid(),
						Operation::Start, demoClientArgs, userDemoControlInterfaces );

		controlFeature( m_demoClientWindowFeature.uid(), Operation::Start, demoClientArgs,
						{ master.localSessionControlInterface().weakPointer() } );

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
									const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_demoFeature ||
		feature == m_shareOwnScreenWindowFeature || feature == m_shareOwnScreenFullScreenFeature ||
		feature == m_shareUserScreenWindowFeature || feature == m_shareUserScreenFullScreenFeature )
	{
		controlFeature( feature.uid(), Operation::Stop, {}, computerControlInterfaces );

		controlFeature( m_demoClientWindowFeature.uid(), Operation::Stop, {},
						{ master.localSessionControlInterface().weakPointer() } );

		// no demo clients left?
		if( m_demoClientHosts.isEmpty() )
		{
			// then we can stop the server
			controlFeature( m_demoServerFeature.uid(), Operation::Stop, {},
							{ master.localSessionControlInterface().weakPointer() } );

			controlFeature( m_demoServerFeature.uid(), Operation::Stop, {}, computerControlInterfaces );

			// reset demo access token
			initializeCredentials();
		}

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
											 const MessageContext& messageContext,
											 const FeatureMessage& message )
{
	if( message.featureUid() == m_demoServerFeature.uid() )
	{
		if( message.command() == StartDemoServer )
		{
			// add VNC server password to message
			server.featureWorkerManager().
				sendMessageToManagedSystemWorker(
					FeatureMessage{ message }
						.addArgument( Argument::VncServerPassword, VeyonCore::authenticationCredentials().internalVncServerPassword().toByteArray() ) );
		}
		else if( message.command() != StopDemoServer ||
				 server.featureWorkerManager().isWorkerRunning( m_demoServerFeature.uid() ) )
		{
			// forward message to worker
			server.featureWorkerManager().sendMessageToManagedSystemWorker( message );
		}

		return true;
	}

	if( message.featureUid() == m_demoClientFullScreenFeature.uid() ||
		message.featureUid() == m_demoClientWindowFeature.uid() )
	{
		// if a demo server is started, it's likely that the demo accidentally was
		// started on master computer as well therefore we deny starting a demo on
		// hosts on which a demo server is running - exception: debug mode
		if( message.featureUid() == m_demoClientFullScreenFeature.uid() &&
			server.featureWorkerManager().isWorkerRunning( m_demoServerFeature.uid() ) &&
			VeyonCore::config().logLevel() < Logger::LogLevel::Debug )
		{
			return false;
		}

		if( message.command() == StopDemoClient &&
			server.featureWorkerManager().isWorkerRunning( message.featureUid() ) == false )
		{
			return true;
		}

		auto socket = qobject_cast<QTcpSocket *>( messageContext.ioDevice() );
		if( socket == nullptr )
		{
			vCritical() << "invalid socket";
			return false;
		}

		if( message.command() == StartDemoClient &&
			message.argument( Argument::DemoServerHost ).toString().isEmpty() )
		{
			// set the peer address as demo server host
			server.featureWorkerManager().sendMessageToManagedSystemWorker(
				FeatureMessage{ message }
					.addArgument( Argument::DemoServerHost, socket->peerAddress().toString() ) );
		}
		else
		{
			// forward message to worker
			server.featureWorkerManager().sendMessageToManagedSystemWorker( message );
		}

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( message.featureUid() == m_demoServerFeature.uid() )
	{
		switch( message.command() )
		{
		case StartDemoServer:
			if( m_demoServer == nullptr )
			{
				setAccessToken( message.argument( Argument::DemoAccessToken ).toByteArray() );

				m_demoServer = new DemoServer( message.argument( Argument::VncServerPort ).toInt(),
											   message.argument( Argument::VncServerPassword ).toByteArray(),
											   *this,
											   m_configuration,
											   message.argument( Argument::DemoServerPort ).toInt(),
											   this );
			}
			return true;

		case StopDemoServer:
			delete m_demoServer;
			m_demoServer = nullptr;

			QCoreApplication::quit();

			return true;

		default:
			break;
		}
	}
	else if( message.featureUid() == m_demoClientFullScreenFeature.uid() ||
			 message.featureUid() == m_demoClientWindowFeature.uid() )
	{
		switch( message.command() )
		{
		case StartDemoClient:
			setAccessToken( message.argument( Argument::DemoAccessToken ).toByteArray() );

			if( m_demoClient == nullptr )
			{
				const auto demoServerHost = message.argument( Argument::DemoServerHost ).toString();
				const auto demoServerPort = message.argument( Argument::DemoServerPort ).toInt();
				const auto isFullscreenDemo = message.featureUid() == m_demoClientFullScreenFeature.uid();

				vDebug() << "connecting with master" << demoServerHost;
				m_demoClient = new DemoClient( demoServerHost, demoServerPort, isFullscreenDemo );
			}
			return true;

		case StopDemoClient:
			delete m_demoClient;
			m_demoClient = nullptr;

			QCoreApplication::quit();

			return true;

		default:
			break;
		}
	}

	return false;
}



ConfigurationPage* DemoFeaturePlugin::createConfigurationPage()
{
	return new DemoConfigurationPage( m_configuration );
}



bool DemoFeaturePlugin::controlDemoServer( Operation operation, const QVariantMap& arguments,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation == Operation::Start )
	{
		const auto demoServerPort = arguments.value( a2s(Argument::DemoServerPort),
													 VeyonCore::config().demoServerPort() + VeyonCore::sessionId() ).toInt();
		const auto vncServerPort = arguments.value( a2s(Argument::VncServerPort),
													VeyonCore::config().vncServerPort() + VeyonCore::sessionId() ).toInt();
		const auto demoAccessToken = arguments.value( a2s(Argument::DemoAccessToken),
													  accessToken().toByteArray() ).toByteArray();

		sendFeatureMessage( FeatureMessage{ m_demoServerFeature.uid(), StartDemoServer }
								.addArgument( Argument::DemoAccessToken, demoAccessToken )
								.addArgument( Argument::VncServerPort, vncServerPort )
								.addArgument( Argument::DemoServerPort, demoServerPort ),
							computerControlInterfaces );

		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ m_demoServerFeature.uid(), StopDemoServer },
							computerControlInterfaces );

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::controlDemoClient( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation == Operation::Start )
	{
		const auto demoAccessToken = arguments.value( a2s(Argument::DemoAccessToken),
													  accessToken().toByteArray() ).toByteArray();
		const auto demoServerHost = arguments.value( a2s(Argument::DemoServerHost) ).toString();
		const auto demoServerPort = arguments.value( a2s(Argument::DemoServerPort),
													 VeyonCore::config().demoServerPort() + VeyonCore::sessionId() ).toInt();

		const auto disableUpdates = m_configuration.slowDownThumbnailUpdates();

		for( const auto& computerControlInterface : computerControlInterfaces )
		{
			m_demoClientHosts += computerControlInterface->computer().hostAddress();
			if( disableUpdates )
			{
				computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Disabled );
			}
		}

		sendFeatureMessage( FeatureMessage{ featureUid, StartDemoClient }
								.addArgument( Argument::DemoAccessToken, demoAccessToken )
								.addArgument( Argument::DemoServerHost, demoServerHost )
								.addArgument( Argument::DemoServerPort, demoServerPort ),
							computerControlInterfaces );

		return true;
	}

	if( operation == Operation::Stop )
	{
		const auto enableUpdates = m_configuration.slowDownThumbnailUpdates();

		for( const auto& computerControlInterface : computerControlInterfaces )
		{
			m_demoClientHosts.removeAll( computerControlInterface->computer().hostAddress() );
			if( enableUpdates )
			{
				computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Monitoring );
			}
		}

		sendFeatureMessage( FeatureMessage{ featureUid, StopDemoClient }, computerControlInterfaces );

		return true;
	}

	return false;
}


IMPLEMENT_CONFIG_PROXY(DemoConfiguration)
