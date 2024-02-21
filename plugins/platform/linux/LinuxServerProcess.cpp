/*
 * LinuxServiceFunctions.cpp - implementation of LinuxServerProcess class
 *
 * Copyright (c) 2021-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QFileInfo>

#include <csignal>
#ifdef HAVE_LIBPROCPS
#include <proc/readproc.h>
#endif
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Filesystem.h"
#include "LinuxCoreFunctions.h"
#include "LinuxServerProcess.h"
#include "VeyonConfiguration.h"


LinuxServerProcess::LinuxServerProcess( const QProcessEnvironment& processEnvironment,
										const QString& sessionPath, int sessionId, QObject* parent ) :
	QProcess( parent ),
	m_sessionPath( sessionPath ),
	m_sessionId( sessionId )
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
			LinuxCoreFunctions::forEachChildProcess([=](const pids_stack* stack, const pids_info* info)
			{
				Q_UNUSED(info)
				const pid_t tid = PIDS_VAL(0, s_int, stack, info);
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
