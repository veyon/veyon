/*
 * ComputerControlInterface.cpp - interface class for controlling a computer
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

#include "BuiltinFeatures.h"
#include "ComputerControlInterface.h"
#include "Computer.h"
#include "FeatureControl.h"
#include "MonitoringMode.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncConnection.h"


ComputerControlInterface::ComputerControlInterface( const Computer& computer, int port, QObject* parent ) :
	QObject( parent ),
	m_computer( computer ),
	m_port( port ),
	m_state( State::Disconnected ),
	m_userLoginName(),
	m_userFullName(),
	m_scaledScreenSize(),
	m_connection( nullptr ),
	m_connectionWatchdogTimer( this ),
	m_userUpdateTimer( this ),
	m_activeFeaturesUpdateTimer( this )
{
	m_connectionWatchdogTimer.setInterval( ConnectionWatchdogTimeout );
	m_connectionWatchdogTimer.setSingleShot( true );
	connect( &m_connectionWatchdogTimer, &QTimer::timeout, this, &ComputerControlInterface::restartConnection );

	connect( &m_userUpdateTimer, &QTimer::timeout, this, &ComputerControlInterface::updateUser );
	connect( &m_activeFeaturesUpdateTimer, &QTimer::timeout, this, &ComputerControlInterface::updateActiveFeatures );
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



void ComputerControlInterface::start( QSize scaledScreenSize, UpdateMode updateMode, AuthenticationProxy* authenticationProxy )
{
	// make sure we do not leak
	stop();

	m_scaledScreenSize = scaledScreenSize;

	if( m_computer.hostAddress().isEmpty() == false )
	{
		m_connection = new VeyonConnection;
		m_connection->setAuthenticationProxy( authenticationProxy );

		auto vncConnection = m_connection->vncConnection();
		vncConnection->setHost( m_computer.hostAddress() );
		if( m_port > 0 )
		{
			vncConnection->setPort( m_port );
		}
		vncConnection->setQuality( VncConnection::Quality::Thumbnail );
		vncConnection->setScaledSize( m_scaledScreenSize );

		connect( vncConnection, &VncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::resetWatchdog );
		connect( vncConnection, &VncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::screenUpdated );

		connect( vncConnection, &VncConnection::framebufferSizeChanged, this, &ComputerControlInterface::screenSizeChanged );

		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateState );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateUser );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateActiveFeatures );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::stateChanged );

		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::handleFeatureMessage );
		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::resetWatchdog );

		setUpdateMode( updateMode );

		vncConnection->start();
	}
	else
	{
		vWarning() << "computer host address is empty!";
	}
}



void ComputerControlInterface::stop()
{
	if( m_connection )
	{
		m_connection->stopAndDeleteLater();
		m_connection = nullptr;
	}

	m_activeFeaturesUpdateTimer.stop();
	m_userUpdateTimer.stop();
	m_connectionWatchdogTimer.stop();

	m_state = State::Disconnected;
}



bool ComputerControlInterface::hasValidFramebuffer() const
{
	return vncConnection() && vncConnection()->hasValidFramebuffer();
}



QSize ComputerControlInterface::screenSize() const
{
	if( vncConnection() )
	{
		return vncConnection()->image().size();
	}

	return {};
}



void ComputerControlInterface::setScaledScreenSize( QSize scaledScreenSize )
{
	m_scaledScreenSize = scaledScreenSize;

	if( vncConnection() )
	{
		vncConnection()->setScaledSize( m_scaledScreenSize );
	}
}



QImage ComputerControlInterface::scaledScreen() const
{
	if( vncConnection() && vncConnection()->isConnected() )
	{
		return vncConnection()->scaledScreen();
	}

	return {};
}



QImage ComputerControlInterface::screen() const
{
	if( vncConnection() && vncConnection()->isConnected() )
	{
		return vncConnection()->image();
	}

	return {};
}



void ComputerControlInterface::setUserInformation( const QString& userLoginName, const QString& userFullName, int sessionId )
{
	if( userLoginName != m_userLoginName ||
		userFullName != m_userFullName ||
		sessionId != m_userSessionId )
	{
		m_userLoginName = userLoginName;
		m_userFullName = userFullName;
		m_userSessionId = sessionId;

		Q_EMIT userChanged();
	}
}



void ComputerControlInterface::setActiveFeatures( const FeatureUidList& activeFeatures )
{
	if( activeFeatures != m_activeFeatures )
	{
		m_activeFeatures = activeFeatures;

		Q_EMIT activeFeaturesChanged();
	}
}



void ComputerControlInterface::updateActiveFeatures()
{
	lock();

	if( vncConnection() && state() == State::Connected )
	{
		VeyonCore::builtinFeatures().featureControl().queryActiveFeatures( { weakPointer() } );
	}
	else
	{
		setActiveFeatures( {} );
	}

	unlock();
}



void ComputerControlInterface::sendFeatureMessage( const FeatureMessage& featureMessage, bool wake )
{
	if( m_connection && m_connection->isConnected() )
	{
		m_connection->sendFeatureMessage( featureMessage, wake );
	}
}



bool ComputerControlInterface::isMessageQueueEmpty()
{
	if( vncConnection() && vncConnection()->isConnected() )
	{
		return vncConnection()->isEventQueueEmpty();
	}

	return true;
}



void ComputerControlInterface::setUpdateMode( UpdateMode updateMode )
{
	m_updateMode = updateMode;

	const auto computerMonitoringUpdateInterval = VeyonCore::config().computerMonitoringUpdateInterval();

	switch( updateMode )
	{
	case UpdateMode::Disabled:
		if( vncConnection() )
		{
			vncConnection()->setFramebufferUpdateInterval( UpdateIntervalDisabled );
		}

		m_userUpdateTimer.stop();
		m_activeFeaturesUpdateTimer.start( UpdateIntervalDisabled );
		break;

	case UpdateMode::Monitoring:
	case UpdateMode::Live:
		if( vncConnection() )
		{
			vncConnection()->setFramebufferUpdateInterval( updateMode == UpdateMode::Monitoring ?
															   computerMonitoringUpdateInterval : -1 );
		}

		m_userUpdateTimer.start( computerMonitoringUpdateInterval );
		m_activeFeaturesUpdateTimer.start( computerMonitoringUpdateInterval );
		break;
	}
}



ComputerControlInterface::Pointer ComputerControlInterface::weakPointer()
{
	return Pointer( this, []( ComputerControlInterface* ) { } );
}



void ComputerControlInterface::resetWatchdog()
{
	if( state() == State::Connected )
	{
		m_connectionWatchdogTimer.start();
	}
}



void ComputerControlInterface::restartConnection()
{
	if( vncConnection() )
	{
		vDebug();
		vncConnection()->restart();

		m_connectionWatchdogTimer.stop();
	}
}



void ComputerControlInterface::updateState()
{
	lock();

	if( vncConnection() )
	{
		switch( vncConnection()->state() )
		{
		case VncConnection::State::Disconnected: m_state = State::Disconnected; break;
		case VncConnection::State::Connecting: m_state = State::Connecting; break;
		case VncConnection::State::Connected: m_state = State::Connected; break;
		case VncConnection::State::HostOffline: m_state = State::HostOffline; break;
		case VncConnection::State::ServerNotRunning: m_state = State::ServerNotRunning; break;
		case VncConnection::State::AuthenticationFailed: m_state = State::AuthenticationFailed; break;
		default: m_state = VncConnection::State::Disconnected; break;
		}
	}
	else
	{
		m_state = State::Disconnected;
	}

	unlock();
}



void ComputerControlInterface::updateUser()
{
	lock();

	if( vncConnection() && state() == State::Connected )
	{
		if( userLoginName().isEmpty() )
		{
			VeyonCore::builtinFeatures().monitoringMode().queryLoggedOnUserInfo( { weakPointer() } );
		}
	}
	else
	{
		setUserInformation( {}, {}, -1 );
	}

	unlock();
}



void ComputerControlInterface::handleFeatureMessage( const FeatureMessage& message )
{
	Q_EMIT featureMessageReceived( message, weakPointer() );
}
