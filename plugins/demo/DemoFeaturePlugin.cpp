/*
 * DemoFeaturePlugin.cpp - implementation of DemoFeaturePlugin class
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

#include <QCoreApplication>

#include "AuthenticationCredentials.h"
#include "Computer.h"
#include "CryptoCore.h"
#include "DemoClient.h"
#include "DemoConfigurationPage.h"
#include "DemoFeaturePlugin.h"
#include "DemoServer.h"
#include "FeatureWorkerManager.h"
#include "VeyonConfiguration.h"
#include "Logger.h"


DemoFeaturePlugin::DemoFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_fullscreenDemoFeature( Feature::Mode | Feature::AllComponents,
							 Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" ),
							 tr( "Fullscreen demo" ), tr( "Stop demo" ),
							 tr( "In this mode your screen is being displayed in "
								 "fullscreen mode on all computers while input "
								 "devices of the users are locked." ),
							 QStringLiteral(":/demo/presentation-fullscreen.png") ),
	m_windowDemoFeature( Feature::Mode | Feature::AllComponents,
						 Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" ),
						 tr( "Window demo" ), tr( "Stop demo" ),
						 tr( "In this mode your screen being displayed in a "
							 "window on all computers. The users are "
							 "able to switch to other windows as needed." ),
						 QStringLiteral(":/demo/presentation-window.png") ),
	m_demoServerFeature( Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
						 Feature::Uid( "e4b6e743-1f5b-491d-9364-e091086200f4" ),
						 tr( "Demo server" ), QString(), QString() ),
	m_features( { m_fullscreenDemoFeature, m_windowDemoFeature, m_demoServerFeature } ),
	m_demoAccessToken( CryptoCore::generateChallenge().toBase64() ),
	m_demoClientHosts(),
	m_demoServer( nullptr ),
	m_demoClient( nullptr )
{
}



DemoFeaturePlugin::~DemoFeaturePlugin()
{
}



bool DemoFeaturePlugin::startMasterFeature( const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces,
											ComputerControlInterface& localServiceInterface,
											QWidget* parent )
{
	Q_UNUSED(parent);

	if( feature == m_windowDemoFeature || feature == m_fullscreenDemoFeature )
	{
		localServiceInterface.sendFeatureMessage( FeatureMessage( m_demoServerFeature.uid(), StartDemoServer ).
												  addArgument( DemoAccessToken, m_demoAccessToken ) );

		for( auto computerControlInterface : computerControlInterfaces )
		{
			m_demoClientHosts += computerControlInterface->computer().hostAddress();
		}

		qDebug() << "DemoFeaturePlugin::startMasterFeature(): clients:" << m_demoClientHosts;

		return sendFeatureMessage( FeatureMessage( feature.uid(), StartDemoClient ).addArgument( DemoAccessToken, m_demoAccessToken ),
								   computerControlInterfaces );
	}

	return false;
}



bool DemoFeaturePlugin::stopMasterFeature( const Feature& feature,
										   const ComputerControlInterfaceList& computerControlInterfaces,
										   ComputerControlInterface& localComputerControlInterface,
										   QWidget* parent )
{
	Q_UNUSED(parent);

	if( feature == m_windowDemoFeature || feature == m_fullscreenDemoFeature )
	{
		sendFeatureMessage( FeatureMessage( feature.uid(), StopDemoClient ), computerControlInterfaces );

		for( auto computerControlInterface : computerControlInterfaces )
		{
			m_demoClientHosts.removeAll( computerControlInterface->computer().hostAddress() );
		}

		qDebug() << "DemoFeaturePlugin::stopMasterFeature(): clients:" << m_demoClientHosts;

		// no demo clients left?
		if( m_demoClientHosts.isEmpty() )
		{
			// then we can stop the server
			localComputerControlInterface.sendFeatureMessage( FeatureMessage( m_demoServerFeature.uid(), StopDemoServer ) );
		}

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
													ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool DemoFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
													 FeatureWorkerManager& featureWorkerManager )
{
	if( message.featureUid() == m_demoServerFeature.uid() )
	{
		if( featureWorkerManager.isWorkerRunning( m_demoServerFeature ) == false )
		{
			featureWorkerManager.startWorker( m_demoServerFeature );
		}

		if( message.command() == StartDemoServer )
		{
			// add VNC server password to message
			featureWorkerManager.
					sendMessage( FeatureMessage( m_demoServerFeature.uid(), StartDemoServer ).
								 addArgument( VncServerPort, VeyonCore::config().vncServerPort() ).
								 addArgument( VncServerPassword, VeyonCore::authenticationCredentials().internalVncServerPassword() ).
								 addArgument( DemoAccessToken, message.argument( DemoAccessToken ) ) );
		}
		else
		{
			// forward message to worker
			featureWorkerManager.sendMessage( message );
		}

		return true;
	}
	else if( message.featureUid() == m_fullscreenDemoFeature.uid() ||
			 message.featureUid() == m_windowDemoFeature.uid() )
	{
		// if a demo server is started, it's likely that the demo accidentally was
		// started on master computer as well therefore we deny starting a demo on
		// hosts on which a demo server is running - exception: debug mode
		if( featureWorkerManager.isWorkerRunning( m_demoServerFeature ) &&
				VeyonCore::config().logLevel() < Logger::LogLevelDebug )
		{
			return false;
		}

		if( featureWorkerManager.isWorkerRunning( message.featureUid() ) == false &&
				message.command() != StopDemoClient )
		{
			featureWorkerManager.startWorker( message.featureUid() );
		}

		QTcpSocket* socket = dynamic_cast<QTcpSocket *>( message.ioDevice() );
		if( socket == nullptr )
		{
			qCritical( "DemoFeaturePlugin::handleServiceFeatureMessage(): socket is NULL!" );
			return false;
		}

		if( message.command() == StartDemoClient )
		{
			// construct a new message as we have to append the peer address as demo server host
			FeatureMessage startDemoClientMessage( message.featureUid(), message.command() );
			startDemoClientMessage.addArgument( DemoAccessToken, message.argument( DemoAccessToken ) );
			startDemoClientMessage.addArgument( DemoServerHost, socket->peerAddress().toString() );
			featureWorkerManager.sendMessage( startDemoClientMessage );
		}
		else
		{
			// forward message to worker
			featureWorkerManager.sendMessage( message );
		}

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	if( message.featureUid() == m_demoServerFeature.uid() )
	{
		switch( message.command() )
		{
		case StartDemoServer:
			if( m_demoServer == nullptr )
			{
				m_demoServer = new DemoServer( message.argument( VncServerPort ).toInt(),
											   message.argument( VncServerPassword ).toString(),
											   message.argument( DemoAccessToken ).toString(),
											   m_configuration,
											   this );
			}
			return true;

		case StopDemoServer:
			delete m_demoServer;
			m_demoServer = nullptr;
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
			VeyonCore::authenticationCredentials().setToken( message.argument( DemoAccessToken ).toString() );

			if( m_demoClient == nullptr )
			{
				const auto demoServerHost = message.argument( DemoServerHost ).toString();
				const auto isFullscreenDemo = message.featureUid() == m_fullscreenDemoFeature.uid();

				qDebug() << "DemoClient: connecting with master" << demoServerHost;
				m_demoClient = new DemoClient( demoServerHost, isFullscreenDemo );
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
