/*
 * WindowsService.cpp - implementation of WindowsService-class
 *
 * Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "VeyonCore.h"

#include <QProcess>
#include <QTime>
#include <QThread>
#include <QApplication>
#include <QMessageBox>

#include "WindowsService.h"
#include "LocalSystem.h"

#ifdef VEYON_BUILD_WIN32

class SasEventListener : public QThread
{
public:
	typedef void (WINAPI *SendSas)(BOOL asUser);

	enum {
		WaitPeriod = 1000
	};

	SasEventListener()
	{
		m_sasLibrary = LoadLibrary( L"sas.dll" );
		m_sendSas = (SendSas)GetProcAddress( m_sasLibrary, "SendSAS" );
		if( m_sendSas == nullptr )
		{
			qWarning( "SendSAS is not supported by operating system!" );
		}

		m_sasEvent = CreateEvent( nullptr, false, false, L"Global\\VeyonServiceSasEvent" );
		m_stopEvent = CreateEvent( nullptr, false, false, L"StopEvent" );
	}

	~SasEventListener() override
	{
		CloseHandle( m_stopEvent );
		CloseHandle( m_sasEvent );

		FreeLibrary( m_sasLibrary );

	}

	void stop()
	{
		SetEvent( m_stopEvent );
	}

	void run() override
	{
		HANDLE eventObjects[2] = { m_sasEvent, m_stopEvent };

		while( isInterruptionRequested() == false )
		{
			int event = WaitForMultipleObjects( 2, eventObjects, false, WaitPeriod );

			switch( event )
			{
			case WAIT_OBJECT_0 + 0:
				ResetEvent( m_sasEvent );
				if( m_sendSas )
				{
					m_sendSas( false );
				}
				break;

			case WAIT_OBJECT_0 + 1:
				requestInterruption();
				break;

			default:
				break;
			}
		}
	}


private:
	HMODULE m_sasLibrary;
	SendSas m_sendSas;
	HANDLE m_sasEvent;
	HANDLE m_stopEvent;

};


class VeyonServiceSubProcess
{
public:
	VeyonServiceSubProcess() :
		m_subProcessHandle( NULL )
	{
	}

	~VeyonServiceSubProcess()
	{
		stop();
	}

	void start( int sessionId )
	{
		stop();

		qInfo() << "Starting server for user" << LocalSystem::User::loggedOnUser().name();
		// run with the same user as winlogon.exe does
		m_subProcessHandle =
				LocalSystem::Process(
					LocalSystem::Process::findProcessId( "winlogon.exe",
														 sessionId )
					).runAsUser( VeyonCore::serverFilePath(),
								 LocalSystem::Desktop().name() );
	}

	void stop()
	{
		if( m_subProcessHandle )
		{
			qInfo( "Waiting for server to shutdown" );
			if( WaitForSingleObject( m_subProcessHandle, 10000 ) ==
																WAIT_TIMEOUT )
			{
				qWarning( "Terminating server" );
				TerminateProcess( m_subProcessHandle, 0 );
			}
			CloseHandle( m_subProcessHandle ),
			m_subProcessHandle = NULL;
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



WindowsService::WindowsService( const QString &serviceName )
		:
	m_name( serviceName )
{
}




WindowsService *WindowsService::s_this = NULL;
SERVICE_STATUS WindowsService::s_status;
SERVICE_STATUS_HANDLE WindowsService::s_statusHandle;
HANDLE WindowsService::s_stopServiceEvent = (DWORD) NULL;
QAtomicInt WindowsService::s_sessionChangeEvent = 0;



bool WindowsService::runAsService()
{
	// Create a service entry table
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ (LPWSTR) m_name.utf16(), (LPSERVICE_MAIN_FUNCTION) serviceMain },
		{ NULL, NULL }
	} ;

	s_this = this;

	// call the service control dispatcher with our entry table
	if( !StartServiceCtrlDispatcher( dispatchTable ) )
	{
		qCritical( "WindowsService::runAsService(): "
					"StartServiceCtrlDispatcher failed." );
		return false;
	}

	return true;
}




void WINAPI WindowsService::serviceMain( DWORD argc, char **argv )
{
	DWORD context = 1;
	// register the service control handler
	s_statusHandle = RegisterServiceCtrlHandlerEx( (LPCWSTR) s_this->m_name.utf16(), serviceCtrl, &context );

	if( s_statusHandle == 0 )
	{
		return;
	}

	// Set up some standard service state values
	s_status.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;
	s_status.dwServiceSpecificExitCode = 0;

	// Give this status to the SCM
	if( !reportStatus(
			SERVICE_START_PENDING,	// Service state
			NO_ERROR,		// Exit code type
			15000 ) )		// Hint as to how long WinVNC
						// should have hung before you
						// assume error
	{
		reportStatus( SERVICE_STOPPED, 0, 0 );
		return;
	}

	s_stopServiceEvent = CreateEvent( 0, FALSE, FALSE, 0 );

	// report the status to the service control manager.
	if( !reportStatus( SERVICE_RUNNING, NO_ERROR, 0 ) )
	{
		return;
	}

	SasEventListener sasEventListener;
	sasEventListener.start();

	monitorSessions();

	CloseHandle( s_stopServiceEvent );

	sasEventListener.stop();
	sasEventListener.wait( SasEventListener::WaitPeriod );

	// Tell the service manager that we've stopped.
	reportStatus( SERVICE_STOPPED, 0, 0 );
}




// Service control routine
DWORD WINAPI WindowsService::serviceCtrl( DWORD _ctrlcode, DWORD dwEventType,
							LPVOID lpEventData,
							LPVOID lpContext )
{
	// What control code have we been sent?
	switch( _ctrlcode )
	{
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			// STOP : The service must stop
			s_status.dwCurrentState = SERVICE_STOP_PENDING;
			SetEvent( s_stopServiceEvent );
			break;

		case SERVICE_CONTROL_INTERROGATE:
			// Service control manager just wants to know our state
			break;

		case SERVICE_CONTROL_SESSIONCHANGE:
			switch( dwEventType )
			{
				case WTS_SESSION_LOGOFF:
					qInfo( "Session change event: WTS_SESSION_LOGOFF" );
					s_sessionChangeEvent = 1;
					break;
				case WTS_SESSION_LOGON:
					qInfo( "Session change event: WTS_SESSION_LOGON" );
					s_sessionChangeEvent = 1;
					break;
			}
			break;

		default:
			// Control code not recognised
			break;
	}

	// Tell the control manager what we're up to.
	reportStatus( s_status.dwCurrentState, NO_ERROR, 0 );

	return NO_ERROR;
}



// Service manager status reporting
bool WindowsService::reportStatus( DWORD state, DWORD exitCode, DWORD waitHint )
{
	static DWORD checkpoint = 1;
	bool result = true;

	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
	if( state == SERVICE_START_PENDING )
	{
		s_status.dwControlsAccepted = 0;
	}
	else
	{
		s_status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
										SERVICE_ACCEPT_SHUTDOWN |
										SERVICE_ACCEPT_SESSIONCHANGE;
	}

	// Save the new status we've been given
	s_status.dwCurrentState = state;
	s_status.dwWin32ExitCode = exitCode;
	s_status.dwWaitHint = waitHint;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if( ( state == SERVICE_RUNNING ) || ( state == SERVICE_STOPPED ) )
	{
		s_status.dwCheckPoint = 0;
	}
	else
	{
		s_status.dwCheckPoint = checkpoint++;
	}

	qDebug( "Reporting service status: %d", (int) state );

	// Tell the SCM our new status
	if( !( result = SetServiceStatus( s_statusHandle, &s_status ) ) )
	{
		qCritical( "WindowsService::reportStatus(...): SetServiceStatus failed." );
	}

	return result;
}




void WindowsService::monitorSessions()
{
	VeyonServiceSubProcess veyonProcess;

	HANDLE hShutdownEvent = CreateEvent( NULL, FALSE, FALSE,
									L"Global\\SessionEventUltra" );
	ResetEvent( hShutdownEvent );

	const DWORD SESSION_INVALID = 0xFFFFFFFF;
	DWORD oldSessionId = SESSION_INVALID;

	QTime lastServiceStart;

	while( WaitForSingleObject( s_stopServiceEvent, 1000 ) == WAIT_TIMEOUT )
	{
		bool sessionChanged = s_sessionChangeEvent.testAndSetOrdered( 1, 0 );

		const DWORD sessionId = WTSGetActiveConsoleSessionId();
		if( oldSessionId != sessionId || sessionChanged )
		{
			qInfo( "Session ID changed from %d to %d", (int) oldSessionId, (int) sessionId );
			// some logic for not reacting to desktop changes when the screen
			// locker got active - we also don't update oldSessionId so when
			// switching back to the original desktop, the above condition
			// should not be met and nothing should happen
			if( LocalSystem::Desktop::screenLockDesktop().isActive() )
			{
				qDebug( "ScreenLockDesktop is active - ignoring" );
				continue;
			}

			if( oldSessionId != SESSION_INVALID || sessionChanged )
			{
				// workaround for situations where service is stopped
				// while it is still starting up
				do
				{
					SetEvent( hShutdownEvent );
				}
				while( lastServiceStart.elapsed() < 10000 && veyonProcess.isRunning() );

				veyonProcess.stop();

				Sleep( 5000 );
			}
			if( sessionId != SESSION_INVALID || sessionChanged )
			{
				veyonProcess.start( sessionId );
				lastServiceStart.restart();
			}

			oldSessionId = sessionId;
		}
		else if( veyonProcess.isRunning() == false )
		{
			veyonProcess.start( sessionId );
			oldSessionId = sessionId;
			lastServiceStart.restart();
		}
	}

	qInfo( "Service shutdown" );

	SetEvent( hShutdownEvent );
	veyonProcess.stop();

	CloseHandle( hShutdownEvent );
}


#endif
