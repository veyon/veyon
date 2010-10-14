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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include "DemoServerMaster.h"
#include "Ipc/QtSlaveLauncher.h"
#include "ItalcCore.h"


DemoServerMaster::DemoServerMaster() :
	Ipc::Master()
{
}



DemoServerMaster::~DemoServerMaster()
{
}



void DemoServerMaster::start( int sourcePort, int destinationPort )
{
	// create a slave launcher for the ICA executable as it contains the
	// DemoServerSlave
	Ipc::QtSlaveLauncher *s = new Ipc::QtSlaveLauncher(
		QCoreApplication::applicationDirPath() + QDir::separator() + "ica" );

	createSlave( ItalcCore::Ipc::IdDemoServer, s );
	sendMessage( ItalcCore::Ipc::IdDemoServer,
					Ipc::Msg( ItalcCore::Ipc::DemoServer::StartDemoServer ).
						addArg( ItalcCore::Ipc::DemoServer::UserRole, ItalcCore::role ).
						addArg( ItalcCore::Ipc::DemoServer::SourcePort, sourcePort ).
						addArg( ItalcCore::Ipc::DemoServer::DestinationPort, destinationPort ) );

	m_serverPort = destinationPort;
}



void DemoServerMaster::stop()
{
	stopSlave( ItalcCore::Ipc::IdDemoServer );
}



void DemoServerMaster::updateAllowedHosts()
{
	sendMessage( ItalcCore::Ipc::IdDemoServer,
					Ipc::Msg( ItalcCore::Ipc::DemoServer::UpdateAllowedHosts ).
						addArg( ItalcCore::Ipc::DemoServer::AllowedHosts, m_allowedHosts ) );
}



bool DemoServerMaster::handleMessage( const Ipc::Msg &m )
{
	return false;
}

