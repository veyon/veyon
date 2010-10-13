/*
 * IpcMaster.cpp - class Ipc::Master which manages IpcSlaves
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
#include <QtNetwork/QLocalSocket>

#include "Ipc/Master.h"

namespace Ipc
{

Master::Master() :
	m_serverId( Ipc::Id( "Ipc::Master::%1").arg( qrand() ) ),
	m_processes()
{
	if( !listen( m_serverId ) )
	{
		qCritical( "Error in listen() in Ipc::Master::Master()" );
	}

	connect( this, SIGNAL( newConnection() ),
			 this, SLOT( acceptConnection() ) );
}




Master::~Master()
{
	const QList<Ipc::Id> processIds = m_processes.keys();
	foreach( const Ipc::Id &id, processIds )
	{
		stopSlave( id );
	}
}




void Master::createSlave( const Ipc::Id &id )
{
	// make sure to stop a slave with the same id
	stopSlave( id );

	ProcessInformation pi;
	pi.process = new QProcess;
	pi.sock = NULL;
	pi.process->start( QCoreApplication::applicationFilePath(),
						QStringList() << "-slave" << id << m_serverId );

	m_processes[id] = pi;
}




void Master::stopSlave( const Ipc::Id &id )
{
	if( isSlaveRunning( id ) )
	{
		if( isSlaveRunning( id ) )
		{
			sendMessage( id, Ipc::Commands::Quit );

			if( !m_processes[id].process->waitForFinished( 5000 ) )
			{
				m_processes[id].process->terminate();
			}
		}

		delete m_processes[id].process;
		delete m_processes[id].sock;
		m_processes.remove( id );
	}
}




bool Master::isSlaveRunning( const Ipc::Id &id )
{
	if( m_processes.contains( id ) )
	{
		return m_processes[id].process->state() == QProcess::Running;
	}

	return false;
}




void Master::sendMessage( const Ipc::Id &id, const Ipc::Msg &msg )
{
	if( m_processes.contains( id ) )
	{
		if( m_processes[id].sock )
		{
			msg.send( m_processes[id].sock );
		}
		else
		{
			m_processes[id].pendingMessages << msg;
		}
	}
}




Ipc::Msg Master::receiveMessage( const Ipc::Id &id )
{
	Ipc::Msg m;
	if( m_processes.contains( id ) && m_processes[id].sock )
	{
		m.receive( m_processes[id].sock );
	}

	return m;
}




void Master::acceptConnection()
{
	QLocalSocket *s = nextPendingConnection();
	Ipc::Msg( Ipc::Commands::Identify ).send( s );

	// check whether we got a proper identification message
	Ipc::Msg answer;
	if( answer.receive( s ).isValid() &&
			answer.cmd() == Ipc::Commands::Identify )
	{
		const Ipc::Id id = answer.arg( "id" );
		if( m_processes.contains( id ) )
		{
			m_processes[id].sock = s;

			// connect to our slot
			connect( s, SIGNAL( readyRead() ),
						this, SLOT( receiveMessages() ) );

			// send all pending messages that were queued before the connection
			// was established completely
			foreach( const Ipc::Msg &m, m_processes[id].pendingMessages )
			{
				m.send( s );
			}
		}
	}
}




void Master::receiveMessages()
{
	// one or more sockets are ready to read
	for( ProcessMap::Iterator it = m_processes.begin();
								it != m_processes.end(); ++it )
	{
		while( it->sock->bytesAvailable() > 0 )
		{
			// receive and handle message
			Ipc::Msg m;
			if( m.receive( it->sock ).isValid() )
			{
				if( m.cmd() == Ipc::Commands::UnknownCommand )
				{
					qWarning() << "Slave" << it.key() <<
						"could not handle command" << m.arg( "Command" );
				}
				else
				{
					handleMessage( m );
				}
			}
		}
	}
}



}
