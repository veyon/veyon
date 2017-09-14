/*
 * FeatureWorkerManager.cpp - class for managing feature worker instances
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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
#include "LocalSystem.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

// clazy:excludeall=detaching-member

FeatureWorkerManager::FeatureWorkerManager( FeatureManager& featureManager, QObject* parent ) :
	QObject( parent ),
	m_featureManager( featureManager ),
	m_tcpServer( this )
{
	connect( &m_tcpServer, &QTcpServer::newConnection,
			 this, &FeatureWorkerManager::acceptConnection );

	if( !m_tcpServer.listen( QHostAddress::LocalHost, VeyonCore::config().featureWorkerManagerPort() ) )
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



QString FeatureWorkerManager::workerProcessFilePath()
{
	QString path = QCoreApplication::applicationDirPath() + QDir::separator() + "veyon-worker";
#ifdef VEYON_BUILD_WIN32
	path += ".exe";
#endif
	return QDTNS( path );
}



void FeatureWorkerManager::startWorker( const Feature& feature )
{
	if( thread() != QThread::currentThread() )
	{
		QMetaObject::invokeMethod( this, "startWorker", Qt::BlockingQueuedConnection,
								   Q_ARG( Feature, feature ) );
		return;
	}

	stopWorker( feature );

	Worker worker;

	worker.process = new QProcess( this );
	worker.process->setProcessChannelMode( QProcess::ForwardedChannels );

	connect( worker.process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			 worker.process, &QProcess::deleteLater );

	qDebug() << "Starting worker for feature" << feature.displayName() << feature.uid();
	worker.process->start( workerProcessFilePath(),	{ feature.uid().toString(),
													  QString::number( VeyonCore::config().featureWorkerManagerPort() ) } );

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
		if( m_workers[message.featureUid()].socket == nullptr )
		{
			m_workers[message.featureUid()].socket = socket;
		}

		m_workersMutex.unlock();

		if( message.command() >= 0 )
		{
			m_featureManager.handleServiceFeatureMessage( message, *this );
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
