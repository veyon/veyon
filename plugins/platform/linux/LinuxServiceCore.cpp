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
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>

#include "LinuxPlatformConfiguration.h"
#include "LinuxServerProcess.h"
#include "LinuxServiceCore.h"
#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"
#include "VeyonConfiguration.h"


static const QStringList safeSessionEnvironmentVariables = {
	// Session / login identity
	QStringLiteral("XDG_SESSION_ID"),
	QStringLiteral("XDG_SESSION_CLASS"),
	QStringLiteral("XDG_SESSION_TYPE"),
	QStringLiteral("XDG_SEAT"),
	QStringLiteral("HOME"),
	QStringLiteral("USER"),
	QStringLiteral("LOGNAME"),
	QStringLiteral("SHELL"),

	// Desktop/session identification
	QStringLiteral("XDG_CURRENT_DESKTOP"),
	QStringLiteral("XDG_SESSION_DESKTOP"),
	QStringLiteral("DESKTOP_SESSION"),
	QStringLiteral("KDE_SESSION_VERSION"),
	QStringLiteral("KDE_FULL_SESSION"),
	QStringLiteral("GNOME_DESKTOP_SESSION_ID"),
	QStringLiteral("GDMSESSION"),

	// Display server (X11)
	QStringLiteral("DISPLAY"),
	QStringLiteral("XAUTHORITY"),

	// Display server (Wayland)
	QStringLiteral("WAYLAND_DISPLAY"),
	QStringLiteral("XDG_RUNTIME_DIR"),

	// IPC / bus infrastructure
	QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
	QStringLiteral("DBUS_STARTER_ADDRESS"),

	// XDG base directories
	QStringLiteral("XDG_DATA_HOME"),
	QStringLiteral("XDG_CONFIG_HOME"),
	QStringLiteral("XDG_CACHE_HOME"),
	QStringLiteral("XDG_STATE_HOME"),
	QStringLiteral("XDG_DATA_DIRS"),
	QStringLiteral("XDG_CONFIG_DIRS"),

	// Locale / internationalization
	QStringLiteral("LANG"),
	QStringLiteral("LANGUAGE"),
	QStringLiteral("LC_ALL"),
	QStringLiteral("LC_CTYPE"),
	QStringLiteral("LC_NUMERIC"),
	QStringLiteral("LC_TIME"),
	QStringLiteral("LC_COLLATE"),
	QStringLiteral("LC_MONETARY"),
	QStringLiteral("LC_MESSAGES"),
	QStringLiteral("LC_PAPER"),
	QStringLiteral("LC_NAME"),
	QStringLiteral("LC_ADDRESS"),
	QStringLiteral("LC_TELEPHONE"),
	QStringLiteral("LC_MEASUREMENT"),
	QStringLiteral("LC_IDENTIFICATION"),

	// Toolkit-specific
	QStringLiteral("QT_QPA_PLATFORM"),
	QStringLiteral("QT_QPA_PLATFORMTHEME"),
	QStringLiteral("QT_STYLE_OVERRIDE"),
	QStringLiteral("QT_SCALE_FACTOR"),
	QStringLiteral("QT_AUTO_SCREEN_SCALE_FACTOR"),
	QStringLiteral("GTK_THEME"),
	QStringLiteral("GDK_BACKEND"),
	QStringLiteral("GDK_SCALE"),
	QStringLiteral("GDK_DPI_SCALE"),

	// Audio
	QStringLiteral("PULSE_SERVER"),
	QStringLiteral("PULSE_COOKIE"),

	// Misc desktop-integration
	QStringLiteral("_LXSESSION_PID"),
	QStringLiteral("XDG_SESSION_COOKIE"),
	QStringLiteral("SSH_AUTH_SOCK"),
};


static bool isValidLocaleValue(const QString& value)
{
	// Reject empty or overly long values (defense against parser bugs / DoS)
	if (value.isEmpty() || value.length() > 64)
	{
		return false;
	}

	// Valid POSIX locale names look like: en_US.UTF-8, de_DE, C, C.UTF-8, POSIX
	// Also allow modifiers e.g. en_US.UTF-8@euro and multi-value LANGUAGE lists (en_US:de_DE)
	static const QRegularExpression validLocalePattern(QStringLiteral(R"(^[A-Za-z0-9_.@:\-]+$)"));

	if (validLocalePattern.match(value).hasMatch() == false)
	{
		return false;
	}

	// Explicitly reject path traversal / injection characters just in case
	if (value.contains(QLatin1Char('/')) ||
		value.contains(QLatin1Char('\\')) ||
		value.contains(QLatin1Char('\n')) ||
		value.contains(QLatin1Char('\r')) ||
		value.contains(QLatin1Char('\0')))
	{
		return false;
	}

	return true;
}


static QProcessEnvironment sessionToServerEnvironment(const QProcessEnvironment& sessionEnvironment)
{
	QProcessEnvironment serverEnvironment;

	for (const auto& key : sessionEnvironment.keys())
	{
		if (safeSessionEnvironmentVariables.contains(key) == false)
		{
			continue;
		}

		const auto value = sessionEnvironment.value(key);

		// additionally validate LC_*/LANG/LANGUAGE values against the expected locale format
		if ((key == QLatin1String("LANGUAGE") ||
			 key.startsWith(QLatin1String("LC_")) ||
			 key == QLatin1String("LANG")) &&
			isValidLocaleValue(value) == false)
		{
			vWarning() << "discarding suspicious locale value for" << key;
			continue;
		}

		serverEnvironment.insert(key, value);
	}

	serverEnvironment.insert(QStringLiteral("PATH"), QStringLiteral("/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"));
	return serverEnvironment;
}



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

	const auto sessionEnvironment = LinuxSessionFunctions::getSessionEnvironment(sessionLeader);
	if (sessionEnvironment.isEmpty())
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

	auto serverEnvironment = sessionToServerEnvironment(sessionEnvironment);
	serverEnvironment.insert(LinuxSessionFunctions::sessionPathEnvVarName(), sessionPath);

	// if pam-systemd is not in use, we have to set the XDG_SESSION_ID environment variable manually
	if (serverEnvironment.contains(LinuxSessionFunctions::xdgSessionIdEnvVarName()) == false)
	{
		const auto sessionId = LinuxSessionFunctions::getSessionId(sessionPath);
		if (sessionId.isEmpty() == false)
		{
			serverEnvironment.insert(LinuxSessionFunctions::xdgSessionIdEnvVarName(), sessionId);
		}
	}

	// workaround for #817 where LinuxSessionFunctions::getSessionEnvironment() does not return all
	// environment variables when executed via systemd for an established KDE session and xdg-open fails
	if (sessionType != LinuxSessionFunctions::Type::Wayland &&
		serverEnvironment.value(LinuxSessionFunctions::xdgCurrentDesktopEnvVarName()) == QLatin1String("KDE") &&
		serverEnvironment.contains(LinuxSessionFunctions::kdeSessionVersionEnvVarName()) == false)
	{
		serverEnvironment.insert(LinuxSessionFunctions::xdgCurrentDesktopEnvVarName(), QStringLiteral("X-Generic"));
	}

	// Wayland: inject critical environment variables from well-known paths
	// if they were not captured from the session leader's process tree.
	// This ensures DBUS_SESSION_BUS_ADDRESS, XDG_RUNTIME_DIR and
	// WAYLAND_DISPLAY are always available for the portal-based screen
	// capture even if /proc/<pid>/environ could not provide them.
	if (sessionType == LinuxSessionFunctions::Type::Wayland)
	{
		const auto sessionUserPath = LinuxSessionFunctions::getSessionUser(sessionPath);
		if (sessionUserPath.isEmpty() == false)
		{
			const auto uid = LinuxUserFunctions::getUserProperty(
								 sessionUserPath, QStringLiteral("UID")).toUInt();

			if (uid > 0)
			{
				if (serverEnvironment.contains(QStringLiteral("DBUS_SESSION_BUS_ADDRESS")) == false)
				{
					serverEnvironment.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
											 QStringLiteral("unix:path=/run/user/%1/bus").arg(uid));
					vDebug() << "Wayland: injected DBUS_SESSION_BUS_ADDRESS from known path";
				}

				if (serverEnvironment.contains(QStringLiteral("XDG_RUNTIME_DIR")) == false)
				{
					serverEnvironment.insert(QStringLiteral("XDG_RUNTIME_DIR"),
											 QStringLiteral("/run/user/%1").arg(uid));
				}

				if (serverEnvironment.contains(QStringLiteral("WAYLAND_DISPLAY")) == false)
				{
					QDir dir(QStringLiteral("/run/user/%1").arg(uid));
					const auto sockets = dir.entryList({QStringLiteral("wayland-*")}, QDir::System);
					if (sockets.isEmpty() == false)
					{
						serverEnvironment.insert(QStringLiteral("WAYLAND_DISPLAY"), sockets.first());
					}
				}
			}
		}

		// Warn about any remaining missing critical variables
		static const QStringList criticalVars = {
			QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
			QStringLiteral("WAYLAND_DISPLAY"),
			QStringLiteral("XDG_RUNTIME_DIR"),
		};
		for (const auto& var : criticalVars)
		{
			if (serverEnvironment.contains(var) == false)
			{
				vWarning() << "Wayland session:" << var
						   << "not available - Portal/RemoteDesktop screen capture may not work";
			}
		}
	}

	const auto sessionId = m_sessionManager.openSession( sessionPath );

	vInfo() << "Starting server for new session" << sessionPath
			<< "with ID" << sessionId
			<< "at seat" << LinuxSessionFunctions::getSessionSeat( sessionPath ).path;

	serverEnvironment.insert(QLatin1String(ServiceDataManager::serviceDataTokenEnvironmentVariable()),
							 QString::fromUtf8(m_dataManager.token().toByteArray()));

	auto serverProcess = new LinuxServerProcess(serverEnvironment, sessionPath, sessionId, sessionType, this);
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
