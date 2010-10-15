/*
 * DemoServerSlave.cpp - an IcaSlave providing the demo window
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

#include <QtCore/QThread>
#include <QtNetwork/QHostInfo>

#include "DemoServerSlave.h"
#include "ItalcCoreServer.h"
#include "ItalcVncServer.h"

const Ipc::Command DemoServerSlave::StartDemoServer = ItalcCore::Ipc::DemoServer::StartDemoServer;
const Ipc::Argument DemoServerSlave::UserRole = ItalcCore::Ipc::DemoServer::UserRole;
const Ipc::Argument DemoServerSlave::SourcePort = ItalcCore::Ipc::DemoServer::SourcePort;
const Ipc::Argument DemoServerSlave::DestinationPort = ItalcCore::Ipc::DemoServer::DestinationPort;

const Ipc::Command DemoServerSlave::UpdateAllowedHosts = ItalcCore::Ipc::DemoServer::UpdateAllowedHosts;
const Ipc::Argument DemoServerSlave::AllowedHosts = ItalcCore::Ipc::DemoServer::AllowedHosts;


// a helper threading running the VNC reflector
class DemoServerThread : public QThread
{
public:
	DemoServerThread( int srcPort, int dstPort ) :
		QThread(),
		m_srcPort( srcPort ),
		m_dstPort( dstPort )
	{
	}


protected:
	virtual void run()
	{
		ItalcCoreServer coreServer;	// required for authentication
		ItalcVncServer::runVncReflector( m_srcPort, m_dstPort );
	}


private:
	int m_srcPort;
	int m_dstPort;

} ;




DemoServerSlave::DemoServerSlave() :
	IcaSlave(),
	m_demoServerThread( NULL )
{
}



DemoServerSlave::~DemoServerSlave()
{
	delete m_demoServerThread;
}



bool DemoServerSlave::handleMessage( const Ipc::Msg &m )
{
	if( m.cmd() == StartDemoServer )
	{
		ItalcCore::role =
				static_cast<ItalcCore::UserRoles>( m.argV( UserRole ).toInt() );
		m_demoServerThread = new DemoServerThread(
			m.argV( SourcePort ).toInt(), m.argV( DestinationPort ).toInt() );
		m_demoServerThread->start();
		return true;
	}
	else if( m.cmd() == UpdateAllowedHosts )
	{
		const QStringList allowedHosts = m.argV( AllowedHosts ).toStringList();
		// resolve IP addresses of all hosts
		QStringList allowedIPs;
		foreach( const QString &host, allowedHosts )
		{
			if( QHostAddress().setAddress( host ) )
			{
				allowedIPs += host;
			}
			else
			{
				QList<QHostAddress> addr = QHostInfo::fromName( host ).addresses();
				foreach( const QHostAddress a, addr )
				{
					allowedIPs += a.toString();
				}
			}
		}

		// now we can use the list of IP addresses for host-based authentication
		ItalcCoreServer::instance()->setAllowedIPs( allowedIPs );

		return true;
	}

	return false;
}

