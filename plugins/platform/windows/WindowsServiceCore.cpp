/*
 * WindowsServiceCore.cpp - implementation of WindowsServiceCore class
 *
 * Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>

#include "WindowsServiceCore.h"
#include "SasEventListener.h"
#include "WindowsCoreFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsServerProcess.h"
#include "WtsSessionManager.h"


WindowsServiceCore* WindowsServiceCore::s_instance = nullptr;


WindowsServiceCore::WindowsServiceCore( const QString& name,
										const PlatformServiceFunctions::ServiceEntryPoint& serviceEntryPoint ) :
	m_name( WindowsCoreFunctions::toWCharArray( name ) ),
	m_serviceEntryPoint( serviceEntryPoint )
{
	s_instance = this;

	// allocate session 0 (PlatformSessionFunctions::DefaultSessionId) so we can always assign it to the console session
	if( m_sessionManager.mode() != PlatformSessionManager::Mode::Active ||
		( WtsSessionManager::activeSessions().size() <= 1 &&
		  WtsSessionManager::activeConsoleSession() != WtsSessionManager::InvalidSession ) )
	{
		m_sessionManager.openSession( QStringLiteral("0 (console)") );
	}

	// enable privileges required to create process with access token from other process
	WindowsCoreFunctions::enablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, true );
	WindowsCoreFunctions::enablePrivilege( SE_INCREASE_QUOTA_NAME, true );
}



WindowsServiceCore* WindowsServiceCore::instance()
{
	if( s_instance == nullptr )
	{
		vCritical() << "invalid instance!";
	}

	return s_instance;
}



bool WindowsServiceCore::runAsService()
{
	static const std::array<SERVICE_TABLE_ENTRY, 2> dispatchTable = { {
		{ m_name.data(), serviceMainStatic },
		{ nullptr, nullptr }
	} } ;

	WindowsInputDeviceFunctions::checkInterceptionInstallation();

	if( !StartServiceCtrlDispatcher( dispatchTable.data() ) )
	{
		vCritical() << "StartServiceCtrlDispatcher failed.";
		return false;
	}

	return true;
}



void WindowsServiceCore::manageServerInstances()
{
	m_serverShutdownEvent = CreateEvent( nullptr, false, false, L"Global\\SessionEventVeyon" );
	ResetEvent( m_serverShutdownEvent );

	if( m_sessionManager.mode() != PlatformSessionManager::Mode::Local )
	{
		manageServersForAllSessions();
	}
	else
	{
		manageServerForConsoleSession();
	}

	CloseHandle( m_serverShutdownEvent );
}



void WindowsServiceCore::manageServersForAllSessions()
{
	QMap<WtsSessionManager::SessionId, VeyonServerProcess*> serverProcesses;

	const auto activeSessionOnly = m_sessionManager.mode() == PlatformSessionManager::Mode::Active;

	do
	{
		auto wtsSessionIds = WtsSessionManager::activeSessions();

		const auto consoleSessionId = WtsSessionManager::activeConsoleSession();
		if( consoleSessionId != WtsSessionManager::InvalidSession &&
			wtsSessionIds.contains( consoleSessionId ) == false )
		{
			wtsSessionIds.append( consoleSessionId );
		}

		const auto includeConsoleSession = activeSessionOnly &&
										   wtsSessionIds.size() == 1 &&
										   wtsSessionIds.first() == consoleSessionId;
		const auto excludeConsoleSession = activeSessionOnly &&
										   wtsSessionIds.size() > 1;

		if( excludeConsoleSession )
		{
			wtsSessionIds.removeAll( consoleSessionId );
		}

		for( auto it = serverProcesses.begin(); it != serverProcesses.end(); )
		{
			if( wtsSessionIds.contains( it.key() ) == false )
			{
				delete it.value();
				if( it.key() != consoleSessionId || excludeConsoleSession )
				{
					m_sessionManager.closeSession( QString::number(it.key() ) );
				}
				it = serverProcesses.erase( it );
			}
			else
			{
				it.value()->restartIfNotRunning();
				++it;
			}
		}

		for (auto wtsSessionId : std::as_const(wtsSessionIds))
		{
			if( serverProcesses.contains( wtsSessionId ) == false )
			{
				if( wtsSessionId != consoleSessionId || includeConsoleSession )
				{
					m_sessionManager.openSession( QString::number(wtsSessionId) );
				}

				serverProcesses[wtsSessionId] = new VeyonServerProcess(wtsSessionId, m_dataManager.token());
			}
		}

		std::array<HANDLE, 2> events{m_sessionChangeEvent, m_stopServiceEvent};
		WaitForMultipleObjects(events.size(), events.data(), FALSE, SessionPollingInterval);

	} while (m_serviceStopRequested == 0);

	vInfo() << "Service shutdown";

	SetEvent( m_serverShutdownEvent );

	qDeleteAll(serverProcesses);
}



void WindowsServiceCore::manageServerForConsoleSession()
{
	VeyonServerProcess* serverProcess = nullptr;

	auto oldWtsSessionId = WtsSessionManager::InvalidSession;

	do {
		const auto sessionChanged = m_sessionChanged.testAndSetOrdered(1, 0);
		const auto wtsSessionId = WtsSessionManager::activeConsoleSession();

		if( oldWtsSessionId != wtsSessionId || sessionChanged )
		{
			vInfo() << "Session ID changed from" << oldWtsSessionId << "to" << wtsSessionId;

			if( oldWtsSessionId != WtsSessionManager::InvalidSession || sessionChanged )
			{
				// workaround for situations where server is stopped while it is still starting up
				do
				{
					SetEvent( m_serverShutdownEvent );
				}
				while (serverProcess && serverProcess->isRunning() &&
					   serverProcess->uptime() < MinimumServerUptimeTime);

				delete serverProcess;
				serverProcess = nullptr;
			}

			if( wtsSessionId != WtsSessionManager::InvalidSession || sessionChanged )
			{
				delete serverProcess;
				serverProcess = nullptr;

				if (m_serviceStopRequested == 0 &&
					wtsSessionId != WtsSessionManager::InvalidSession)
				{
					serverProcess = new VeyonServerProcess(wtsSessionId, m_dataManager.token());
				}
			}

			oldWtsSessionId = wtsSessionId;
		}
		else if (serverProcess && serverProcess->isRunning() == false)
		{
			qDebug() << "server not running - (re)starting";
			if (wtsSessionId != WtsSessionManager::InvalidSession)
			{
				serverProcess->restartIfNotRunning();
			}

			oldWtsSessionId = wtsSessionId;
		}

		std::array<HANDLE, 2> events{m_sessionChangeEvent, m_stopServiceEvent};
		WaitForMultipleObjects(events.size(), events.data(), FALSE, SessionPollingInterval);

	} while (m_serviceStopRequested == 0);

	vInfo() << "Service shutdown";

	SetEvent( m_serverShutdownEvent );
	delete serverProcess;
}



void WindowsServiceCore::serviceMainStatic( DWORD argc, LPWSTR* argv )
{
	Q_UNUSED(argc)
	Q_UNUSED(argv)

	instance()->serviceMain();
}



DWORD WindowsServiceCore::serviceCtrlStatic( DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context )
{
	return instance()->serviceCtrl( ctrlCode, eventType, eventData, context );
}



void WindowsServiceCore::serviceMain()
{
	DWORD context = 1;

	m_statusHandle = RegisterServiceCtrlHandlerEx( m_name.data(), serviceCtrlStatic, &context );

	if( m_statusHandle == nullptr )
	{
		vCritical() << "Invalid service status handle";
		return;
	}

	memset( &m_status, 0, sizeof( m_status ) );
	m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	if( reportStatus( SERVICE_START_PENDING, NO_ERROR, ServiceStartTimeout ) == false )
	{
		vCritical() << "Service start timed out";
		reportStatus( SERVICE_STOPPED, 0, 0 );
		return;
	}

	m_stopServiceEvent = CreateEvent( nullptr, false, false, nullptr );
	m_sessionChangeEvent = CreateEvent(nullptr, false, false, nullptr);

	if( reportStatus( SERVICE_RUNNING, NO_ERROR, 0 ) == false )
	{
		vCritical() << "Could not report service as running";
		return;
	}

	SasEventListener sasEventListener;
	sasEventListener.start();

	m_serviceEntryPoint();

	CloseHandle(m_sessionChangeEvent);
	CloseHandle( m_stopServiceEvent );

	sasEventListener.stop();
	sasEventListener.wait( SasEventListener::WaitPeriod );

	// Tell the service manager that we've stopped.
	reportStatus( SERVICE_STOPPED, 0, 0 );
}



DWORD WindowsServiceCore::serviceCtrl( DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context )
{
	Q_UNUSED(context);

	static const QMap<DWORD, const char *> controlMessages{
		{ SERVICE_CONTROL_SHUTDOWN, "SHUTDOWN" },
		{ SERVICE_CONTROL_STOP, "STOP" },
		{ SERVICE_CONTROL_PAUSE, "PAUSE" },
		{ SERVICE_CONTROL_CONTINUE, "CONTINUE" },
		{ SERVICE_CONTROL_INTERROGATE, "INTERROGATE" },
		{ SERVICE_CONTROL_PARAMCHANGE, "PARAM CHANGE" },
		{ SERVICE_CONTROL_DEVICEEVENT, "DEVICE EVENT" },
		{ SERVICE_CONTROL_HARDWAREPROFILECHANGE, "HARDWARE PROFILE CHANGE" },
		{ SERVICE_CONTROL_POWEREVENT, "POWER EVENT" },
		{ SERVICE_CONTROL_SESSIONCHANGE, "SESSION CHANGE" }
	};

	static const QMap<DWORD, const char *> sessionChangeEventTypes{
		{ WTS_CONSOLE_CONNECT, "CONSOLE CONNECT" },
		{ WTS_CONSOLE_DISCONNECT, "CONSOLE DISCONNECT" },
		{ WTS_REMOTE_CONNECT, "REMOTE CONNECT" },
		{ WTS_REMOTE_DISCONNECT, "REMOTE DISCONNECT" },
		{ WTS_SESSION_LOGON, "LOGON" },
		{ WTS_SESSION_LOGOFF, "LOGOFF" },
		{ WTS_SESSION_LOCK, "LOCK" },
		{ WTS_SESSION_UNLOCK, "UNLOCK" },
		{ WTS_SESSION_REMOTE_CONTROL, "REMOTE CONTROL" },
		{ WTS_SESSION_CREATE, "CREATE" },
		{ WTS_SESSION_TERMINATE, "TERMINATE" }
	};

	if( ctrlCode != SERVICE_CONTROL_SESSIONCHANGE &&
		controlMessages.contains( ctrlCode ) )
	{
		vDebug() << "control code:" << controlMessages[ctrlCode] << "event type:" << eventType;
	}

	// What control code have we been sent?
	switch( ctrlCode )
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		m_status.dwCurrentState = SERVICE_STOP_PENDING;
		m_serviceStopRequested = 1;
		SetEvent( m_stopServiceEvent );
		break;

	case SERVICE_CONTROL_INTERROGATE:
		// Service control manager just wants to know our state
		break;

	case SERVICE_CONTROL_SESSIONCHANGE:
		if( sessionChangeEventTypes.contains( eventType ) )
		{
			const auto notification = reinterpret_cast<WTSSESSION_NOTIFICATION *>( eventData );
			vDebug() << "session change event:" << sessionChangeEventTypes[eventType]
					 << "for session" << ( notification ? notification->dwSessionId : -1 );
		}
		switch( eventType )
		{
		case WTS_SESSION_LOGON:
		case WTS_SESSION_LOGOFF:
		case WTS_CONSOLE_CONNECT:
		case WTS_CONSOLE_DISCONNECT:
		case WTS_REMOTE_CONNECT:
		case WTS_REMOTE_DISCONNECT:
			m_sessionChanged = 1;
			SetEvent(m_sessionChangeEvent);
			break;
		}
		break;

	default:
		// Control code not recognised
		break;
	}

	// Tell the control manager what we're up to.
	reportStatus( m_status.dwCurrentState, NO_ERROR, 0 );

	return NO_ERROR;
}



// Service manager status reporting
bool WindowsServiceCore::reportStatus( DWORD state, DWORD exitCode, DWORD waitHint )
{
	static DWORD checkpoint = 1;
	bool result = true;

	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
	if( state == SERVICE_START_PENDING )
	{
		m_status.dwControlsAccepted = 0;
	}
	else
	{
		m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
	}

	// Save the new status we've been given
	m_status.dwCurrentState = state;
	m_status.dwWin32ExitCode = exitCode;
	m_status.dwWaitHint = waitHint;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if( ( state == SERVICE_RUNNING ) || ( state == SERVICE_STOPPED ) )
	{
		m_status.dwCheckPoint = 0;
	}
	else
	{
		m_status.dwCheckPoint = checkpoint++;
	}

	// Tell the SCM our new status
	if( !( result = SetServiceStatus( m_statusHandle, &m_status ) ) )
	{
		vCritical() << "SetServiceStatus failed.";
	}

	return result;
}
