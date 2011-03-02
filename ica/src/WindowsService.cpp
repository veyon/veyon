/*
 * WindowsService.cpp - implementation of WindowsService-class
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTime>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>

#include "WindowsService.h"
#include "LocalSystem.h"
#include "Logger.h"

#ifdef ITALC_BUILD_WIN32


class ItalcServiceSubProcess
{
public:
	ItalcServiceSubProcess() :
		m_subProcessHandle( NULL )
	{
	}

	~ItalcServiceSubProcess()
	{
		stop();
	}

	void start( int sessionId )
	{
		stop();

		char appPath[MAX_PATH];
		if( GetModuleFileName( NULL, appPath, ARRAYSIZE(appPath) ) )
		{
			LogStream() << "Starting core server for user"
						<< LocalSystem::User::loggedOnUser().name();
			// run with the same user as winlogon.exe does
			m_subProcessHandle =
				LocalSystem::Process(
					LocalSystem::Process::findProcessId( "winlogon.exe",
															sessionId )
									).runAsUser( appPath,
											LocalSystem::Desktop().name() );
		}
	}

	void stop()
	{
		if( m_subProcessHandle )
		{
			ilog( Info, "Waiting for core server to shutdown" );
			if( WaitForSingleObject( m_subProcessHandle, 10000 ) ==
																WAIT_TIMEOUT )
			{
				ilog( Warning, "Terminating core server" );
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



WindowsService::WindowsService(
			const QString &serviceName,
			const QString &serviceArg,
			const QString &displayName,
			const QString &serviceDependencies,
			int argc,
			char **argv )
		:
	m_name( serviceName ),
	m_arg( serviceArg ),
	m_displayName( displayName ),
	m_dependencies( serviceDependencies ),
	m_running( false ),
	m_quiet( false ),
	m_argc( argc ),
	m_argv( argv )
{
}




bool WindowsService::evalArgs( int &argc, char **argv )
{
	if( argc <= 1 )
	{
		return false;
	}

	char appPath[MAX_PATH];
	if( !GetModuleFileName( NULL, appPath, ARRAYSIZE(appPath) ) )
	{
		return false;
	}

	if( argv[1] == m_arg )
	{
		ItalcCore::init();
		Logger l( "ItalcServiceMonitor" );
		return runAsService();
	}

	QApplication app( argc, argv );

	ItalcCore::init();

	Logger l( "ItalcServiceControl" );

	QStringList args = app.arguments();
	args.removeFirst();

	m_quiet = args.removeAll( "-quiet" ) > 0;

	struct ServiceOp
	{
		const char *arg;
		typedef bool(WindowsService:: * OpFunc)();
		OpFunc opFunc;
	} ;

	static const ServiceOp serviceOps[] = {
		{ "-registerservice", &WindowsService::install },
		{ "-unregisterservice", &WindowsService::remove },
		{ "-startservice", &WindowsService::start },
		{ "-stopservice", &WindowsService::stop },
		{ "", NULL }
	} ;

	foreach( QString arg, args )
	{
		arg = arg.toLower();
		for( size_t i = 0; i < sizeof( serviceOps ); ++i )
		{
			if( arg == serviceOps[i].arg )
			{
				// make sure to run as administrator.
				if( !LocalSystem::Process::isRunningAsAdmin() )
				{
					QString serviceArgs = serviceOps[i].arg;
					if( m_quiet )
					{
						serviceArgs += " -quiet";
					}
					LocalSystem::Process::runAsAdmin( appPath,
											serviceArgs.toUtf8().constData() );
				}
				else
				{
					(this->*serviceOps[i].opFunc)();
				}
				return true;
			}
		}
		qWarning() << "Unknown option" << arg;
	}

	return false;
}




WindowsService *WindowsService::s_this = NULL;
SERVICE_STATUS WindowsService::s_status;
SERVICE_STATUS_HANDLE WindowsService::s_statusHandle;
HANDLE WindowsService::s_stopServiceEvent = (DWORD) NULL;
QAtomicInt WindowsService::s_sessionChangeEvent = 0;



bool WindowsService::install()
{
	const unsigned int pathlength = 2048;
	char path[pathlength];
	char servicecmd[pathlength];

	// Get the filename of this executable
	if( GetModuleFileName( NULL, path, pathlength-(m_arg.length()+2) ) == 0 )
	{
		QMessageBox::critical( NULL, m_displayName,
			QApplication::tr( "Unable to register service '%1'." ).
								arg( m_displayName ) );
		return false;
	}

	// Append the service-start flag to the end of the path:
	if( strlen( path ) + 4 + m_arg.length() < pathlength )
	{
		sprintf( servicecmd, "\"%s\" %s", path,
					m_arg.toLocal8Bit().constData() );
	}
	else
	{
		return false;
	}

	// Open the default, local Service Control Manager database
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( hsrvmanager == NULL )
	{
		QMessageBox::critical( NULL, m_displayName,
				QApplication::tr(
					"The Service Control Manager could "
					"not be contacted (do you have the "
					"neccessary rights?!) - the "
					"service '%1' was not registered." ).
							arg( m_displayName ) );
		return false;
	}

	// Create an entry for the WinVNC service
	SC_HANDLE hservice = CreateService(
			hsrvmanager,		// SCManager database
			m_name.toLocal8Bit().constData(),	// name of service
			m_displayName.toLocal8Bit().constData(),// name to display
			SERVICE_ALL_ACCESS,	// desired access
			SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
						// service type
			SERVICE_AUTO_START,	// start type
			SERVICE_ERROR_NORMAL,	// error control type
			servicecmd,		// service's binary
			NULL,			// no load ordering group
			NULL,			// no tag identifier
			NULL,//m_dependencies.toLocal8Bit().constData(), // dependencies
			NULL,			// LocalSystem account
			NULL );			// no password
	if( hservice == NULL)
	{
		DWORD error = GetLastError();
		if( error == ERROR_SERVICE_EXISTS && !m_quiet )
		{
			QMessageBox::warning( NULL, m_displayName,
				QApplication::tr( "The service '%1' is already " "registered." ).
					arg( m_displayName ) );
		}
		else
		{
			QMessageBox::critical( NULL, m_displayName,
				QApplication::tr( "The service '%1' could not be registered." ).
					arg( m_displayName ) );
		}
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	SC_ACTION service_actions;
	service_actions.Delay = 10000;
	service_actions.Type = SC_ACTION_RESTART;


	SERVICE_FAILURE_ACTIONS service_failure_actions;
	service_failure_actions.dwResetPeriod = 0;
	service_failure_actions.lpRebootMsg = NULL;
	service_failure_actions.lpCommand = NULL;
	service_failure_actions.lpsaActions = &service_actions;
	service_failure_actions.cActions = 1;
	ChangeServiceConfig2( hservice, SERVICE_CONFIG_FAILURE_ACTIONS,
						&service_failure_actions );
/*	QProcess::execute(
		QString( "sc failure %1 reset= 0 actions= restart/1000"
							).arg( m_name ) );*/

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );
	// Everything went fine
	if( !m_quiet )
	{
		QMessageBox::information( NULL, m_displayName,
			QApplication::tr( "The service '%1' was successfully registered." ).
														arg( m_displayName ) );
	}

	return true;
}




bool WindowsService::remove()
{
	// Open the SCM
	SC_HANDLE hsrvmanager = OpenSCManager(
					NULL,	// machine (NULL == local)
					NULL,	// database (NULL == default)
				SC_MANAGER_ALL_ACCESS	// access required
							);
	bool suc = true;

	if( hsrvmanager )
	{
		SC_HANDLE hservice = OpenService( hsrvmanager,
						m_name.toLocal8Bit().constData(),
							SERVICE_ALL_ACCESS );

		if( hservice != NULL )
		{
			SERVICE_STATUS status;

			// Try to stop the service
			if( ControlService( hservice, SERVICE_CONTROL_STOP, &status ) )
			{
				while( QueryServiceStatus( hservice, &status ) )
				{
					if( status.dwCurrentState == SERVICE_STOP_PENDING )
					{
						Sleep( 1000 );
					}
					else
					{
						break;
					}
				}

				if( status.dwCurrentState != SERVICE_STOPPED )
				{
					QMessageBox::critical( NULL, m_displayName,
						QApplication::tr( "The service '%1' could not be "
											"stopped." ).arg( m_displayName ) );
					suc = false;
				}
			}

			// Now remove the service from the SCM
			if( suc && DeleteService( hservice ) )
			{
				if( !m_quiet )
				{
					QMessageBox::information( NULL, m_displayName,
						QApplication::tr( "The service '%1' has been "
											"unregistered." ).arg( m_displayName ) );
				}
			}
			else
			{
				DWORD error = GetLastError();
				if( error == ERROR_SERVICE_MARKED_FOR_DELETE )
				{
					if( !m_quiet )
					{
						QMessageBox::critical( NULL, m_displayName,
							QApplication::tr( "The service '%1' isn't "
												"registered and therefore "
												"can't be unregistered." ).
								arg( m_displayName ) );
					}
				}
				else
				{
						QMessageBox::critical( NULL, m_displayName,
							QApplication::tr( "The service '%1' could not be "
												"unregistered." ).
								arg( m_displayName ) );
				}
				suc = false;
			}
			CloseServiceHandle( hservice );
		}
		else
		{
			QMessageBox::critical( NULL, m_displayName,
				QApplication::tr( "The service '%1' could not be found." ).
					arg( m_displayName ) );
			suc = false;
		}
		CloseServiceHandle( hsrvmanager );
	}
	else
	{
		QMessageBox::critical( NULL, m_displayName,
			QApplication::tr( "The Service Control Manager could not be "
								"contacted (do you have the neccessary "
								"rights?!) - the service '%1' was not "
								"unregistered." ).arg( m_displayName ) );
		suc = false;
	}
	return suc;
}



bool WindowsService::start()
{
	return QProcess::execute( QString( "net start %1" ).arg( m_name ) ) == 0;

/*	// Open the SCM
	SC_HANDLE hsrvmanager = OpenSCManager(
					NULL,	// machine (NULL == local)
					NULL,	// database (NULL == default)
				SC_MANAGER_ALL_ACCESS	// access required
							);
	if( hsrvmanager )
	{
		SC_HANDLE hservice = OpenService( hsrvmanager,
						m_name.toUtf8().constData(),
							SERVICE_ALL_ACCESS );

		if( hservice != NULL )
		{
			SERVICE_STATUS status;
			status.dwCurrentState = SERVICE_START_PENDING;
			if( StartService( hservice, 0, NULL ) )
			{
				while( QueryServiceStatus( hservice, &status ) )
				{
					if( status.dwCurrentState ==
							SERVICE_START_PENDING )
					{
						Sleep( 1000 );
					}
					else
					{
						break;
					}
				}
			}
			CloseServiceHandle( hservice );
			CloseServiceHandle( hsrvmanager );
			if( status.dwCurrentState != SERVICE_RUNNING )
			{
	QMessageBox::critical( NULL, m_displayName,
		QApplication::tr( "The service '%1' could not be started." ).
							arg( m_displayName ) );
				return false;
			}
		}
		else
		{
			QMessageBox::critical( NULL, m_displayName,
		QApplication::tr( "The service '%1' could not be found." ).
							arg( m_displayName ) );
			CloseServiceHandle( hsrvmanager );
			return false;
		}
	}
	else
	{
		QMessageBox::critical( NULL, m_displayName,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the service '%1' was not "
						"started." ).
							arg( m_displayName )  );
		return false;
	}*/
	return true;
}




bool WindowsService::stop()
{
	// Open the SCM
	SC_HANDLE hsrvmanager = OpenSCManager(
					NULL,	// machine (NULL == local)
					NULL,	// database (NULL == default)
				SC_MANAGER_ALL_ACCESS	// access required
							);
	if( hsrvmanager )
	{
		SC_HANDLE hservice = OpenService( hsrvmanager,
					m_name.toLocal8Bit().constData(),
							SERVICE_ALL_ACCESS );

		if( hservice != NULL )
		{
			SERVICE_STATUS status;

			// Try to stop the service
			if( ControlService( hservice, SERVICE_CONTROL_STOP,
								&status ) )
			{
				while( QueryServiceStatus( hservice, &status ) )
				{
					if( status.dwCurrentState == SERVICE_STOP_PENDING )
					{
						Sleep( 1000 );
					}
					else
					{
						break;
					}
				}

				if( status.dwCurrentState != SERVICE_STOPPED )
				{
	QMessageBox::critical( NULL, m_displayName,
		QApplication::tr( "The service '%1' could not be stopped." ).
						arg( m_displayName ) );
					CloseServiceHandle( hservice );
					CloseServiceHandle( hsrvmanager );
					return false;
				}
			}
			CloseServiceHandle( hservice );
		}
		else
		{
			QMessageBox::critical( NULL, m_displayName,
				QApplication::tr( "The service '%1' could not be found." ).
						arg( m_displayName ) );
			CloseServiceHandle( hsrvmanager );
			return false;
		}
		CloseServiceHandle( hsrvmanager );
	}
	else
	{
		QMessageBox::critical( NULL, m_displayName,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the service '%1' was not "
						"stopped." ).
							arg( m_displayName ) );
		return false;
	}
	return true;
}




bool WindowsService::runAsService()
{
	if( m_running )
	{
		return false;
	}
	m_running = true;

	// Create a service entry table
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ m_name.toLocal8Bit().data(), (LPSERVICE_MAIN_FUNCTION) serviceMain },
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
	s_statusHandle = RegisterServiceCtrlHandlerEx(
					s_this->m_name.toLocal8Bit().constData(),
								serviceCtrl, &context );

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

	monitorSessions();

	CloseHandle( s_stopServiceEvent );

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
					ilog( Info, "Session change event: WTS_SESSION_LOGOFF" );
					s_sessionChangeEvent = 1;
					break;
				case WTS_SESSION_LOGON:
					ilog( Info, "Session change event: WTS_SESSION_LOGON" );
					//s_sessionChangeEvent = 1;	// no need to restart server upon logon
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

	ilogf( Debug, "Reporting service status: %d", state );

	// Tell the SCM our new status
	if( !( result = SetServiceStatus( s_statusHandle, &s_status ) ) )
	{
		qCritical( "WindowsService::reportStatus(...): "
						"SetServiceStatus failed." );
	}

	return result;
}




void WindowsService::monitorSessions()
{
	ItalcServiceSubProcess italcProcess;

	HANDLE hShutdownEvent = CreateEvent( NULL, FALSE, FALSE,
									"Global\\SessionEventUltra" );
	ResetEvent( hShutdownEvent );

	const DWORD SESSION_INVALID = 0xFFFFFFFF;
	DWORD oldSessionId = SESSION_INVALID;

	QTime lastServiceStart;

	while( WaitForSingleObject( s_stopServiceEvent, 1000 ) == WAIT_TIMEOUT )
	{
		bool sessionChanged = s_sessionChangeEvent.testAndSetOrdered( 1, 0 );
		// ignore session change events on Windows Vista and Windows 7 as
		// monitoring session IDs is reliable enough there and prevents us
		// from uneccessary server restarts
		if( sessionChanged &&
			( QSysInfo::windowsVersion() == QSysInfo::WV_VISTA ||
				QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7 ) )
		{
			ilog( Info, "Ignoring session change event as the operating system "
						"is recent enough" );
			sessionChanged = false;
		}

		const DWORD sessionId = WTSGetActiveConsoleSessionId();
		if( oldSessionId != sessionId || sessionChanged )
		{
			ilogf( Info, "Session ID changed from %d to %d",
									oldSessionId, sessionId );
			// some logic for not reacting to desktop changes when the screen
			// locker got active - we also don't update oldSessionId so when
			// switching back to the original desktop, the above condition
			// should not be met and nothing should happen
			if( LocalSystem::Desktop::screenLockDesktop().isActive() )
			{
				ilog( Debug, "ScreenLockDesktop is active - ignoring" );
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
				while( lastServiceStart.elapsed() < 10000 && italcProcess.isRunning() );

				italcProcess.stop();

				Sleep( 5000 );
			}
			if( sessionId != SESSION_INVALID || sessionChanged )
			{
				italcProcess.start( sessionId );
				lastServiceStart.restart();
			}

			oldSessionId = sessionId;
		}
		else if( italcProcess.isRunning() == false )
		{
			italcProcess.start( sessionId );
			oldSessionId = sessionId;
			lastServiceStart.restart();
		}
	}

	ilog( Info, "Service shutdown" );

	SetEvent( hShutdownEvent );
	italcProcess.stop();

	CloseHandle( hShutdownEvent );
}


#endif

