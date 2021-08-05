/*
 * PlatformSessionManager.cpp - implementation of PlatformSessionManager class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMutexLocker>

#include "PlatformSessionManager.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"


PlatformSessionManager::PlatformSessionManager( QObject* parent ) :
	QThread( parent ),
	m_multiSession( VeyonCore::config().multiSessionModeEnabled() ),
	m_maximumSessionCount( VeyonCore::config().maximumSessionCount() )
{
	vDebug();

	start();
}



PlatformSessionManager::~PlatformSessionManager()
{
	vDebug();

	quit();
	wait();
}



void PlatformSessionManager::run()
{
	if( m_multiSession )
	{
		auto server = new QLocalServer;
		server->setSocketOptions( QLocalServer::WorldAccessOption );
		server->listen( serverName() );

		connect( server, &QLocalServer::newConnection, server, [this, server]() {
			auto connection = server->nextPendingConnection();
			connect( connection, &QLocalSocket::disconnected, connection, &QLocalSocket::deleteLater );

			m_mutex.lock();
			VariantArrayMessage(connection).write( m_sessions ).send();
			m_mutex.unlock();

			connection->flush();
			connection->disconnectFromServer();
		} );
	}

	QThread::run();
}

PlatformSessionManager::SessionId PlatformSessionManager::openSession( const PlatformSessionId& platformSessionId )
{
	QMutexLocker l( &m_mutex );

	const auto sessionIds = m_sessions.values();

	for( SessionId i = 0; i < m_maximumSessionCount; ++i )
	{
		if( sessionIds.contains( i ) == false )
		{
			vDebug() << "Opening session" << i << "for platform session" << platformSessionId;
			m_sessions[platformSessionId] = i;

			return i;
		}
	}

	return PlatformSessionFunctions::InvalidSessionId;
}



void PlatformSessionManager::closeSession( const PlatformSessionId& platformSessionId )
{
	QMutexLocker l( &m_mutex );

	const auto closedSessionId = m_sessions.take( platformSessionId ).toInt();

	vDebug() << "Closing session" << closedSessionId << "for platform session" << platformSessionId;
}



PlatformSessionManager::SessionId PlatformSessionManager::resolveSessionId( const PlatformSessionId& platformSessionId )
{
	if( VeyonCore::component() == VeyonCore::Component::Service )
	{
		return PlatformSessionFunctions::DefaultSessionId;
	}

	QLocalSocket socket;
	socket.connectToServer( serverName(), QLocalSocket::ReadOnly );
	if( socket.waitForConnected( ServerConnectTimeout ) == false )
	{
		if( VeyonCore::component() != VeyonCore::Component::CLI &&
			VeyonCore::component() != VeyonCore::Component::Configurator )
		{
			vCritical() << "could not read session map";
		}

		return PlatformSessionFunctions::InvalidSessionId;
	}

	if( waitForMessage( &socket ) )
	{
		VariantArrayMessage message( &socket );
		message.receive();
		return message.read().toMap().value( platformSessionId, PlatformSessionFunctions::InvalidSessionId ).toInt();
	}

	vCritical() << "could not receive session map";

	return PlatformSessionFunctions::InvalidSessionId;
}



bool PlatformSessionManager::waitForMessage(QLocalSocket* socket)
{
	// wait for acknowledge
	QElapsedTimer messageTimeoutTimer;
	messageTimeoutTimer.start();

	VariantArrayMessage inMessage( socket );
	while( messageTimeoutTimer.elapsed() < MessageReadTimeout &&
		   inMessage.isReadyForReceive() == false )
	{
		socket->waitForReadyRead( SocketWaitTimeout );
	}

	return inMessage.isReadyForReceive();
}
