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

#include "AuthenticationCredentials.h"
#include "Computer.h"
#include "CryptoCore.h"
#include "DemoClient.h"
#include "DemoConfigurationPage.h"
#include "DemoFeaturePlugin.h"
#include "DemoServer.h"
#include "EnumHelper.h"
#include "FeatureWorkerManager.h"
#include "Logger.h"
#include "FeatureWorkerManager.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


DemoFeaturePlugin::DemoFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_demoFeature( QStringLiteral( "Demo" ),
				   Feature::Mode | Feature::AllComponents,
				   Feature::Uid( "6f4cd922-b63e-40bf-9451-536065c7cdf9" ),
				   Feature::Uid(),
				   tr( "Demo" ), tr( "Stop demo" ),
				   tr( "Share your screen or allow a user to share his screen with other users." ),
				   QStringLiteral(":/demo/demo.png") ),
	m_fullscreenDemoFeature( QStringLiteral( "FullscreenDemo" ),
							 Feature::Mode | Feature::AllComponents,
							 Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" ),
							 m_demoFeature.uid(),
							 tr( "Share your own screen in fullscreen mode" ), {},
							 tr( "In this mode your screen is being displayed in "
								 "fullscreen mode on all computers while the input "
								 "devices of the users are locked." ),
							 QStringLiteral(":/demo/presentation-fullscreen.png") ),
	m_windowDemoFeature( QStringLiteral( "WindowDemo" ),
						 Feature::Mode | Feature::AllComponents,
						 Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" ),
						 m_demoFeature.uid(),
						 tr( "Share your own screen in a window" ), {},
						 tr( "In this mode your screen being displayed in a "
							 "window on all computers. The users are "
							 "able to switch to other windows as needed." ),
						 QStringLiteral(":/demo/presentation-window.png") ),
	m_demoServerFeature( QStringLiteral( "DemoServer" ),
						 Feature::Session | Feature::Service | Feature::Worker,
						 Feature::Uid( "e4b6e743-1f5b-491d-9364-e091086200f4" ),
						 m_demoFeature.uid(),
						 tr( "Demo server" ), {}, {} ),
	m_features( { m_demoFeature, m_fullscreenDemoFeature, m_windowDemoFeature, m_demoServerFeature } ),
	m_demoAccessToken( CryptoCore::generateChallenge() ),
	m_demoClientHosts(),
	m_configuration( &VeyonCore::config() ),
	m_demoServer( nullptr ),
	m_demoClient( nullptr )
{
}



bool DemoFeaturePlugin::controlFeature( const Feature& feature,
									   Operation operation,
									   const QVariantMap& arguments,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_demoServerFeature )
	{
		return controlDemoServer( operation, arguments, computerControlInterfaces );
	}

	if( feature == m_windowDemoFeature || feature == m_fullscreenDemoFeature )
	{
		return controlDemoClient( feature, operation, arguments, computerControlInterfaces );
	}

	return false;
}



bool DemoFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_windowDemoFeature || feature == m_fullscreenDemoFeature )
	{
		// start demo server
		controlFeature( m_demoServerFeature, Operation::Start, {},
						{ master.localSessionControlInterface().weakPointer() } );

		// start demo clients
		controlFeature( feature, Operation::Start, {}, computerControlInterfaces );

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
									const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_windowDemoFeature || feature == m_fullscreenDemoFeature )
	{
		controlFeature( feature, Operation::Stop, {}, computerControlInterfaces );

		// no demo clients left?
		if( m_demoClientHosts.isEmpty() )
		{
			// then we can stop the server
			controlFeature( m_demoServerFeature, Operation::Stop, {},
							{ master.localSessionControlInterface().weakPointer() } );

			// reset demo access token
			m_demoAccessToken = CryptoCore::generateChallenge();
		}

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
											 ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master);
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

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
						.addArgument( VncServerPassword, VeyonCore::authenticationCredentials().internalVncServerPassword().toByteArray() ) );
		}
		else if( message.command() != StopDemoServer ||
				 server.featureWorkerManager().isWorkerRunning( m_demoServerFeature.uid() ) )
		{
			// forward message to worker
			server.featureWorkerManager().sendMessageToManagedSystemWorker( message );
		}

		return true;
	}
	else if( message.featureUid() == m_fullscreenDemoFeature.uid() ||
			 message.featureUid() == m_windowDemoFeature.uid() )
	{
		// if a demo server is started, it's likely that the demo accidentally was
		// started on master computer as well therefore we deny starting a demo on
		// hosts on which a demo server is running - exception: debug mode
		if( server.featureWorkerManager().isWorkerRunning( m_demoServerFeature.uid() ) &&
			VeyonCore::config().logLevel() < Logger::LogLevel::Debug )
		{
			return false;
		}

		if( server.featureWorkerManager().isWorkerRunning( message.featureUid() ) == false &&
			message.command() == StopDemoClient )
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
			message.argument( DemoServerHost ).toString().isEmpty() )
		{
			// set the peer address as demo server host
			server.featureWorkerManager().sendMessageToManagedSystemWorker(
				FeatureMessage{ message }
					.addArgument( DemoServerHost, socket->peerAddress().toString() ) );
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
				m_demoServer = new DemoServer( message.argument( VncServerPort ).toInt(),
											   message.argument( VncServerPassword ).toByteArray(),
											   message.argument( DemoAccessToken ).toByteArray(),
											   m_configuration,
											   message.argument( DemoServerPort ).toInt(),
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
	else if( message.featureUid() == m_fullscreenDemoFeature.uid() ||
			 message.featureUid() == m_windowDemoFeature.uid() )
	{
		switch( message.command() )
		{
		case StartDemoClient:
			VeyonCore::authenticationCredentials().setToken( message.argument( DemoAccessToken ).toByteArray() );

			if( m_demoClient == nullptr )
			{
				const auto demoServerHost = message.argument( DemoServerHost ).toString();
				const auto demoServerPort = message.argument( DemoServerPort ).toInt();
				const auto isFullscreenDemo = message.featureUid() == m_fullscreenDemoFeature.uid();

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
		const auto demoServerPort = arguments.value( EnumHelper::itemName(Arguments::DemoServerPort).toLower(),
													 VeyonCore::config().demoServerPort() + VeyonCore::sessionId() ).toInt();
		const auto vncServerPort = arguments.value( EnumHelper::itemName(Arguments::VncServerPort).toLower(),
													VeyonCore::config().vncServerPort() + VeyonCore::sessionId() ).toInt();
		const auto demoAccessToken = arguments.value( EnumHelper::itemName(Arguments::DemoAccessToken).toLower(),
													  m_demoAccessToken.toByteArray() ).toByteArray();

		return sendFeatureMessage( FeatureMessage{ m_demoServerFeature.uid(), StartDemoServer }
									   .addArgument( DemoAccessToken, demoAccessToken )
									   .addArgument( VncServerPort, vncServerPort )
									   .addArgument( DemoServerPort, demoServerPort ),
								   computerControlInterfaces );
	}

	if( operation == Operation::Stop )
	{
		return sendFeatureMessage( FeatureMessage{ m_demoServerFeature.uid(), StopDemoServer },
								   computerControlInterfaces );
	}

	return false;
}



bool DemoFeaturePlugin::controlDemoClient( const Feature& feature, Operation operation, const QVariantMap& arguments,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation == Operation::Start )
	{
		const auto demoAccessToken = arguments.value( EnumHelper::itemName(Arguments::DemoAccessToken).toLower(),
													  m_demoAccessToken.toByteArray() ).toByteArray();
		const auto demoServerHost = arguments.value( EnumHelper::itemName(Arguments::DemoServerHost).toLower() ).toString();
		const auto demoServerPort = arguments.value( EnumHelper::itemName(Arguments::DemoServerPort).toLower(),
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

		return sendFeatureMessage( FeatureMessage{ feature.uid(), StartDemoClient }
									   .addArgument( DemoAccessToken, demoAccessToken )
									   .addArgument( DemoServerHost, demoServerHost )
									   .addArgument( DemoServerPort, demoServerPort ),
								   computerControlInterfaces );
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

		return sendFeatureMessage( FeatureMessage{ feature.uid(), StopDemoClient }, computerControlInterfaces );
	}

	return false;
}


IMPLEMENT_CONFIG_PROXY(DemoConfiguration)
