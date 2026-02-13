/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServiceFunctions class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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
#include <QTimer>

#include "LinuxPlatformConfiguration.h"
#include "LinuxServerProcess.h"
#include "LinuxServiceCore.h"
#include "LinuxSessionFunctions.h"
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
	startServers();

	QEventLoop eventLoop;
	eventLoop.exec();
}



void LinuxServiceCore::startServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath )
{
	Q_UNUSED(login1SessionId)

	const auto sessionPath = sessionObjectPath.path();

	vDebug() << "new session" << sessionPath;

	startServer( sessionPath );
}



void LinuxServiceCore::stopServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath )
{
	Q_UNUSED(login1SessionId)

	const auto sessionPath = sessionObjectPath.path();

	vDebug() << "session removed" << sessionPath;

	if( m_serverProcesses.contains( sessionPath ) )
	{
		stopServer( sessionPath );

		// make sure to (re-)start server instances for preempted/suspended sessions such as the login manager session
		if( m_sessionManager.mode() != PlatformSessionManager::Mode::Multi )
		{
			startServers();
		}
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



void LinuxServiceCore::startServers()
{
	vDebug();

	const auto sessions = LinuxSessionFunctions::listSessions();

	for( const auto& s : sessions )
	{
		if( m_serverProcesses.contains( s ) == false &&
			m_deferredServerSessions.contains( s ) == false &&
			( m_sessionManager.mode() == PlatformSessionManager::Mode::Multi || m_serverProcesses.isEmpty() ) )
		{
			startServer( s );
		}
	}
}



void LinuxServiceCore::startServer( const QString& sessionPath )
{
	const auto sessionType = LinuxSessionFunctions::getSessionType( sessionPath );

	if( sessionType == LinuxSessionFunctions::Type::Wayland )
	{
		vWarning() << "Wayland session detected but trying to start Veyon Server anyway, even though Veyon Server does "
					  "not supported Wayland sessions. If you encounter problems, please switch to X11-based sessions!";
	}

	// do not start server for non-graphical sessions
	if( sessionType == LinuxSessionFunctions::Type::TTY )
	{
		vDebug() << "Not starting Veyon Server in TTY session";
		return;
	}

	// do not start server for sessions with unspecified type
	if (sessionType == LinuxSessionFunctions::Type::Unspecified)
	{
		vDebug() << "Not starting Veyon Server in a session with unspecified type";
		return;
	}

	const auto sessionState = LinuxSessionFunctions::getSessionState( sessionPath );
	if( sessionState == LinuxSessionFunctions::State::Opening )
	{
		vDebug() << "Session" << sessionPath << "still is being opening - retrying in" << SessionStateProbingInterval << "msecs";
		deferServerStart( sessionPath, SessionStateProbingInterval );
		return;
	}

	// only start server for online or active sessions
	if( sessionState != LinuxSessionFunctions::State::Online &&
		sessionState != LinuxSessionFunctions::State::Active )
	{
		vInfo() << "Not starting server for session" << sessionPath << "in state" << sessionState;
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
		deferServerStart( sessionPath, SessionEnvironmentProbingInterval );
		return;
	}

	if( m_sessionManager.mode() != PlatformSessionManager::Mode::Multi )
	{
		// make sure no other server is still running
		stopAllServers();
	}

	const auto sessionUptime = LinuxSessionFunctions::getSessionUptimeSeconds( sessionPath );
	const auto minimumSessionUptime = LinuxPlatformConfiguration(&VeyonCore::config()).minimumUserSessionLifetime();

	if( sessionUptime >= 0 &&
		sessionUptime < minimumSessionUptime )
	{
		vDebug() << "Session" << sessionPath << "too young - retrying in" << minimumSessionUptime - sessionUptime << "msecs";
		deferServerStart( sessionPath, int(minimumSessionUptime - sessionUptime) );
		return;
	}

	sessionEnvironment.insert( LinuxSessionFunctions::sessionPathEnvVarName(), sessionPath );

	// if pam-systemd is not in use, we have to set the XDG_SESSION_ID environment variable manually
	if( sessionEnvironment.contains( LinuxSessionFunctions::xdgSessionIdEnvVarName() ) == false )
	{
		const auto sessionId = LinuxSessionFunctions::getSessionId(sessionPath);
		if (sessionId.isEmpty() == false)
		{
			sessionEnvironment.insert(LinuxSessionFunctions::xdgSessionIdEnvVarName(), sessionId);
		}
	}

	// workaround for #817 where LinuxSessionFunctions::getSessionEnvironment() does not return all
	// environment variables when executed via systemd for an established KDE session and xdg-open fails
	if (sessionEnvironment.value(LinuxSessionFunctions::xdgCurrentDesktopEnvVarName()) == QLatin1String("KDE") &&
		sessionEnvironment.contains(LinuxSessionFunctions::kdeSessionVersionEnvVarName()) == false)
	{
		sessionEnvironment.insert(LinuxSessionFunctions::xdgCurrentDesktopEnvVarName(), QStringLiteral("X-Generic"));
	}

	const auto sessionId = m_sessionManager.openSession( sessionPath );

	vInfo() << "Starting server for new session" << sessionPath
			<< "with ID" << sessionId
			<< "at seat" << LinuxSessionFunctions::getSessionSeat( sessionPath ).path;

	sessionEnvironment.insert( QLatin1String( ServiceDataManager::serviceDataTokenEnvironmentVariable() ),
							   QString::fromUtf8( m_dataManager.token().toByteArray() ) );

	auto serverProcess = new LinuxServerProcess( sessionEnvironment, sessionPath, sessionId, this );
	serverProcess->start();

	connect(serverProcess, &QProcess::stateChanged, this, [=, this]() { checkSessionState(sessionPath); });

	m_serverProcesses[sessionPath] = serverProcess;
	m_deferredServerSessions.removeAll( sessionPath );
}



void LinuxServiceCore::deferServerStart( const QString& sessionPath, int delay )
{
	QTimer::singleShot(delay, this, [=, this]() { startServer(sessionPath); });

	if( m_deferredServerSessions.contains( sessionPath ) == false )
	{
		m_deferredServerSessions.append( sessionPath );
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

	auto serverProcess = std::as_const(m_serverProcesses)[sessionPath];
	serverProcess->disconnect(this);
	serverProcess->stop();
	serverProcess->deleteLater();

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
	const auto sessionState = LinuxSessionFunctions::getSessionState( sessionPath );
	if( sessionState == LinuxSessionFunctions::State::Closing ||
		sessionState == LinuxSessionFunctions::State::Unknown )
	{
		vDebug() << "Stopping server for currently closing session" << sessionPath;
		stopServer( sessionPath );
	}
	else
	{
		// restart server if crashed
		const auto serverProcess = m_serverProcesses.value(sessionPath);
		if (serverProcess && serverProcess->state() == QProcess::NotRunning)
		{
			QTimer::singleShot(ServerRestartInterval, serverProcess, [serverProcess]() { serverProcess->start(); });
		}
	}
}
