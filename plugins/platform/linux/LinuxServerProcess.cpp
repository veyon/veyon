/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServerProcess class
 *
 * Copyright (c) 2021-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QDBusConnectionInterface>
#include <QDir>
#include <QFileInfo>

#include <csignal>
#include <grp.h>
#include <pwd.h>
#ifdef HAVE_LIBPROCPS
#include <proc/readproc.h>
#endif
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Filesystem.h"
#include "LinuxCoreFunctions.h"
#include "LinuxServerProcess.h"
#include "LinuxUserFunctions.h"
#include "VeyonConfiguration.h"


LinuxServerProcess::LinuxServerProcess(const QProcessEnvironment& processEnvironment,
									   const QString& sessionPath, int sessionId,
									   LinuxSessionFunctions::Type sessionType,
									   QObject* parent) :
	QProcess(parent),
	m_sessionPath(sessionPath),
	m_sessionId(sessionId),
	m_sessionType(sessionType)
{
	setProcessEnvironment( processEnvironment );
}



LinuxServerProcess::~LinuxServerProcess()
{
	stop();
}



void LinuxServerProcess::start()
{
	if( VeyonCore::config().logToSystem() )
	{
		setProcessChannelMode( QProcess::ForwardedChannels );
	}

	const auto catchsegv{ QStringLiteral("/usr/bin/catchsegv") };
	if( qEnvironmentVariableIsSet("VEYON_VALGRIND_SERVERS") )
	{
		QProcess::start( QStringLiteral("/usr/bin/valgrind"),
			   { QStringLiteral("--error-limit=no"),
				 QStringLiteral("--log-file=valgrind-veyon-server-%1.log").arg(m_sessionId),
				 VeyonCore::filesystem().serverFilePath() } );
	}
	else if( VeyonCore::isDebugging() && QFileInfo::exists( catchsegv ) )
	{
		QProcess::start( catchsegv, { VeyonCore::filesystem().serverFilePath() } );
	}
	else if (m_sessionType == LinuxSessionFunctions::Type::Wayland)
	{
		const auto sessionUserPath = LinuxSessionFunctions::getSessionUser(m_sessionPath);
		const auto sessionUserName = LinuxUserFunctions::getUserProperty(sessionUserPath, QStringLiteral("Name")).toString();
		if (sessionUserName.isEmpty())
		{
			vCritical() << "failed to determine user name of session user" << sessionUserPath;
		}

		m_sessionUserId = LinuxUserFunctions::userIdFromName(sessionUserName);

		if (m_sessionUserId == LinuxUserFunctions::InvalidUserId)
		{
			vCritical() << "failed to determine user ID of user" << sessionUserName;
		}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		setChildProcessModifier([this] { setProcessUserId(); });
#endif

		// Check for xdg-desktop-portal >= 1.20 via the presence of the
		// org.freedesktop.host.portal.Registry D-Bus service. When available,
		// launch veyon-server directly which inherits the full session
		// environment (DBUS_SESSION_BUS_ADDRESS, WAYLAND_DISPLAY, XDG_RUNTIME_DIR)
		// captured from the session leader. On older portal versions fall back
		// to kioclient/gio D-Bus activation.
		const auto portalRegistryService = QStringLiteral("org.freedesktop.host.portal.Registry");
		if(QDBusConnection::sessionBus().interface()->isServiceRegistered(portalRegistryService))
		{
			vDebug() << "xdg-desktop-portal >= 1.20 detected, launching server directly";
			QProcess::start(VeyonCore::filesystem().serverFilePath(), QStringList{});
		}
		else
		{
			vDebug() << "xdg-desktop-portal < 1.20, falling back to D-Bus activation";
			const auto desktopEnvironment = LinuxSessionFunctions::getSessionDesktopEnvironment(m_sessionPath);
			const auto desktopFile = VeyonCore::applicationsDirectory() + QDir::separator()
									 + QStringLiteral("io.veyon.veyon-server.desktop");
			switch(desktopEnvironment)
			{
			case LinuxSessionFunctions::DesktopEnvironment::KDE:
				QProcess::start(QStringLiteral("kioclient"), {QStringLiteral("exec"), desktopFile});
				break;
			case LinuxSessionFunctions::DesktopEnvironment::GNOME:
				QProcess::start(QStringLiteral("gio"), {QStringLiteral("launch"), desktopFile});
				break;
			default:
				vWarning() << "Unsupported desktop environment with Wayland session, launching server directly";
				QProcess::start(VeyonCore::filesystem().serverFilePath(), QStringList{});
				break;
			}
		}
	}
	else
	{
		QProcess::start( VeyonCore::filesystem().serverFilePath(), QStringList{} );
	}
}



void LinuxServerProcess::stop()
{
	const auto sendSignalRecursively = []( pid_t pid, int sig ) {
		if( pid > 0 )
		{
#ifdef HAVE_LIBPROCPS
			LinuxCoreFunctions::forEachChildProcess(
				[=]( proc_t* procInfo ) {
					if( procInfo->tid > 0 && ::kill( procInfo->tid, sig ) < 0 && errno != ESRCH )
					{
						vCritical() << "kill() failed with" << errno;
					}
					return true;
				},
				pid, 0, true );
#elif defined(HAVE_LIBPROC2)
			LinuxCoreFunctions::forEachChildProcess([=](const pids_stack* stack)
			{
				const pid_t tid = PIDS_VAL(0, s_int, stack);
				if (tid > 0 && ::kill(tid, sig) < 0 && errno != ESRCH)
				{
					vCritical() << "kill() failed with" << errno;
				}
				return true;
			},
			pid,
			{}, true);
#endif

			if( ::kill( pid, sig ) < 0 && errno != ESRCH )
			{
				vCritical() << "kill() failed with" << errno;
			}

			// clean up process
			waitpid( pid, nullptr, WNOHANG );
		}
	};

	const auto pid = pid_t(processId());

	// manually set process state since we're managing the process termination on our own
	setProcessState( QProcess::NotRunning );

	// tell x11vnc and child processes (in case spawned via catchsegv) to shutdown
	sendSignalRecursively( pid, SIGINT );

	if( LinuxCoreFunctions::waitForProcess( pid, ServerShutdownTimeout, ServerWaitSleepInterval ) == false )
	{
		sendSignalRecursively( pid, SIGTERM );

		if( LinuxCoreFunctions::waitForProcess( pid, ServerTerminateTimeout, ServerWaitSleepInterval ) == false )
		{
			vWarning() << "server for session" << m_sessionPath << "still running - killing now";
			sendSignalRecursively( pid, SIGKILL );
			LinuxCoreFunctions::waitForProcess( pid, ServerKillTimeout, ServerWaitSleepInterval );
		}
	}
}



void LinuxServerProcess::setProcessUserId()
{
	if (m_sessionUserId != LinuxUserFunctions::InvalidUserId)
	{
		// Look up the user's full credential data before dropping privileges
		const auto pw_entry = getpwuid(m_sessionUserId);
		if (pw_entry != nullptr)
		{
			// Set supplementary groups first (needs EUID=root)
			initgroups(pw_entry->pw_name, pw_entry->pw_gid);
			// Set primary group GID
			setgid(pw_entry->pw_gid);
		}
		// Set UID last — after this the process has no more root privileges
		setuid(m_sessionUserId);
	}
}
