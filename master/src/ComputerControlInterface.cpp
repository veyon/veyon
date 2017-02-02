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
	m_vncConnection( nullptr ),
	m_coreConnection( nullptr ),
	m_screenUpdated( false )
{
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



void ComputerControlInterface::start()
{
	m_vncConnection = new ItalcVncConnection( this );
	m_vncConnection->setHost( m_computer.hostAddress() );
	m_vncConnection->setQuality( ItalcVncConnection::ThumbnailQuality );
	m_vncConnection->setFramebufferUpdateInterval( 1000 );	// TODO: replace hard-coded value

	m_coreConnection = new ItalcCoreConnection( m_vncConnection );

	connect( m_vncConnection, &ItalcVncConnection::framebufferUpdateComplete,
			 this, &ComputerControlInterface::setScreenUpdateFlag );
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
}




QImage ComputerControlInterface::screen() const
{
	if( m_vncConnection )
	{
		return m_vncConnection->scaledScreen();
	}

	return QImage();
}
