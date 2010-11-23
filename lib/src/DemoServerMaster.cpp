/*
 * DemoServerMaster.cpp - class for controlling the VNC reflector slave process
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#include "DemoServerMaster.h"
#include "ItalcCore.h"
#include "ItalcSlaveManager.h"


DemoServerMaster::DemoServerMaster( ItalcSlaveManager *slaveManager ) :
	m_slaveManager( slaveManager ),
	m_allowedHosts(),
	m_serverPort( -1 )
{
}



DemoServerMaster::~DemoServerMaster()
{
}



void DemoServerMaster::start( int sourcePort, int destinationPort )
{
	m_slaveManager->createSlave( ItalcSlaveManager::IdDemoServer );
	m_slaveManager->sendMessage( ItalcSlaveManager::IdDemoServer,
					Ipc::Msg( ItalcSlaveManager::DemoServer::StartDemoServer ).
						addArg( ItalcSlaveManager::DemoServer::CommonSecret,
									ItalcCore::authenticationCredentials->commonSecret() ).
						addArg( ItalcSlaveManager::DemoServer::SourcePort, sourcePort ).
						addArg( ItalcSlaveManager::DemoServer::DestinationPort, destinationPort ) );

	m_serverPort = destinationPort;
}



void DemoServerMaster::stop()
{
	m_slaveManager->stopSlave( ItalcSlaveManager::IdDemoServer );
}



void DemoServerMaster::updateAllowedHosts()
{
	m_slaveManager->sendMessage( ItalcSlaveManager::IdDemoServer,
					Ipc::Msg( ItalcSlaveManager::DemoServer::UpdateAllowedHosts ).
						addArg( ItalcSlaveManager::DemoServer::AllowedHosts, m_allowedHosts ) );
}


