/*
 * DemoFeaturePlugin.cpp - implementation of DemoFeaturePlugin class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
#include <QScreen>

#include "AuthenticationCredentials.h"
#include "Computer.h"
#include "CryptoCore.h"
#include "DemoClient.h"
#include "DemoConfigurationPage.h"
#include "DemoFeaturePlugin.h"
#include "DemoServer.h"
#include "FeatureWorkerManager.h"
#include "HostAddress.h"
#include "Logger.h"
#include "PlatformSessionFunctions.h"
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
	m_demoClientFullScreenFeature( QStringLiteral( "FullScreenDemo" ),
								   Feature::Meta | Feature::AllComponents,
								   Feature::Uid( "7b6231bd-eb89-45d3-af32-f70663b2f878" ),
								   {}, tr("Full screen demo"), {}, {} ),
	m_demoClientWindowFeature( QStringLiteral( "WindowDemo" ),
							   Feature::Meta | Feature::AllComponents,
							   Feature::Uid( "ae45c3db-dc2e-4204-ae8b-374cdab8c62c" ),
							   {}, tr("Window demo"), {}, {} ),
	m_shareOwnScreenFullScreenFeature( QStringLiteral( "ShareOwnScreenFullScreen" ),
									   Feature::Mode | Feature::AllComponents,
									   Feature::Uid( "07b375e1-8ab6-4b48-bcb7-75fb3d56035b" ),
									   m_demoFeature.uid(),
									   tr( "Share your own screen in fullscreen mode" ), {},
									   tr( "In this mode your screen is being displayed in "
										   "full screen mode on all computers while the input "
										   "devices of the users are locked." ),
									   QStringLiteral(":/demo/presentation-fullscreen.png") ),
	m_shareOwnScreenWindowFeature( QStringLiteral( "ShareOwnScreenWindow" ),
								   Feature::Mode | Feature::AllComponents,
								   Feature::Uid( "68c55fb9-127e-4c9f-9c90-28b998bf1a47" ),
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
	m_staticFeatures( {
		m_demoFeature, m_demoServerFeature,
		m_demoClientFullScreenFeature, m_demoClientWindowFeature,
		m_shareOwnScreenFullScreenFeature, m_shareOwnScreenWindowFeature,
		m_shareUserScreenFullScreenFeature, m_shareUserScreenWindowFeature
	} ),
	m_demoAccessToken( CryptoCore::generateChallenge() ),
	m_configuration( &VeyonCore::config() ),
	m_demoServer( nullptr ),
	m_demoClient( nullptr )
{
	connect( qGuiApp, &QGuiApplication::screenAdded, this, &DemoFeaturePlugin::addScreen );
	connect( qGuiApp, &QGuiApplication::screenRemoved, this, &DemoFeaturePlugin::removeScreen );
	connect( &m_demoServerControlTimer, &QTimer::timeout, this, &DemoFeaturePlugin::controlDemoServer );

	updateFeatures();
}



Feature::Uid DemoFeaturePlugin::metaFeature( Feature::Uid featureUid ) const
{
	if( featureUid == m_shareOwnScreenFullScreenFeature.uid() ||
		featureUid == m_shareUserScreenFullScreenFeature.uid() )
	{
		return m_demoClientFullScreenFeature.uid();
	}
	else if( featureUid == m_shareOwnScreenWindowFeature.uid() ||
			 featureUid == m_shareUserScreenWindowFeature.uid() )
	{
		return m_demoClientWindowFeature.uid();
	}

	return FeatureProviderInterface::metaFeature( featureUid );
}



bool DemoFeaturePlugin::controlFeature( Feature::Uid featureUid,
									   Operation operation,
									   const QVariantMap& arguments,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( featureUid == m_demoServerFeature.uid() )
	{
		m_demoServerArguments = arguments;

		if( operation == Operation::Start )
		{
			m_demoServerControlTimer.start( DemoServerControlInterval );
			m_demoServerControlInterfaces = computerControlInterfaces;
		}
		else
		{
			m_demoServerControlTimer.stop();
		}

		return controlDemoServer();
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
		// start demo clients
		controlFeature( feature == m_shareOwnScreenFullScreenFeature ? m_demoClientFullScreenFeature.uid()
																	 : m_demoClientWindowFeature.uid(),
						Operation::Start, {}, computerControlInterfaces );

		// start demo server
		controlFeature( m_demoServerFeature.uid(), Operation::Start, {},
						{ master.localSessionControlInterface().weakPointer() } );

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

		auto vncServerPortOffset = 0;
		auto demoServerPort = VeyonCore::config().demoServerPort();

		const auto demoServerInterface = master.selectedComputerControlInterfaces().first();
		const auto demoServerHost = demoServerInterface->computer().hostAddress();
		const auto primaryServerPort = HostAddress::parsePortNumber( demoServerHost );

		if( primaryServerPort > 0 )
		{
			const auto sessionId = primaryServerPort - VeyonCore::config().veyonServerPort();
			vncServerPortOffset = sessionId;
			demoServerPort += sessionId;
		}

		// start demo clients
		auto userDemoControlInterfaces = computerControlInterfaces;
		userDemoControlInterfaces.removeAll( demoServerInterface );

		const QVariantMap demoClientArgs{
			{ argToString(Argument::DemoServerHost), HostAddress::parseHost(demoServerHost) },
			{ argToString(Argument::DemoServerPort), demoServerPort },
		};

		controlFeature( feature == m_shareUserScreenFullScreenFeature ? m_demoClientFullScreenFeature.uid()
																	  : m_demoClientWindowFeature.uid(),
						Operation::Start, demoClientArgs, userDemoControlInterfaces );

		controlFeature( m_demoClientWindowFeature.uid(), Operation::Start, demoClientArgs,
						{ master.localSessionControlInterface().weakPointer() } );

		// start demo server
		controlFeature( m_demoServerFeature.uid(), Operation::Start,
						{
							{ argToString(Argument::VncServerPortOffset), vncServerPortOffset },
							{ argToString(Argument::DemoServerPort), demoServerPort },
							},
						master.selectedComputerControlInterfaces() );

		return true;
	}

	const auto screenIndex = m_screenSelectionFeatures.indexOf( feature );
	if( screenIndex >= 0 )
	{
		m_screenSelection = screenIndex;

		updateFeatures();
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
		controlFeature( m_demoClientFullScreenFeature.uid(), Operation::Stop, {}, computerControlInterfaces );
		controlFeature( m_demoClientWindowFeature.uid(), Operation::Stop, {}, computerControlInterfaces );

		controlFeature( m_demoClientWindowFeature.uid(), Operation::Stop, {},
						{ master.localSessionControlInterface().weakPointer() } );

		controlDemoServer();

		// no demo clients left?
		if( m_demoServerClients.isEmpty() )
		{
			// then we can stop the server
			m_demoServerControlTimer.stop();

			// reset demo access token
			m_demoAccessToken = CryptoCore::generateChallenge();
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
						.addArgument( Argument::VncServerPassword, VeyonCore::authenticationCredentials().internalVncServerPassword().toByteArray() )
						.addArgument( Argument::VncServerPort, server.vncServerBasePort() + message.argument(Argument::VncServerPortOffset).toInt() ) );
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

		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false )
		{
			vDebug() << "not starting demo client since not running in a user session";
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
				m_demoServer = new DemoServer( message.argument( Argument::VncServerPort ).toInt(),
											   message.argument( Argument::VncServerPassword ).toByteArray(),
											   message.argument( Argument::DemoAccessToken ).toByteArray(),
											   m_configuration,
											   message.argument( Argument::DemoServerPort ).toInt(),
											   this );
			}
			return true;

		case StopDemoServer:
			if( m_demoServer )
			{
				m_demoServer->terminate();
			}
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
			VeyonCore::authenticationCredentials().setToken( message.argument( Argument::DemoAccessToken ).toByteArray() );

			if( m_demoClient == nullptr )
			{
				const auto demoServerHost = message.argument( Argument::DemoServerHost ).toString();
				const auto demoServerPort = message.argument( Argument::DemoServerPort ).toInt();
				const auto isFullscreenDemo = message.featureUid() == m_demoClientFullScreenFeature.uid();
				const auto viewport = message.argument( Argument::Viewport ).toRect();

				vDebug() << "connecting with master" << demoServerHost;
				m_demoClient = new DemoClient( demoServerHost, demoServerPort, isFullscreenDemo, viewport );
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



void DemoFeaturePlugin::addScreen( QScreen* screen )
{
	m_screens = QGuiApplication::screens();

	const auto screenIndex = m_screens.indexOf( screen ) + 1;
	if( m_screenSelection > ScreenSelectionNone &&
		screenIndex <= m_screenSelection )
	{
		m_screenSelection++;
	}

	updateFeatures();
}



void DemoFeaturePlugin::removeScreen( QScreen* screen )
{
	const auto screenIndex = m_screens.indexOf( screen ) + 1;
	if( screenIndex == m_screenSelection )
	{
		m_screenSelection = ScreenSelectionNone;
	}

	m_screens = QGuiApplication::screens();

	m_screenSelection = qMin( m_screenSelection, m_screens.size() );

	updateFeatures();
}



void DemoFeaturePlugin::updateFeatures()
{
	m_screenSelectionFeatures.clear();

	if( m_screens.size() > 1 )
	{
		m_screenSelectionFeatures.reserve( m_screens.size() + 1 );

		auto allScreensFlags = Feature::Option | Feature::Master;
		if( m_screenSelection <= ScreenSelectionNone )
		{
			allScreensFlags |= Feature::Checked;
		}

		m_screenSelectionFeatures.append( Feature{ QStringLiteral("DemoAllScreens"),
												   allScreensFlags,
												   Feature::Uid("2aca1e9f-25f9-4d0f-9729-01b03c80ab28"),
												   m_demoFeature.uid(), tr("All screens"), {}, {}, {} } );

		int index = 1;
		for( auto screen : qAsConst(m_screens) )
		{
			const auto name = QStringLiteral( "DemoScreen%1" ).arg( index );

			auto displayName = tr( "Screen %1 [%2]" ).arg( index ).arg( screen->name() );

#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
			if( screen->manufacturer().isEmpty() == false &&
				screen->model().isEmpty() == false )
			{
				displayName += QStringLiteral(" – %1 %2").arg( screen->manufacturer(), screen->model() );
			}
#endif

			auto flags = Feature::Option | Feature::Master;
			if( index == m_screenSelection )
			{
				flags |= Feature::Checked;
			}

			m_screenSelectionFeatures.append( Feature{ name, flags,
													   Feature::Uid::createUuidV5( m_demoFeature.uid(), name ),
													   m_demoFeature.uid(), displayName, {}, {}, {} } );
			++index;
		}
	}

	m_features = m_staticFeatures + m_screenSelectionFeatures;

	const auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	if( master )
	{
		master->reloadSubFeatures();
	}
}




QRect DemoFeaturePlugin::viewportFromScreenSelection() const
{
	if( m_screenSelection <= ScreenSelectionNone )
	{
		return {};
	}

	QPoint minimumScreenPosition{};
	for( const auto* screen : m_screens )
	{
		minimumScreenPosition.setX( qMin( minimumScreenPosition.x(), screen->geometry().x() ) );
		minimumScreenPosition.setY( qMin( minimumScreenPosition.y(), screen->geometry().y() ) );
	}

	const auto screen = m_screens.value( m_screenSelection - 1 );
	if( screen )
	{
		return screen->geometry().translated( -minimumScreenPosition );
	}

	return {};
}



bool DemoFeaturePlugin::controlDemoServer()
{
	if( m_demoServerClients.isEmpty() )
	{
		sendFeatureMessage( FeatureMessage{ m_demoServerFeature.uid(), StopDemoServer },
							m_demoServerControlInterfaces );
	}
	else
	{
		const auto demoServerPort = m_demoServerArguments.value( argToString(Argument::DemoServerPort),
																 VeyonCore::config().demoServerPort() + VeyonCore::sessionId() ).toInt();
		const auto vncServerPortOffset = m_demoServerArguments.value( argToString(Argument::VncServerPortOffset),
																	  VeyonCore::sessionId() ).toInt();
		const auto demoAccessToken = m_demoServerArguments.value( argToString(Argument::DemoAccessToken),
																  m_demoAccessToken.toByteArray() ).toByteArray();

		sendFeatureMessage( FeatureMessage{ m_demoServerFeature.uid(), StartDemoServer }
								.addArgument( Argument::DemoAccessToken, demoAccessToken )
								.addArgument( Argument::VncServerPortOffset, vncServerPortOffset )
								.addArgument( Argument::DemoServerPort, demoServerPort ),
							m_demoServerControlInterfaces );

		return true;
	}

	return false;
}



bool DemoFeaturePlugin::controlDemoClient( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation == Operation::Start )
	{
		const auto demoAccessToken = arguments.value( argToString(Argument::DemoAccessToken),
													  m_demoAccessToken.toByteArray() ).toByteArray();
		const auto demoServerHost = arguments.value( argToString(Argument::DemoServerHost) ).toString();
		const auto demoServerPort = arguments.value( argToString(Argument::DemoServerPort),
													 VeyonCore::config().demoServerPort() + VeyonCore::sessionId() ).toInt();

		QRect viewport{
			arguments.value( argToString(Argument::ViewportX) ).toInt(),
			arguments.value( argToString(Argument::ViewportY) ).toInt(),
			arguments.value( argToString(Argument::ViewportWidth) ).toInt(),
			arguments.value( argToString(Argument::ViewportHeight) ).toInt()
		};

		if( viewport.isNull() || viewport.isEmpty() )
		{
			viewport = viewportFromScreenSelection();
		}

		const auto disableUpdates = m_configuration.slowDownThumbnailUpdates();

		for( const auto& computerControlInterface : computerControlInterfaces )
		{
			m_demoServerClients += computerControlInterface;

			if( disableUpdates )
			{
				computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Disabled );
			}
		}

		sendFeatureMessage( FeatureMessage{ featureUid, StartDemoClient }
								.addArgument( Argument::DemoAccessToken, demoAccessToken )
								.addArgument( Argument::DemoServerHost, demoServerHost )
								.addArgument( Argument::DemoServerPort, demoServerPort )
								.addArgument( Argument::Viewport, viewport ),
							computerControlInterfaces );

		return true;
	}

	if( operation == Operation::Stop )
	{
		const auto enableUpdates = m_configuration.slowDownThumbnailUpdates();

		for( const auto& computerControlInterface : computerControlInterfaces )
		{
			m_demoServerClients.removeAll( computerControlInterface );

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
