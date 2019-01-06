/*
 * ComputerControlInterface.cpp - interface class for controlling a computer
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

#include "BuiltinFeatures.h"
#include "ComputerControlInterface.h"
#include "Computer.h"
#include "FeatureControl.h"
#include "MonitoringMode.h"
#include "UserSessionControl.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncConnection.h"


ComputerControlInterface::ComputerControlInterface( const Computer& computer,
													QObject* parent ) :
	QObject( parent ),
	m_computer( computer ),
	m_state( Disconnected ),
	m_user(),
	m_scaledScreenSize(),
	m_vncConnection( nullptr ),
	m_connection( nullptr ),
	m_builtinFeatures( nullptr ),
	m_connectionWatchdogTimer( this ),
	m_screenUpdated( false )
{
	m_connectionWatchdogTimer.setInterval( ConnectionWatchdogTimeout );
	m_connectionWatchdogTimer.setSingleShot( true );
	connect( &m_connectionWatchdogTimer, &QTimer::timeout, this, &ComputerControlInterface::restartConnection );
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



ComputerControlInterface::Pointer ComputerControlInterface::weakPointer()
{
	return Pointer( this, []( ComputerControlInterface* ) { } );
}



void ComputerControlInterface::start( QSize scaledScreenSize, BuiltinFeatures* builtinFeatures )
{
	// make sure we do not leak
	stop();

	m_scaledScreenSize = scaledScreenSize;
	m_builtinFeatures = builtinFeatures;

	if( m_computer.hostAddress().isEmpty() == false )
	{
		m_vncConnection = new VncConnection();
		m_vncConnection->setHost( m_computer.hostAddress() );
		m_vncConnection->setQuality( VncConnection::ThumbnailQuality );
		m_vncConnection->setScaledSize( m_scaledScreenSize );
		m_vncConnection->setFramebufferUpdateInterval( VeyonCore::config().computerMonitoringUpdateInterval() );

		m_connection = new VeyonConnection( m_vncConnection );

		m_vncConnection->start();

		connect( m_vncConnection, &VncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::setScreenUpdateFlag );
		connect( m_vncConnection, &VncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::resetWatchdog );

		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateState );
		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateUser );
		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateActiveFeatures );

		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::handleFeatureMessage );
		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::resetWatchdog );

		auto userUpdateTimer = new QTimer( m_connection );
		connect( userUpdateTimer, &QTimer::timeout, this, &ComputerControlInterface::updateUser );
		userUpdateTimer->start( UserUpdateInterval );

		auto activeFeaturesUpdateTimer = new QTimer( m_connection );
		connect( activeFeaturesUpdateTimer, &QTimer::timeout, this, &ComputerControlInterface::updateActiveFeatures );
		activeFeaturesUpdateTimer->start( ActiveFeaturesUpdateInterval );
	}
	else
	{
		qWarning( "ComputerControlInterface::start(): computer host address is empty!" );
	}
}



void ComputerControlInterface::stop()
{
	if( m_connection )
	{
		delete m_connection;
		m_connection = nullptr;
	}

	if( m_vncConnection )
	{
		// do not delete VNC connection but let it delete itself after stopping automatically
		m_vncConnection->stopAndDeleteLater();
		m_vncConnection = nullptr;
	}

	m_connectionWatchdogTimer.stop();

	m_state = Disconnected;
}



void ComputerControlInterface::setScaledScreenSize( QSize scaledScreenSize )
{
	m_scaledScreenSize = scaledScreenSize;

	if( m_vncConnection )
	{
		m_vncConnection->setScaledSize( m_scaledScreenSize );
	}

	setScreenUpdateFlag();
}



QImage ComputerControlInterface::scaledScreen() const
{
	if( m_vncConnection && m_vncConnection->isConnected() )
	{
		return m_vncConnection->scaledScreen();
	}

	return QImage();
}



QImage ComputerControlInterface::screen() const
{
	if( m_vncConnection && m_vncConnection->isConnected() )
	{
		return m_vncConnection->image();
	}

	return QImage();
}



void ComputerControlInterface::setUser( const QString& user )
{
	if( user != m_user )
	{
		m_user = user;

		emit userChanged();
	}
}



void ComputerControlInterface::setActiveFeatures( const FeatureUidList& activeFeatures )
{
	if( activeFeatures != m_activeFeatures )
	{
		m_activeFeatures = activeFeatures;

		emit activeFeaturesChanged();
	}
}



void ComputerControlInterface::setDesignatedModeFeature( Feature::Uid designatedModeFeature )
{
	m_designatedModeFeature = designatedModeFeature;

	updateActiveFeatures();
}



void ComputerControlInterface::sendFeatureMessage( const FeatureMessage& featureMessage )
{
	if( m_connection && m_connection->isConnected() )
	{
		m_connection->sendFeatureMessage( featureMessage );
	}
}



void ComputerControlInterface::resetWatchdog()
{
	if( state() == Connected )
	{
		m_connectionWatchdogTimer.start();
	}
}



void ComputerControlInterface::restartConnection()
{
	if( m_vncConnection )
	{
		qDebug() << Q_FUNC_INFO;
		m_vncConnection->restart();

		m_connectionWatchdogTimer.stop();
	}
}



void ComputerControlInterface::updateState()
{
	if( m_vncConnection )
	{
		switch( m_vncConnection->state() )
		{
		case VncConnection::Disconnected: m_state = Disconnected; break;
		case VncConnection::Connecting: m_state = Connecting; break;
		case VncConnection::Connected: m_state = Connected; break;
		case VncConnection::HostOffline: m_state = Offline; break;
		case VncConnection::ServiceUnreachable: m_state = ServiceUnreachable; break;
		case VncConnection::AuthenticationFailed: m_state = AuthenticationFailed; break;
		default: m_state = Unknown; break;
		}
	}
	else
	{
		m_state = Disconnected;
	}

	setScreenUpdateFlag();
}



void ComputerControlInterface::updateUser()
{
	if( m_vncConnection && m_connection && state() == Connected )
	{
		if( user().isEmpty() )
		{
			m_builtinFeatures->userSessionControl().getUserSessionInfo( { weakPointer() } );
		}
	}
	else
	{
		setUser( QString() );
	}
}



void ComputerControlInterface::updateActiveFeatures()
{
	if( m_vncConnection && m_connection && state() == Connected )
	{
		m_builtinFeatures->featureControl().queryActiveFeatures( { weakPointer() } );
	}
	else
	{
		setActiveFeatures( {} );
	}
}



void ComputerControlInterface::handleFeatureMessage( const FeatureMessage& message )
{
	emit featureMessageReceived( message, weakPointer() );
}
