/*
 * ComputerControlInterface.cpp - interface class for controlling a computer
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include "ComputerControlInterface.h"
#include "Computer.h"
#include "ItalcVncConnection.h"
#include "ItalcCoreConnection.h"


ComputerControlInterface::ComputerControlInterface(const Computer &computer) :
	QObject(),
	m_computer( computer ),
	m_mode( ModeMonitoring ),
	m_state( Disconnected ),
	m_scaledScreenSize(),
	m_vncConnection( nullptr ),
	m_coreConnection( nullptr ),
	m_screenUpdated( false )
{
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



void ComputerControlInterface::start( const QSize& scaledScreenSize )
{
	m_scaledScreenSize = scaledScreenSize;

	if( m_computer.hostAddress().isEmpty() == false )
	{
		m_vncConnection = new ItalcVncConnection( this );
		m_vncConnection->setHost( m_computer.hostAddress() );
		m_vncConnection->setQuality( ItalcVncConnection::ThumbnailQuality );
		m_vncConnection->setScaledSize( m_scaledScreenSize );
		m_vncConnection->setFramebufferUpdateInterval( 1000 );	// TODO: replace hard-coded value
		m_vncConnection->start();

		m_coreConnection = new ItalcCoreConnection( m_vncConnection );

		connect( m_vncConnection, &ItalcVncConnection::framebufferUpdateComplete,
				 this, &ComputerControlInterface::setScreenUpdateFlag );
		connect( m_vncConnection, &ItalcVncConnection::stateChanged,
				 this, &ComputerControlInterface::updateState );
	}
}



void ComputerControlInterface::stop()
{
	if( m_coreConnection )
	{
		delete m_coreConnection;
	}

	if( m_vncConnection )
	{
		// do not delete VNC connection but let it delete itself after stopping automatically
		m_vncConnection->stop( true );
	}

	m_state = Disconnected;
}



void ComputerControlInterface::setScaledScreenSize( const QSize& scaledScreenSize )
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
		case ItalcVncConnection::Disconnected: m_state = Disconnected; break;
		case ItalcVncConnection::Connecting: m_state = Connecting; break;
		case ItalcVncConnection::Connected: m_state = Connected; break;
		case ItalcVncConnection::HostUnreachable: m_state = Unreachable; break;
		default: m_state = Unknown; break;
		}
	}
	else
	{
		m_state = Disconnected;
	}

	setScreenUpdateFlag();
}
