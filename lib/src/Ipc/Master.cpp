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
#include <QtNetwork/QTcpSocket>

#include "Ipc/Master.h"
#include "Ipc/QtSlaveLauncher.h"
#include "Logger.h"

namespace Ipc
{

Master::Master( const QString &applicationFilePath ) :
	QTcpServer(),
	m_applicationFilePath( applicationFilePath ),
	m_socketReceiveMapper( this ),
	m_processes(),
	m_processMapMutex( QMutex::Recursive )
{
	if( !listen( QHostAddress::LocalHost ) )
	{
		qCritical( "Error in listen() in Ipc::Master::Master()" );
	}

	ilogf( Info, "Ipc::Master: listening at port %d", serverPort() );

	connect( &m_socketReceiveMapper, SIGNAL( mapped( QObject *) ),
				this, SLOT( receiveMessage( QObject * ) ) );

	connect( this, SIGNAL( newConnection() ),
			 this, SLOT( acceptConnection() ) );

	connect( this, SIGNAL( messagesPending() ),
				this, SLOT( sendPendingMessages() ), Qt::QueuedConnection );
}




Master::~Master()
{
	QMutexLocker l( &m_processMapMutex );

	const QList<Ipc::Id> processIds = m_processes.keys();
	foreach( const Ipc::Id &id, processIds )
	{
		stopSlave( id );
	}

	ilog( Info, "Stopped Ipc::Master" );
}




void Master::createSlave( const Ipc::Id &id, SlaveLauncher *slaveLauncher )
{
	// make sure to stop a slave with the same id
	stopSlave( id );

	if( slaveLauncher == NULL )
	{
		slaveLauncher = new QtSlaveLauncher( applicationFilePath() );
	}

	ProcessInformation pi;

	pi.slaveLauncher = slaveLauncher;

	m_processMapMutex.lock();
	m_processes[id] = pi;
	m_processMapMutex.unlock();

	LogStream() << "Starting slave" << id << "at port" << serverPort();
	pi.slaveLauncher->start( QStringList() << "-slave" << id <<
											QString::number( serverPort() ) );

}




void Master::stopSlave( const Ipc::Id &id )
{
	QMutexLocker l( &m_processMapMutex );

	if( m_processes.contains( id ) )
	{
		LogStream() << "Stopping slave" << id;
		if( isSlaveRunning( id ) )
		{
			l.unlock();
			sendMessage( id, Ipc::Commands::Quit );
			m_processes[id].slaveLauncher->stop();
			l.relock();
		}

		delete m_processes[id].slaveLauncher;

		if( m_processes[id].sock != NULL )
		{
			// schedule deletion of socket - can't delete it here as this
			// crashes Qt on Win32
			m_processes[id].sock->deleteLater();
		}

		m_processes.remove( id );
	}
	else
	{
		qDebug() << "Ipc::Master: can't stop slave" << id
					<< "as it does not exist";
	}
}




bool Master::isSlaveRunning( const Ipc::Id &id )
{
	QMutexLocker l( &m_processMapMutex );

	if( m_processes.contains( id ) )
	{
		return m_processes[id].slaveLauncher->isRunning();
	}

	return false;
}




void Master::sendMessage( const Ipc::Id &id, const Ipc::Msg &msg )
{
	QMutexLocker l( &m_processMapMutex );

	if( m_processes.contains( id ) )
	{
		qDebug() << "Ipc::Master: queuing message" << msg.cmd()
					<< "for slave" << id;
		m_processes[id].pendingMessages << msg;
		if( m_processes[id].sock )
		{
			emit messagesPending();
		}
	}
	else
	{
		qWarning() << "Ipc::Master: can't send message" << msg.cmd()
					<< "to non-existing slave" << id;
	}
}




Ipc::Msg Master::receiveMessage( const Ipc::Id &id )
{
	QMutexLocker l( &m_processMapMutex );

	Ipc::Msg m;
	if( m_processes.contains( id ) && m_processes[id].sock )
	{
		m.receive( m_processes[id].sock );
		qDebug() << "Ipc::Master: received message" << m.cmd()
					<< "from slave" << id;
	}

	return m;
}




void Master::acceptConnection()
{
	qDebug( "Ipc::Master: accepting connection" );

	QTcpSocket *s = nextPendingConnection();

	// connect to readyRead() signal of new connection
	connect( s, SIGNAL( readyRead() ),
				&m_socketReceiveMapper, SLOT( map() ) );

	// add mapping so we can identify the socket later
	m_socketReceiveMapper.setMapping( s, s );

	Ipc::Msg( Ipc::Commands::Identify ).send( s );

}




void Master::receiveMessage( QObject *sockObj )
{
	QTcpSocket *sock = qobject_cast<QTcpSocket *>( sockObj );
	if( !sock )
	{
		return;
	}

	while( sock->bytesAvailable() > 0 )
	{
		// receive and handle message
		Ipc::Msg m;
		if( m.receive( sock ).isValid() )
		{
			QMutexLocker l( &m_processMapMutex );

			// search for slave ID
			Ipc::Id slaveId;
			for( ProcessMap::ConstIterator it = m_processes.begin();
							it != m_processes.end(); ++it )
			{
				if( it->sock == sock )
				{
					slaveId = it.key();
				}
			}

			if( m.cmd() != Ipc::Commands::Ping )
			{
				qDebug() << "Ipc::Master: received message" << m.cmd()
							<< "from slave" << slaveId;
			}

			if( m.cmd() == Ipc::Commands::UnknownCommand )
			{
				qWarning() << "Slave" << slaveId
						<< "could not handle command"
						<< m.arg( Ipc::Arguments::Command );
			}
			else if( m.cmd() == Ipc::Commands::Ping )
			{
				// send message back
				m.send( sock );
			}
			else if( m.cmd() == Ipc::Commands::Identify )
			{
				// check whether we got a proper identification message
				const Ipc::Id id = m.arg( Ipc::Arguments::Id );
				if( m_processes.contains( id ) &&
						m_processes[id].sock == NULL )
				{
					m_processes[id].sock = sock;
					// send all pending messages that were queued before the
					// connection was established completely
					sendPendingMessages();
				}
				else
				{
					qWarning() << "Slave not existing or already registered" << id;
					delete sock;
					break;
				}
			}
			else
			{
				handleMessage( slaveId, m );
			}
		}
	}
}



void Master::sendPendingMessages()
{
	qDebug() << "Master::sendPendingMessages()";
	QMutexLocker l( &m_processMapMutex );

	for( ProcessMap::Iterator it = m_processes.begin();
							it != m_processes.end(); ++it )
	{
		if( it->sock && !it->pendingMessages.isEmpty() )
		{
			foreach( const Ipc::Msg &m, it->pendingMessages )
			{
				qDebug() << "Ipc::Master: sending message" << m.cmd()
							<< "to slave" << it.key()
							<< "with arguments" << m.args();
				m.send( it->sock );
			}
			it->pendingMessages.clear();
		}
	}
}



}
