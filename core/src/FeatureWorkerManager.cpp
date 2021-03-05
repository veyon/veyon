/*
 * FeatureWorkerManager.cpp - class for managing feature worker instances
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QCoreApplication>
#include <QDir>
#include <QThread>
#include <QTimer>

#include "FeatureManager.h"
#include "FeatureWorkerManager.h"
#include "Filesystem.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"

// clazy:excludeall=detaching-member

FeatureWorkerManager::FeatureWorkerManager( VeyonServerInterface& server, FeatureManager& featureManager, QObject* parent ) :
	QObject( parent ),
	m_server( server ),
	m_featureManager( featureManager ),
	m_tcpServer( this ),
	m_workersMutex( QMutex::Recursive )
{
	connect( &m_tcpServer, &QTcpServer::newConnection,
			 this, &FeatureWorkerManager::acceptConnection );

	if( !m_tcpServer.listen( QHostAddress::LocalHost,
							 static_cast<quint16>( VeyonCore::config().featureWorkerManagerPort() + VeyonCore::sessionId() ) ) )
	{
		vCritical() << "can't listen on localhost!";
	}

	auto pendingMessagesTimer = new QTimer( this );
	connect( pendingMessagesTimer, &QTimer::timeout, this, &FeatureWorkerManager::sendPendingMessages );

	pendingMessagesTimer->start( 100 );
}



FeatureWorkerManager::~FeatureWorkerManager()
{
	m_tcpServer.close();

	// properly shutdown all worker processes
	while( m_workers.isEmpty() == false )
	{
		stopWorker( m_workers.firstKey() );
	}
}



bool FeatureWorkerManager::startManagedSystemWorker( Feature::Uid featureUid )
{
	if( thread() != QThread::currentThread() )
	{
		vCritical() << "thread mismatch for feature" << featureUid;
		return false;
	}

	stopWorker( featureUid );

	Worker worker;

	worker.process = new QProcess;
	worker.process->setProcessChannelMode( QProcess::ForwardedChannels );

	connect( worker.process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			 worker.process, &QProcess::deleteLater );

	vDebug() << "Starting managed system worker for feature" << featureUid;
	if( qEnvironmentVariableIsSet("VEYON_VALGRIND_WORKERS") )
	{
		worker.process->start( QStringLiteral("valgrind"),
							   { QStringLiteral("--error-limit=no"),
								 QStringLiteral("--leak-check=full"),
								 QStringLiteral("--show-leak-kinds=all"),
								 QStringLiteral("--log-file=valgrind-%1.log").arg(VeyonCore::formattedUuid(featureUid)),
								 VeyonCore::filesystem().workerFilePath(), featureUid.toString() } );
	}
	else
	{
		worker.process->start( VeyonCore::filesystem().workerFilePath(), { featureUid.toString() } );
	}

	m_workersMutex.lock();
	m_workers[featureUid] = worker;
	m_workersMutex.unlock();

	return true;
}



bool FeatureWorkerManager::startUnmanagedSessionWorker( Feature::Uid featureUid )
{
	if( thread() != QThread::currentThread() )
	{
		vCritical() << "thread mismatch for feature" << featureUid;
		return false;
	}

	stopWorker( featureUid );

	Worker worker;

	vDebug() << "Starting worker (unmanaged session process) for feature" << featureUid;

	const auto currentUser = VeyonCore::platform().userFunctions().currentUser();
	if( currentUser.isEmpty() )
	{
		vDebug() << "could not determine current user - probably a console session with logon screen";
		return false;
	}

	const auto ret = VeyonCore::platform().coreFunctions().
					 runProgramAsUser( VeyonCore::filesystem().workerFilePath(), { featureUid.toString() },
									   currentUser,
									   VeyonCore::platform().coreFunctions().activeDesktopName() );
	if( ret == false )
	{
		return false;
	}

	m_workersMutex.lock();
	m_workers[featureUid] = worker;
	m_workersMutex.unlock();

	return true;
}



bool FeatureWorkerManager::stopWorker( Feature::Uid featureUid )
{
	if( thread() != QThread::currentThread() )
	{
		vCritical() << "thread mismatch for feature" << featureUid;
		return false;
	}

	QMutexLocker locker( &m_workersMutex );

	if( m_workers.contains( featureUid ) )
	{
		vDebug() << "Stopping worker for feature" << featureUid;

		auto& worker = m_workers[featureUid];

		if( worker.socket )
		{
			worker.socket->disconnect( this );
			disconnect( worker.socket );

			worker.socket->close();
			worker.socket->deleteLater();
		}

		if( worker.process )
		{
			auto killTimer = new QTimer;
			connect( killTimer, &QTimer::timeout, worker.process, &QProcess::terminate );
			connect( killTimer, &QTimer::timeout, worker.process, &QProcess::kill );
			connect( killTimer, &QTimer::timeout, killTimer, &QTimer::deleteLater );
			killTimer->start( 5000 );
		}

		m_workers.remove( featureUid );
	}

	return false;
}



void FeatureWorkerManager::sendMessageToManagedSystemWorker( const FeatureMessage& message )
{
	if( isWorkerRunning( message.featureUid() ) == false &&
		startManagedSystemWorker( message.featureUid() ) == false )
	{
		vCritical() << "could not start managed system worker";
		return;
	}

	sendMessage( message );

}



void FeatureWorkerManager::sendMessageToUnmanagedSessionWorker( const FeatureMessage& message )
{
	if( isWorkerRunning( message.featureUid() ) == false &&
		startUnmanagedSessionWorker( message.featureUid() ) == false )
	{
		vDebug() << "User session likely not yet available - retrying worker start";
		QTimer::singleShot( UnmanagedSessionProcessRetryInterval, this,
							[=]() { sendMessageToUnmanagedSessionWorker( message ); } );
		return;
	}

	sendMessage( message );
}



bool FeatureWorkerManager::isWorkerRunning( Feature::Uid featureUid )
{
	QMutexLocker locker( &m_workersMutex );
	return m_workers.contains( featureUid );
}



FeatureUidList FeatureWorkerManager::runningWorkers()
{
	QMutexLocker locker( &m_workersMutex );

	return m_workers.keys();
}



void FeatureWorkerManager::acceptConnection()
{
	vDebug() << "accepting connection";

	QTcpSocket* socket = m_tcpServer.nextPendingConnection();

	// connect to readyRead() signal of new connection
	connect( socket, &QTcpSocket::readyRead,
			 this, [=] () { processConnection( socket ); } );

	connect( socket, &QTcpSocket::disconnected,
			 this, [=] () { closeConnection( socket ); } );
}



void FeatureWorkerManager::processConnection( QTcpSocket* socket )
{
	FeatureMessage message;
	message.receive( socket );

	m_workersMutex.lock();

	// set socket information
	if( m_workers.contains( message.featureUid() ) )
	{
		if( m_workers[message.featureUid()].socket.isNull() )
		{
			m_workers[message.featureUid()].socket = socket;
			sendPendingMessages();
		}

		m_workersMutex.unlock();

		if( message.command() >= 0 )
		{
			m_featureManager.handleFeatureMessage( m_server, MessageContext( socket ), message );
		}
	}
	else
	{
		m_workersMutex.unlock();

		vCritical() << "got data from non-existing worker!" << message.featureUid();
	}
}



void FeatureWorkerManager::closeConnection( QTcpSocket* socket )
{
	m_workersMutex.lock();

	for( auto it = m_workers.begin(); it != m_workers.end(); )
	{
		if( it.value().socket == socket )
		{
			vDebug() << "removing worker after socket has been closed";
			it = m_workers.erase( it );
		}
		else
		{
			++it;
		}
	}

	m_workersMutex.unlock();

	socket->deleteLater();
}



void FeatureWorkerManager::sendMessage( const FeatureMessage& message )
{
	m_workersMutex.lock();

	if( m_workers.contains( message.featureUid() ) )
	{
		m_workers[message.featureUid()].pendingMessages.append( message );
	}
	else
	{
		vWarning() << "worker does not exist for feature" << message.featureUid();
	}

	m_workersMutex.unlock();
}



void FeatureWorkerManager::sendPendingMessages()
{
	m_workersMutex.lock();

	for( auto it = m_workers.begin(); it != m_workers.end(); ++it )
	{
		auto& worker = it.value();

		while( worker.socket && worker.pendingMessages.isEmpty() == false )
		{
			worker.pendingMessages.first().send( worker.socket );
			worker.pendingMessages.removeFirst();
		}
	}

	m_workersMutex.unlock();
}
