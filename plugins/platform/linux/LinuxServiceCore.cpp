/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QDBusReply>
#include <QEventLoop>
#include <QProcess>
#include <QTimer>

#include <csignal>
#include <sys/types.h>

#include "Filesystem.h"
#include "LinuxCoreFunctions.h"
#include "LinuxServiceCore.h"
#include "LinuxSessionFunctions.h"
#include "ProcessHelper.h"


LinuxServiceCore::LinuxServiceCore( QObject* parent ) :
	QObject( parent )
{
	connectToLoginManager();
}



LinuxServiceCore::~LinuxServiceCore()
{
	stopAllServers();
}



void LinuxServiceCore::run()
{
	const auto sessions = listSessions();

	for( const auto& s : sessions )
	{
		startServer( s, QDBusObjectPath( s ) );
	}

	QEventLoop eventLoop;
	eventLoop.exec();
}



void LinuxServiceCore::startServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath )
{
	const auto sessionPath = sessionObjectPath.path();

	const auto sessionType = LinuxSessionFunctions::getSessionType( sessionPath );

	if( sessionType == QLatin1String("wayland") )
	{
		vCritical() << "Can't start Veyon Server in Wayland sessions as this is not yet supported. Please switch to X11-based sessions!";
		return;
	}

	// do not start server for non-graphical sessions
	if( sessionType != QLatin1String("x11") )
	{
		return;
	}

	// only start server for online or active sessions
	const auto sessionState = LinuxSessionFunctions::getSessionState( sessionPath );
	if( sessionState != LinuxSessionFunctions::State::Online &&
		sessionState != LinuxSessionFunctions::State::Active )
	{
		return;
	}

	const auto sessionLeader = LinuxSessionFunctions::getSessionLeaderPid( sessionPath );
	if( sessionLeader < 0 )
	{
		vCritical() << "No leader available for session" << sessionPath;
		return;
	}

	auto sessionEnvironment = LinuxSessionFunctions::getSessionEnvironment( sessionLeader );

	if( sessionEnvironment.isEmpty() )
	{
		vWarning() << "Environment for session" << sessionPath << "not yet available - retrying in"
				   << SessionEnvironmentProbingInterval << "msecs";
		QTimer::singleShot( SessionEnvironmentProbingInterval, this,
							[=]() { startServer( login1SessionId, sessionObjectPath ); } );
		return;
	}

	if( m_sessionManager.multiSession() == false && m_serverProcesses.isEmpty() == false )
	{
		// make sure no other server is still running
		stopAllServers();
	}

	const auto sessionUptime = LinuxSessionFunctions::getSessionUptimeSeconds( sessionPath );

	if( sessionUptime >= 0 && sessionUptime < SessionUptimeSecondsMinimum )
	{
		vDebug() << "Session" << sessionPath << "too young - retrying in" << SessionUptimeProbingInterval << "msecs";
		QTimer::singleShot( SessionUptimeProbingInterval, this, [=]() { startServer( login1SessionId, sessionObjectPath ); } );
		return;
	}

	const auto seat = LinuxSessionFunctions::getSessionSeat( sessionPath );

	const auto veyonSessionId = m_sessionManager.openSession( LinuxSessionFunctions::getSessionId( sessionPath ) );

	vInfo() << "Starting server for new session" << sessionPath
			<< "with ID" << veyonSessionId
			<< "at seat" << seat.path;

	sessionEnvironment.insert( QLatin1String( ServiceDataManager::serviceDataTokenEnvironmentVariable() ),
							   QString::fromUtf8( m_dataManager.token().toByteArray() ) );

	auto process = new QProcess( this );
	process->setProcessEnvironment( sessionEnvironment );
	process->start( VeyonCore::filesystem().serverFilePath(), QStringList{} );

	m_serverProcesses[sessionPath] = process;
}



void LinuxServiceCore::stopServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath )
{
	Q_UNUSED(login1SessionId)

	const auto sessionPath = sessionObjectPath.path();

	if( m_serverProcesses.contains( sessionPath ) )
	{
		stopServer( sessionPath );
	}
}



void LinuxServiceCore::connectToLoginManager()
{
	bool success = true;

	const auto service = m_loginManager->service();
	const auto path = m_loginManager->path();
	const auto interface = m_loginManager->interface();

	success &= QDBusConnection::systemBus().connect( service, path, interface, QStringLiteral("SessionNew"),
													 this, SLOT(startServer(QString,QDBusObjectPath)) );

	success &= QDBusConnection::systemBus().connect( service, path, interface, QStringLiteral("SessionRemoved"),
													 this, SLOT(stopServer(QString,QDBusObjectPath)) );

	if( success == false )
	{
		vWarning() << "could not connect to login manager! retrying in" << LoginManagerReconnectInterval << "msecs";
		QTimer::singleShot( LoginManagerReconnectInterval, this, &LinuxServiceCore::connectToLoginManager );
	}
	else
	{
		vDebug() << "connected to login manager";
	}
}



void LinuxServiceCore::stopServer( const QString& sessionPath )
{
	m_sessionManager.closeSession( LinuxSessionFunctions::getSessionId( sessionPath ) );

	if( m_serverProcesses.contains( sessionPath ) == false )
	{
		return;
	}

	vInfo() << "stopping server for removed session" << sessionPath;

	auto process = qAsConst(m_serverProcesses)[sessionPath];

	// tell x11vnc to shutdown
	kill( pid_t(process->processId()), SIGINT );

	if( ProcessHelper::waitForProcess( process, ServerShutdownTimeout, ServerWaitSleepInterval ) == false )
	{
		process->terminate();

		if( ProcessHelper::waitForProcess( process, ServerTerminateTimeout, ServerWaitSleepInterval ) == false )
		{
			vWarning() << "server for session" << sessionPath << "still running - killing now";
			process->kill();
			ProcessHelper::waitForProcess( process, ServerKillTimeout, ServerWaitSleepInterval );
		}
	}

	delete process;
	m_serverProcesses.remove( sessionPath );
}



void LinuxServiceCore::stopAllServers()
{
	while( m_serverProcesses.isEmpty() == false )
	{
		stopServer( m_serverProcesses.firstKey() );
	}
}



QStringList LinuxServiceCore::listSessions()
{
	QStringList sessions;

	const QDBusReply<QDBusArgument> reply = m_loginManager->call( QStringLiteral("ListSessions") );

	if( reply.isValid() )
	{
		const auto data = reply.value();

		data.beginArray();
		while( data.atEnd() == false )
		{
			LinuxSessionFunctions::LoginDBusSession session;

			data.beginStructure();
			data >> session.id >> session.uid >> session.name >> session.seatId >> session.path;
			data.endStructure();

			sessions.append( session.path.path() );
		}
		return sessions;
	}

	vCritical() << "Could not query sessions:" << reply.error().message();

	return sessions;
}

