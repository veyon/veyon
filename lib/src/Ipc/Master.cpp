/*
 * IpcMaster.cpp - class Ipc::Master which manages IpcSlaves
 *
 * Copyright (c) 2010-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtCore/QThread>
#include <QtNetwork/QTcpSocket>

#include "Ipc/Master.h"
#include "Ipc/QtSlaveLauncher.h"
#include "Logger.h"

Q_DECLARE_METATYPE(Ipc::Id)
Q_DECLARE_METATYPE(Ipc::Msg)
Q_DECLARE_METATYPE(Ipc::SlaveLauncher*)

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

	qRegisterMetaType<Ipc::Id>();
	qRegisterMetaType<Ipc::Msg>();
	qRegisterMetaType<Ipc::SlaveLauncher *>();
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




void Master::createSlave( const Ipc::Id& id, Ipc::SlaveLauncher *slaveLauncher )
{
	if( thread() != QThread::currentThread() )
	{
		QMetaObject::invokeMethod( this, "createSlave", Qt::BlockingQueuedConnection,
								   Q_ARG( const Ipc::Id&, id ), Q_ARG( Ipc::SlaveLauncher*, slaveLauncher ) );
		return;
	}

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




void Master::stopSlave( const Ipc::Id& id )
{
	if( thread() != QThread::currentThread() )
	{
		QMetaObject::invokeMethod( this, "stopSlave", Qt::BlockingQueuedConnection, Q_ARG( Ipc::Id, id ) );
		return;
	}

	QMutexLocker l( &m_processMapMutex );

	if( m_processes.contains( id ) )
	{
		LogStream() << "Stopping slave" << id;
		if( isSlaveRunning( id ) )
		{
			// tell slave to quit
			sendMessage( id, Ipc::Commands::Quit );

			// close socket so that client quits even if quit message isn't processed
			if( m_processes[id].sock != NULL )
			{
				m_processes[id].sock->close();
			}

			// terminate process after internal timeout
			m_processes[id].slaveLauncher->stop();
		}

		delete m_processes[id].sock;

		m_processes.remove( id );
	}
	else
	{
		qDebug() << "Ipc::Master: can't stop slave" << id
					<< "as it does not exist";
	}
}




bool Master::isSlaveRunning( const Ipc::Id& id )
{
	QMutexLocker l( &m_processMapMutex );

	return m_processes.contains( id ) &&
			m_processes[id].slaveLauncher &&
			m_processes[id].slaveLauncher->isRunning();

	return false;
}




void Master::sendMessage( const Ipc::Id& id, const Ipc::Msg& msg )
{
	if( thread() != QThread::currentThread() )
	{
		QMetaObject::invokeMethod( this, "sendMessage", Qt::BlockingQueuedConnection,
								   Q_ARG( const Ipc::Id&, id ), Q_ARG(const Ipc::Msg&, msg ) );
		return;
	}

	QMutexLocker l( &m_processMapMutex );

	if( m_processes.contains( id ) )
	{
		ProcessInformation& processInfo = m_processes[id];

		if( processInfo.sock )
		{
			qDebug() << "Ipc::Master: sending message" << msg.cmd() << "to slave" << id;
			msg.send( processInfo.sock );
		}
		else
		{
			qDebug() << "Ipc::Master: queuing message" << msg.cmd() << "for slave" << id;
			processInfo.pendingMessages << msg;
		}
	}
	else
	{
		qWarning() << "Ipc::Master: can't send message" << msg.cmd()
					<< "to non-existing slave" << id;
	}
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
