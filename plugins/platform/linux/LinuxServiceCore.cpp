/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
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

#include <QDBusReply>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>

#include <csignal>
#include <proc/readproc.h>
#include <sys/types.h>

#include "Filesystem.h"
#include "LinuxCoreFunctions.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxServiceCore.h"
#include "LinuxSessionFunctions.h"
#include "ProcessHelper.h"
#include "VeyonConfiguration.h"


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
	const auto sessions = LinuxSessionFunctions::listSessions();

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

	if( sessionType == LinuxSessionFunctions::Type::Wayland )
	{
		vCritical() << "Can't start Veyon Server in Wayland sessions as this is not yet supported. Please switch to X11-based sessions!";
		return;
	}

	// do not start server for non-graphical sessions
	if( sessionType != LinuxSessionFunctions::Type::X11 )
	{
		return;
	}

	const auto sessionState = LinuxSessionFunctions::getSessionState( sessionPath );
	if( sessionState == LinuxSessionFunctions::State::Opening )
	{
		vDebug() << "Session" << sessionPath << "still opening - retrying in" << SessionStateProbingInterval << "msecs";
		QTimer::singleShot( SessionStateProbingInterval, this, [=]() { startServer( login1SessionId, sessionObjectPath ); } );
		return;
	}

	// only start server for online or active sessions
	if( sessionState != LinuxSessionFunctions::State::Online &&
		sessionState != LinuxSessionFunctions::State::Active )
	{
		vDebug() << "Not starting server for session" << sessionPath << "in state" << sessionState;
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

	if( m_sessionManager.multiSession() == false )
	{
		// make sure no other server is still running
		stopAllServers();
	}

	const auto sessionUptime = LinuxSessionFunctions::getSessionUptimeSeconds( sessionPath );

	if( sessionUptime >= 0 &&
		sessionUptime < LinuxPlatformConfiguration(&VeyonCore::config()).minimumUserSessionLifetime() )
	{
		vDebug() << "Session" << sessionPath << "too young - retrying in" << SessionUptimeProbingInterval << "msecs";
		QTimer::singleShot( SessionUptimeProbingInterval, this, [=]() { startServer( login1SessionId, sessionObjectPath ); } );
		return;
	}

	sessionEnvironment.insert( LinuxSessionFunctions::xdgSessionPathEnvVarName(), sessionPath );

	// if pam-systemd is not in use, we have to set the XDG_SESSION_ID environment variable manually
	if( sessionEnvironment.contains( LinuxSessionFunctions::xdgSessionIdEnvVarName() ) == false )
	{
		sessionEnvironment.insert( LinuxSessionFunctions::xdgSessionIdEnvVarName(),
								   LinuxSessionFunctions::getSessionId( sessionPath ) );
	}

	const auto sessionId = m_sessionManager.openSession( sessionPath );

	vInfo() << "Starting server for new session" << sessionPath
			<< "with ID" << sessionId
			<< "at seat" << LinuxSessionFunctions::getSessionSeat( sessionPath ).path;

	sessionEnvironment.insert( QLatin1String( ServiceDataManager::serviceDataTokenEnvironmentVariable() ),
							   QString::fromUtf8( m_dataManager.token().toByteArray() ) );

	auto process = new QProcess( this );
	process->setProcessEnvironment( sessionEnvironment );

	if( VeyonCore::config().logToSystem() )
	{
		process->setProcessChannelMode( QProcess::ForwardedChannels );
	}

	const auto catchsegv{ QStringLiteral("/usr/bin/catchsegv") };
	if( qEnvironmentVariableIsSet("VEYON_VALGRIND_SERVERS") )
	{
		process->start( QStringLiteral("/usr/bin/valgrind"),
						{ QStringLiteral("--error-limit=no"),
						  QStringLiteral("--log-file=valgrind-veyon-server-%1.log").arg(sessionId),
						  VeyonCore::filesystem().serverFilePath() } );
	}
	else if( VeyonCore::isDebugging() && QFileInfo::exists( catchsegv ) )
	{
		process->start( catchsegv, { VeyonCore::filesystem().serverFilePath() } );
	}
	else
	{
		process->start( VeyonCore::filesystem().serverFilePath(), QStringList{} );
	}

	connect( process, &QProcess::stateChanged, this, [=]() { checkSessionState( sessionPath ); } );

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
	m_sessionManager.closeSession( sessionPath );

	if( m_serverProcesses.contains( sessionPath ) == false )
	{
		return;
	}

	vInfo() << "stopping server for removed session" << sessionPath;

	auto process = qAsConst(m_serverProcesses)[sessionPath];

	const auto sendSignalRecursively = []( int pid, int sig ) {
		if( pid > 0 )
		{
			LinuxCoreFunctions::forEachChildProcess(
				[=]( proc_t* procInfo ) {
					if( procInfo->tid > 0 )
					{
						kill( procInfo->tid, sig );
					}
					return true;
				},
				pid, 0, true );
		}
	};

	const auto pid = process->processId();

	// tell x11vnc and child processes (in case spawned via catchsegv) to shutdown
	sendSignalRecursively( pid, SIGINT );

	if( ProcessHelper::waitForProcess( process, ServerShutdownTimeout, ServerWaitSleepInterval ) == false )
	{
		process->terminate();
		sendSignalRecursively( pid, SIGTERM );

		if( ProcessHelper::waitForProcess( process, ServerTerminateTimeout, ServerWaitSleepInterval ) == false )
		{
			vWarning() << "server for session" << sessionPath << "still running - killing now";
			process->kill();
			sendSignalRecursively( pid, SIGKILL );
			ProcessHelper::waitForProcess( process, ServerKillTimeout, ServerWaitSleepInterval );
		}
	}

	process->deleteLater();
	m_serverProcesses.remove( sessionPath );
}



void LinuxServiceCore::stopAllServers()
{
	while( m_serverProcesses.isEmpty() == false )
	{
		stopServer( m_serverProcesses.firstKey() );
	}
}



void LinuxServiceCore::checkSessionState( const QString& sessionPath )
{
	if( LinuxSessionFunctions::getSessionState( sessionPath ) == LinuxSessionFunctions::State::Closing )
	{
		vDebug() << "Stopping server for currently closing session" << sessionPath;
		stopServer( sessionPath );
	}
}
