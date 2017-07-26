/*
 * ComputerControlInterface.cpp - interface class for controlling a computer
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "VeyonVncConnection.h"
#include "VeyonCoreConnection.h"
#include "UserSessionControl.h"


ComputerControlInterface::ComputerControlInterface( const Computer &computer,
													QObject* parent ) :
	QObject( parent ),
	m_computer( computer ),
	m_state( Disconnected ),
	m_user(),
	m_scaledScreenSize(),
	m_vncConnection( nullptr ),
	m_coreConnection( nullptr ),
	m_builtinFeatures( nullptr ),
	m_screenUpdated( false )
{
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



void ComputerControlInterface::start( QSize scaledScreenSize, BuiltinFeatures* builtinFeatures )
{
	m_scaledScreenSize = scaledScreenSize;
	m_builtinFeatures = builtinFeatures;

	if( m_computer.hostAddress().isEmpty() == false )
	{
		m_vncConnection = new VeyonVncConnection();
		m_vncConnection->setHost( m_computer.hostAddress() );
		m_vncConnection->setQuality( VeyonVncConnection::ThumbnailQuality );
		m_vncConnection->setScaledSize( m_scaledScreenSize );
		m_vncConnection->setFramebufferUpdateInterval( FramebufferUpdateInterval );
		m_vncConnection->start();

		m_coreConnection = new VeyonCoreConnection( m_vncConnection );

		connect( m_vncConnection, &VeyonVncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::setScreenUpdateFlag );
		connect( m_vncConnection, &VeyonVncConnection::framebufferUpdateComplete ,this, &ComputerControlInterface::updateUser );
		connect( m_vncConnection, &VeyonVncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::updateActiveFeatures );

		connect( m_vncConnection, &VeyonVncConnection::stateChanged, this, &ComputerControlInterface::updateState );
		connect( m_vncConnection, &VeyonVncConnection::stateChanged, this, &ComputerControlInterface::updateUser );
		connect( m_vncConnection, &VeyonVncConnection::stateChanged, this, &ComputerControlInterface::updateActiveFeatures );

		connect( m_coreConnection, &VeyonCoreConnection::featureMessageReceived,
				 this, &ComputerControlInterface::handleFeatureMessage );
	}
	else
	{
		qWarning( "ComputerControlInterface::start(): computer host address is empty!" );
	}
}



void ComputerControlInterface::stop()
{
	if( m_coreConnection )
	{
		delete m_coreConnection;
		m_coreConnection = nullptr;
	}

	if( m_vncConnection )
	{
		// do not delete VNC connection but let it delete itself after stopping automatically
		m_vncConnection->stop( true );
		m_vncConnection = nullptr;
	}

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
	}
}



void ComputerControlInterface::sendFeatureMessage( const FeatureMessage& featureMessage )
{
	if( m_coreConnection && m_coreConnection->isConnected() )
	{
		m_coreConnection->sendFeatureMessage( featureMessage );
	}
}



void ComputerControlInterface::updateState()
{
	if( m_vncConnection )
	{
		switch( m_vncConnection->state() )
		{
		case VeyonVncConnection::Disconnected: m_state = Disconnected; break;
		case VeyonVncConnection::Connecting: m_state = Connecting; break;
		case VeyonVncConnection::Connected: m_state = Connected; break;
		case VeyonVncConnection::HostOffline: m_state = Offline; break;
		case VeyonVncConnection::ServiceUnreachable: m_state = ServiceUnreachable; break;
		case VeyonVncConnection::AuthenticationFailed: m_state = AuthenticationFailed; break;
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
	if( m_vncConnection && m_coreConnection && state() == Connected )
	{
		if( user().isEmpty() )
		{
			m_builtinFeatures->userSessionControl().getUserSessionInfo( ComputerControlInterfaceList( { this } ) );
		}
	}
	else
	{
		setUser( QString() );
	}
}



void ComputerControlInterface::updateActiveFeatures()
{
	if( m_vncConnection && m_coreConnection && state() == Connected )
	{
		m_builtinFeatures->featureControl().queryActiveFeatures( ComputerControlInterfaceList( { this } ) );
	}
	else
	{
		setActiveFeatures( {} );
	}
}



void ComputerControlInterface::handleFeatureMessage( const FeatureMessage& message )
{
	emit featureMessageReceived( message, *this );
}
