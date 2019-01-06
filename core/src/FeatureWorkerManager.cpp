/*
 * FeatureWorkerManager.cpp - class for managing feature worker instances
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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
	m_tcpServer( this )
{
	connect( &m_tcpServer, &QTcpServer::newConnection,
			 this, &FeatureWorkerManager::acceptConnection );

	if( !m_tcpServer.listen( QHostAddress::LocalHost,
							 static_cast<quint16>( VeyonCore::config().featureWorkerManagerPort() + VeyonCore::sessionId() ) ) )
	{
		qCritical( "FeatureWorkerManager: can't listen on localhost!" );
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



void FeatureWorkerManager::startWorker( const Feature& feature, WorkerProcessMode workerProcessMode )
{
	if( thread() != QThread::currentThread() )
	{
		QMetaObject::invokeMethod( this, "startWorker", Qt::BlockingQueuedConnection,
								   Q_ARG( Feature, feature ) );
		return;
	}

	stopWorker( feature );

	const auto featureUid = feature.uid().toString();

	Worker worker;

	if( workerProcessMode == ManagedSystemProcess )
	{
		worker.process = new QProcess;
		worker.process->setProcessChannelMode( QProcess::ForwardedChannels );

		connect( worker.process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
				 worker.process, &QProcess::deleteLater );

		qDebug() << "Starting worker (managed system process) for feature" << feature.displayName() << featureUid;
		worker.process->start( VeyonCore::filesystem().workerFilePath(), { featureUid } );
	}
	else
	{
		qDebug() << "Starting worker (unmanaged session process) for feature" << feature.displayName() << featureUid;
		VeyonCore::platform().coreFunctions().runProgramAsUser( VeyonCore::filesystem().workerFilePath(), { featureUid },
																VeyonCore::platform().userFunctions().currentUser(),
																VeyonCore::platform().coreFunctions().activeDesktopName() );
	}

	m_workersMutex.lock();
	m_workers[feature.uid()] = worker;
	m_workersMutex.unlock();
}



void FeatureWorkerManager::stopWorker( const Feature &feature )
{
	if( thread() != QThread::currentThread() )
	{
		QMetaObject::invokeMethod( this, "stopWorker", Qt::BlockingQueuedConnection,
								   Q_ARG( Feature, feature ) );
		return;
	}

	m_workersMutex.lock();

	if( m_workers.contains( feature.uid() ) )
	{
		qDebug() << "Stopping worker for feature" << feature.displayName() << feature.uid();

		auto& worker = m_workers[feature.uid()];

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

		m_workers.remove( feature.uid() );
	}

	m_workersMutex.unlock();
}



void FeatureWorkerManager::sendMessage( const FeatureMessage& message )
{
	m_workersMutex.lock();

	if( m_workers.contains( message.featureUid() ) )
	{
		m_workers[message.featureUid()].pendingMessages.append( message );
	}

	m_workersMutex.unlock();
}



bool FeatureWorkerManager::isWorkerRunning( const Feature& feature )
{
	QMutexLocker locker( &m_workersMutex );
	return m_workers.contains( feature.uid() );
}



FeatureUidList FeatureWorkerManager::runningWorkers()
{
	QMutexLocker locker( &m_workersMutex );

	FeatureUidList featureUidList;
	featureUidList.reserve( m_workers.size() );

	for( auto it = m_workers.begin(); it != m_workers.end(); ++it )
	{
		featureUidList.append( it.key().toString() );
	}

	return featureUidList;
}



void FeatureWorkerManager::acceptConnection()
{
	qDebug( "FeatureWorkerManager: accepting connection" );

	QTcpSocket* socket = m_tcpServer.nextPendingConnection();

	// connect to readyRead() signal of new connection
	connect( socket, &QTcpSocket::readyRead,
			 this, [=] () { processConnection( socket ); } );

	connect( socket, &QTcpSocket::disconnected,
			 this, [=] () { closeConnection( socket ); } );
}



void FeatureWorkerManager::processConnection( QTcpSocket* socket )
{
	FeatureMessage message( socket );
	message.receive();

	m_workersMutex.lock();

	// set socket information
	if( m_workers.contains( message.featureUid() ) )
	{
		if( m_workers[message.featureUid()].socket.isNull() )
		{
			m_workers[message.featureUid()].socket = socket;
		}

		m_workersMutex.unlock();

		if( message.command() >= 0 )
		{
			m_featureManager.handleFeatureMessage( m_server, message );
		}

	}
	else
	{
		m_workersMutex.unlock();

		qCritical() << "FeatureWorkerManager: got data from non-existing worker!" << message.featureUid();
	}
}



void FeatureWorkerManager::closeConnection( QTcpSocket* socket )
{
	m_workersMutex.lock();

	for( auto it = m_workers.begin(); it != m_workers.end(); )
	{
		if( it.value().socket == socket )
		{
			qDebug() << "FeatureWorkerManager::closeConnection(): removing worker after socket has been closed";
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
