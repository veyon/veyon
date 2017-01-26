/*
 * MasterCore.cpp - management of application-global instances
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

#include <QtNetwork/QHostAddress>

#include "MasterCore.h"
#include "ItalcVncConnection.h"
#include "ItalcCoreConnection.h"
#include "ComputerManager.h"
#include "PersonalConfig.h"


MasterCore* MasterCore::s_instance = Q_NULLPTR;

MasterCore::MasterCore() :
	m_localDisplay( new ItalcVncConnection ),
	m_localService( new ItalcCoreConnection( m_localDisplay ) ),
	m_personalConfig( new PersonalConfig( Configuration::Store::XmlFile ) ),
	m_computerManager( new ComputerManager )
{
	/*	ItalcVncConnection * conn = new ItalcVncConnection( this );
		// attach ItalcCoreConnection to it so we can send extended iTALC commands
		m_localICA = new ItalcCoreConnection( conn );

		conn->setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
		conn->setPort( ItalcCore::config->coreServerPort() );
		conn->start();
		*/
	m_localDisplay->setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
	m_localDisplay->setFramebufferUpdateInterval( -1 );
	m_localDisplay->start();
/*
	if( !m_localDisplay->waitForConnected( 5000 ) )
	{
		// TODO
	}
*/
}

void MasterCore::shutdown()
{
	m_personalConfig->flushStore();

	delete m_computerManager;
	delete m_localService;
	delete m_personalConfig;
	delete m_localDisplay;
}
