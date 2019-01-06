/*
 * WindowsServiceCore.cpp - implementation of WindowsServiceCore class
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>

#include <QElapsedTimer>

#include "Filesystem.h"
#include "WindowsServiceCore.h"
#include "SasEventListener.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"
#include "WindowsCoreFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WtsSessionManager.h"


class VeyonServerProcess
{
public:
	VeyonServerProcess() :
		m_subProcessHandle( nullptr )
	{
	}

	~VeyonServerProcess()
	{
		stop();
	}

	void start( DWORD wtsSessionId )
	{
		stop();

		qInfo() << "Starting server for WTS session" << wtsSessionId
				<< "with user" << VeyonCore::platform().userFunctions().currentUser();

		m_subProcessHandle = WindowsCoreFunctions::runProgramInSession( VeyonCore::filesystem().serverFilePath(), { },
																		WtsSessionManager::findWinlogonProcessId( wtsSessionId ) );
		if( m_subProcessHandle == nullptr )
		{
			qCritical( "Failed to start server!" );
		}
	}

	void stop()
	{
		if( m_subProcessHandle )
		{
			qInfo( "Waiting for server to shutdown" );
			if( WaitForSingleObject( m_subProcessHandle, 10000 ) == WAIT_TIMEOUT )
			{
				qWarning( "Terminating server" );
				TerminateProcess( m_subProcessHandle, 0 );
			}
			CloseHandle( m_subProcessHandle );
			m_subProcessHandle = nullptr;
		}
	}

	bool isRunning() const
	{
		if( m_subProcessHandle &&
				WaitForSingleObject( m_subProcessHandle, 5000 ) == WAIT_TIMEOUT )
		{
			return true;
		}

		return false;
	}


private:
	HANDLE m_subProcessHandle;
} ;



WindowsServiceCore* WindowsServiceCore::s_instance = nullptr;


WindowsServiceCore::WindowsServiceCore( const QString& name, std::function<void(void)> serviceMainEntry ) :
	m_name( WindowsCoreFunctions::toWCharArray( name ) ),
	m_serviceMainEntry( serviceMainEntry ),
	m_status(),
	m_statusHandle( 0 ),
	m_stopServiceEvent( nullptr ),
	m_serverShutdownEvent( nullptr ),
	m_sessionChangeEvent( 0 )
{
	s_instance = this;

	// enable privileges required to create process with access token from other process
	WindowsCoreFunctions::enablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, true );
	WindowsCoreFunctions::enablePrivilege( SE_INCREASE_QUOTA_NAME, true );
}



WindowsServiceCore::~WindowsServiceCore()
{
	delete[] m_name;
}



WindowsServiceCore *WindowsServiceCore::instance()
{
	Q_ASSERT(s_instance != nullptr);

	return s_instance;
}



bool WindowsServiceCore::runAsService()
{
	SERVICE_TABLE_ENTRY dispatchTable[] = {
		{ m_name, serviceMainStatic },
		{ nullptr, nullptr }
	} ;

	if( !StartServiceCtrlDispatcher( dispatchTable ) )
	{
		qCritical( "WindowsServiceCore::runAsService(): StartServiceCtrlDispatcher failed." );
		return false;
	}

	return true;
}



void WindowsServiceCore::manageServerInstances()
{
	WindowsInputDeviceFunctions::checkInterceptionInstallation();

	m_serverShutdownEvent = CreateEvent( nullptr, false, false, L"Global\\SessionEventUltra" );
	ResetEvent( m_serverShutdownEvent );

	if( VeyonCore::config().isMultiSessionServiceEnabled() )
	{
		manageServersForAllSessions();
	}
	else
	{
		manageServerForActiveConsoleSession();
	}

	CloseHandle( m_serverShutdownEvent );
}



void WindowsServiceCore::manageServersForAllSessions()
{
	QMap<WtsSessionManager::SessionId, VeyonServerProcess*> serverProcesses;

	while( WaitForSingleObject( m_stopServiceEvent, 1000 ) == WAIT_TIMEOUT )
	{
		const auto wtsSessionIds = WtsSessionManager::activeSessions();

		for( auto it = serverProcesses.begin(), end = serverProcesses.end(); it != end; )
		{
			if( wtsSessionIds.contains( it.key() ) == false )
			{
				delete it.value();
				it = serverProcesses.erase( it );
			}
			else
			{
				++it;
			}
		}

		for( auto wtsSessionId : wtsSessionIds )
		{
			if( serverProcesses.contains( wtsSessionId ) == false )
			{
				auto serverProcess = new VeyonServerProcess;
				serverProcess->start( wtsSessionId );

				serverProcesses[wtsSessionId] = serverProcess;
			}
		}
	}

	qInfo( "Service shutdown" );

	SetEvent( m_serverShutdownEvent );

	for( auto it = serverProcesses.begin(), end = serverProcesses.end(); it != end; ++it )
	{
		delete it.value();
	}
}



void WindowsServiceCore::manageServerForActiveConsoleSession()
{
	VeyonServerProcess veyonServerProcess;

	auto oldWtsSessionId = WtsSessionManager::InvalidSession;

	QElapsedTimer lastServiceStart;

	do
	{
		bool sessionChanged = m_sessionChangeEvent.testAndSetOrdered( 1, 0 );

		const auto wtsSessionId = WtsSessionManager::activeConsoleSession();
		if( oldWtsSessionId != wtsSessionId || sessionChanged )
		{
			qInfo() << "WTS session ID changed from" << oldWtsSessionId << "to" << wtsSessionId;

			if( oldWtsSessionId != WtsSessionManager::InvalidSession || sessionChanged )
			{
				// workaround for situations where service is stopped
				// while it is still starting up
				do
				{
					SetEvent( m_serverShutdownEvent );
				}
				while( lastServiceStart.elapsed() < 10000 && veyonServerProcess.isRunning() );

				veyonServerProcess.stop();

				Sleep( 5000 );
			}
			if( wtsSessionId != WtsSessionManager::InvalidSession || sessionChanged )
			{
				veyonServerProcess.start( wtsSessionId );
				lastServiceStart.restart();
			}

			oldWtsSessionId = wtsSessionId;
		}
		else if( veyonServerProcess.isRunning() == false )
		{
			veyonServerProcess.start( wtsSessionId );
			oldWtsSessionId = wtsSessionId;
			lastServiceStart.restart();
		}
	} while( WaitForSingleObject( m_stopServiceEvent, 1000 ) == WAIT_TIMEOUT );

	qInfo( "Service shutdown" );

	SetEvent( m_serverShutdownEvent );
	veyonServerProcess.stop();
}



void WindowsServiceCore::serviceMainStatic( DWORD argc, LPWSTR* argv )
{
	Q_UNUSED(argc)
	Q_UNUSED(argv)

	instance()->serviceMain();
}



DWORD WindowsServiceCore::serviceCtrlStatic(DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context)
{
	return instance()->serviceCtrl( ctrlCode, eventType, eventData, context );
}



void WindowsServiceCore::serviceMain()
{
	DWORD context = 1;

	m_statusHandle = RegisterServiceCtrlHandlerEx( m_name, serviceCtrlStatic, &context );

	if( m_statusHandle == 0 )
	{
		return;
	}

	memset( &m_status, 0, sizeof( m_status ) );
	m_status.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;

	if( reportStatus( SERVICE_START_PENDING, NO_ERROR, 15000 ) == false )
	{
		reportStatus( SERVICE_STOPPED, 0, 0 );
		return;
	}

	m_stopServiceEvent = CreateEvent( 0, FALSE, FALSE, 0 );

	if( reportStatus( SERVICE_RUNNING, NO_ERROR, 0 ) == false )
	{
		return;
	}

	SasEventListener sasEventListener;
	sasEventListener.start();

	m_serviceMainEntry();

	CloseHandle( m_stopServiceEvent );

	sasEventListener.stop();
	sasEventListener.wait( SasEventListener::WaitPeriod );

	// Tell the service manager that we've stopped.
	reportStatus( SERVICE_STOPPED, 0, 0 );
}



DWORD WindowsServiceCore::serviceCtrl( DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context )
{
	Q_UNUSED(eventData)
	Q_UNUSED(context)

	// What control code have we been sent?
	switch( ctrlCode )
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		// STOP : The service must stop
		m_status.dwCurrentState = SERVICE_STOP_PENDING;
		SetEvent( m_stopServiceEvent );
		break;

	case SERVICE_CONTROL_INTERROGATE:
		// Service control manager just wants to know our state
		break;

	case SERVICE_CONTROL_SESSIONCHANGE:
		switch( eventType )
		{
		case WTS_SESSION_LOGOFF:
			qInfo( "Session change event: WTS_SESSION_LOGOFF" );
			m_sessionChangeEvent = 1;
			break;
		case WTS_SESSION_LOGON:
			qInfo( "Session change event: WTS_SESSION_LOGON" );
			m_sessionChangeEvent = 1;
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

	qDebug( "Reporting service status: %d", (int) state );

	// Tell the SCM our new status
	if( !( result = SetServiceStatus( m_statusHandle, &m_status ) ) )
	{
		qCritical( "WindowsServiceCore::reportStatus(...): SetServiceStatus failed." );
	}

	return result;
}
