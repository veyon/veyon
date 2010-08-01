/*
 * SystemService.cpp - implementation of SystemService-class
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef NO_GUI
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#else
#include <QtCore/QCoreApplication>
#endif
#include <QtCore/QLocale>
#include <QtCore/QProcess>
#include <QtCore/QTranslator>

#include "SystemService.h"

#include <italcconfig.h>


#ifdef ITALC_BUILD_WIN32
PFN_WTSQuerySessionInformation pfnWTSQuerySessionInformation = NULL;
PFN_WTSFreeMemory pfnWTSFreeMemory = NULL;
#endif


SystemService::SystemService(
			const QString & _service_name,
			const QString & _service_arg,
			const QString & _service_display_name,
			const QString & _service_dependencies,
			service_main _sm,
			int _argc,
			char * * _argv )
		:
	m_name( _service_name ),
	m_arg( _service_arg ),
	m_displayName( _service_display_name ),
	m_dependencies( _service_dependencies ),
	m_serviceMain( _sm ),
	m_running( false ),
	m_quiet( false ),
	m_argc( _argc ),
	m_argv( _argv )
{
}




bool SystemService::evalArgs( int & _argc, char * * _argv )
{
	if( _argc > 1 )
	{
		const int oac = _argc;
		_argc = 0;
		const QString a = _argv[1];
		if( a == m_arg )
		{
			return runAsService();
		}
#ifndef NO_GUI
		QApplication app( _argc, _argv );
		const QString loc = QLocale::system().name().left( 2 );
		QTranslator app_tr;
		app_tr.load( ":/resources/" + loc + ".qm" );
		app.installTranslator( &app_tr );

		QTranslator qt_tr;
		qt_tr.load( ":/resources/qt_" + loc + ".qm" );
		app.installTranslator( &qt_tr );
#endif
		for( int i = 1; i < oac; ++i )
		{
			const QString a = _argv[i];
			if( a == m_arg )
			{
				return runAsService();
			}
			else if( a == "-registerservice" )
			{
				return install();
			}
			else if( a == "-unregisterservice" )
			{
				return remove();
			}
			else if( a == "-startservice" )
			{
				return start();
			}
			else if( a == "-stopservice" )
			{
				return stop();
			}
			else if( a == "-quiet" )
			{
				m_quiet = true;
			}
		}
		_argc = oac;
	}
	return false;
}



#ifdef ITALC_BUILD_LINUX


#include <QtCore/QFile>
#include <QtCore/QFileInfo>


inline bool isDebian( void )
{
	return QFileInfo( "/etc/debian_version" ).exists() ||
		QFileInfo( "/etc/ubuntu_version" ).exists();
}


bool SystemService::install( void )
{
	const QString sn = "/etc/init.d/" + m_name;
	QFile f( sn );
	f.open( QFile::WriteOnly | QFile::Truncate );
	f.write( QString( "%1 %2\n" ).
			arg( QCoreApplication::applicationFilePath() ).
			arg( m_arg ).toUtf8() );
	f.setPermissions( QFile::ReadOwner | QFile::WriteOwner |
							QFile::ExeOwner |
				QFile::ReadGroup | QFile::ExeGroup |
				QFile::ReadOther | QFile::ExeOther );
	f.close();
#ifdef NO_GUI
	if( isDebian() )
	{
		return QProcess::execute( QString( "update-rc.d %1 defaults 50" ).
								arg( m_name ) ) == 0;
	}
#else
	if( isDebian() )
	{
		QFile f( "/etc/X11/default-display-manager" );
		f.open( QFile::ReadOnly );
		const QString dm = QString( f.readAll() ).section( '/', -1 );
		f.close();
		if( dm == "kdm" )
		{
			QFile f( "/etc/kde3/kdm/Xsetup" );
			f.open( QFile::WriteOnly );
			f.seek( f.size() );
			return f.write( QString( "%1 %2" ).arg( sn ).arg( "start" ).
								toUtf8() ) > 0;
		}
		else if( dm == "gdm" )
		{
		}
		else if( dm == "xdm" )
		{
		}
		else
		{
		}
	}
#endif
	return false;
}


bool SystemService::remove( void )
{
	// TODO
	return false;
}




bool SystemService::start( void )
{
	return QProcess::execute( QString( "/etc/init.d/%1 start" ).
							arg( m_name ) ) == 0;
}




bool SystemService::stop( void )
{
	return QProcess::execute( QString( "/etc/init.d/%1 stop" ).
							arg( m_name ) ) == 0;
}




bool SystemService::runAsService( void )
{
	if( m_running || m_serviceMain == NULL )
	{
		return false;
	}
	//new workThread( serviceMainThread );
	m_serviceMain( this );
	m_running = false;
	return true;
}




void SystemService::serviceMainThread( void * _arg )
{
	SystemService * _this = static_cast<SystemService *>( _arg );
	_this->m_serviceMain( _this );
}




#elif ITALC_BUILD_WIN32


#include <stdio.h>
//#include "omnithread.h"


SystemService * 	SystemService::s_this = NULL;
SERVICE_STATUS		SystemService::s_status;
SERVICE_STATUS_HANDLE	SystemService::s_statusHandle;
DWORD			SystemService::s_error = 0;
DWORD			SystemService::s_serviceThread = (DWORD) NULL;



bool SystemService::install( void )
{
	const unsigned int pathlength = 2048;
	char path[pathlength];
	char servicecmd[pathlength];

	// TODO: replace by QCoreApplication::instance()->applicationFilePath()
	// Get the filename of this executable
	if( GetModuleFileName( NULL, path, pathlength -
				( m_arg.length() + 2 ) ) == 0 )
	{
#ifndef NO_GUI
		QMessageBox::critical( NULL, __app_name,
			QApplication::tr( "Unable to register service '%1'." ).
							arg( m_displayName ) );
#endif
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
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL,
							SC_MANAGER_ALL_ACCESS );
	if( hsrvmanager == NULL )
	{
#ifndef NO_GUI
		QMessageBox::critical( NULL, __app_name,
				QApplication::tr(
					"The Service Control Manager could "
					"not be contacted (do you have the "
					"neccessary rights?!) - the "
					"service '%1' was not registered." ).
							arg( m_displayName ) );
#endif
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
#ifndef NO_GUI
		DWORD error = GetLastError();
		if( error == ERROR_SERVICE_EXISTS && !m_quiet )
		{
			QMessageBox::warning( NULL, __app_name,
				QApplication::tr(
					"The service '%1' is already "
						"registered." ).
							arg( m_displayName ) );
		}
		else
		{
			QMessageBox::critical( NULL, __app_name,
				QApplication::tr(
					"The service '%1' could not "
					"be registered." ).
							arg( m_displayName ) );
		}
#else
		GetLastError();
#endif
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
#ifndef NO_GUI
	if( !m_quiet )
	{
		QMessageBox::information( NULL, __app_name,
			QApplication::tr(
				"The service '%1' was successfully registered."
						).arg( m_displayName ) );
	}
#endif

	return true;
}




bool SystemService::remove( void )
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
			if( ControlService( hservice, SERVICE_CONTROL_STOP,
								&status ) )
			{
				while( QueryServiceStatus( hservice, &status ) )
				{
					if( status.dwCurrentState ==
							SERVICE_STOP_PENDING )
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
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be stopped." ).
							arg( m_displayName ) );
#endif
					suc = false;
				}
			}

			// Now remove the service from the SCM
			if( suc && DeleteService( hservice ) )
			{
#ifndef NO_GUI
	if( !m_quiet )
	{
		QMessageBox::information( NULL, __app_name,
			QApplication::tr( "The service '%1' has been "
						"unregistered." ).
							arg( m_displayName ) );
	}
#endif
			}
			else
			{
				DWORD error = GetLastError();
				if( error == ERROR_SERVICE_MARKED_FOR_DELETE )
				{
#ifndef NO_GUI
	if( !m_quiet )
	{
		QMessageBox::critical( NULL, __app_name,
			QApplication::tr( "The service '%1' isn't registered "
						"and therefore can't be "
						"unregistered." ).
							arg( m_displayName ) );
	}
#endif
				}
				else
				{
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be "
							"unregistered." ).
						arg( m_displayName ) );
#endif
				}
				suc = false;
			}
			CloseServiceHandle( hservice );
		}
		else
		{
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be found." ).
							arg( m_displayName ) );
#endif
			suc = false;
		}
		CloseServiceHandle( hsrvmanager );
	}
	else
	{
#ifndef NO_GUI
		QMessageBox::critical( NULL, __app_name,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the service '%1' was not "
							"unregistered." ).
							arg( m_displayName ) );
#endif
		suc = false;
	}
	return suc;
}



bool SystemService::start( void )
{
	return QProcess::execute( QString( "net start %1" ).arg( m_name ) )
									== 0;

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
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be started." ).
							arg( m_displayName ) );
#endif
				return false;
			}
		}
		else
		{
#ifndef NO_GUI
			QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be found." ).
							arg( m_displayName ) );
#endif
			CloseServiceHandle( hsrvmanager );
			return false;
		}
	}
	else
	{
#ifndef NO_GUI
		QMessageBox::critical( NULL, __app_name,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the service '%1' was not "
						"started." ).
							arg( m_displayName )  );
#endif
		return false;
	}*/
	return true;
}




bool SystemService::stop( void )
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
					if( status.dwCurrentState ==
							SERVICE_STOP_PENDING )
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
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be stopped." ).
						arg( m_displayName ) );
#endif
					CloseServiceHandle( hservice );
					CloseServiceHandle( hsrvmanager );
					return false;
				}
			}
			CloseServiceHandle( hservice );
		}
		else
		{
#ifndef NO_GUI
			QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be found." ).
						arg( m_displayName ) );
#endif
			CloseServiceHandle( hsrvmanager );
			return false;
		}
		CloseServiceHandle( hsrvmanager );
	}
	else
	{
#ifndef NO_GUI
		QMessageBox::critical( NULL, __app_name,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the service '%1' was not "
						"stopped." ).
							arg( m_displayName ) );
#endif
		return false;
	}
	return true;
}




bool SystemService::runAsService( void )
{
	if( m_running || m_serviceMain == NULL )
	{
		return false;
	}
	m_running = true;

	// Create a service entry table
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ m_name.toLocal8Bit().data(), (LPSERVICE_MAIN_FUNCTION) main },
		{ NULL, NULL }
	} ;

	s_this = this;

	// call the service control dispatcher with our entry table
	if( !StartServiceCtrlDispatcher( dispatchTable ) )
	{
		qCritical( "SystemService::runAsService(): "
					"StartServiceCtrlDispatcher failed." );
		return false;
	}

	return true;
}






void SystemService::serviceMainThread( void * _arg )
{
	// Save the current thread identifier
	s_serviceThread = GetCurrentThreadId();

	// report the status to the service control manager.
	if( !reportStatus(	SERVICE_RUNNING,	// service state
				NO_ERROR,		// exit code
				0 ) )			// wait hint
	{
		return;
	}

	SystemService * _this = static_cast<SystemService *>( _arg );
	_this->m_serviceMain( _this );

	// Mark that we're no longer running
	s_serviceThread = (DWORD) NULL;

	// Tell the service manager that we've stopped.
	reportStatus( SERVICE_STOPPED, s_error, 0 );
}



void WINAPI SystemService::main( DWORD _argc, char * * _argv )
{
	HMODULE hWTSAPI32 = LoadLibrary("wtsapi32.dll");
	if( hWTSAPI32 )
	{
		pfnWTSQuerySessionInformation = (PFN_WTSQuerySessionInformation)
			GetProcAddress(hWTSAPI32,"WTSQuerySessionInformationA");
		pfnWTSFreeMemory = (PFN_WTSFreeMemory)
		GetProcAddress(hWTSAPI32,"WTSFreeMemory");
	}
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
		reportStatus( SERVICE_STOPPED, s_error, 0 );
		return;
	}

	//serviceMainThread(0);
	// now start the service for real
	//omni_thread::create( serviceMainThread, s_this );
	( new workThread( serviceMainThread, s_this ) )->start();
	return;
}




QString __sessCurUser;

// Service control routine
DWORD WINAPI SystemService::serviceCtrl( DWORD _ctrlcode, DWORD dwEventType,
							LPVOID lpEventData,
							LPVOID lpContext )
{
	// What control code have we been sent?
	switch( _ctrlcode )
	{
		case SERVICE_CONTROL_STOP:
			// STOP : The service must stop
			s_status.dwCurrentState = SERVICE_STOP_PENDING;
			// Post a quit message to the main service thread
			if( s_serviceThread )
			{
				PostThreadMessage( s_serviceThread, WM_QUIT, 0,
									0 );
			}
			break;

		case SERVICE_CONTROL_INTERROGATE:
			// Service control manager just wants to know our state
			break;

		case SERVICE_CONTROL_SESSIONCHANGE:
			WTSSESSION_NOTIFICATION wtsno;
			CopyMemory( &wtsno, lpEventData,
					sizeof( WTSSESSION_NOTIFICATION ) );

			switch( dwEventType )
			{
				case WTS_SESSION_LOGOFF:
					__sessCurUser = "";
					break;
				case WTS_SESSION_LOGON:
					LPTSTR pBuffer = NULL;
					DWORD dwBufferLen;
	BOOL bRes = (*pfnWTSQuerySessionInformation)( WTS_CURRENT_SERVER_HANDLE,
							wtsno.dwSessionId,
							WTSUserName,
							&pBuffer,
							&dwBufferLen );
					if( bRes != false )
					{
						__sessCurUser = pBuffer;
					}
					(*pfnWTSFreeMemory)( pBuffer );
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
bool SystemService::reportStatus( DWORD _state, DWORD _exit_code,
							DWORD _wait_hint )
{
	static DWORD checkpoint = 1;
	bool result = true;

	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
	if( _state == SERVICE_START_PENDING )
	{
		s_status.dwControlsAccepted = 0;
	}
	else
	{
		if( QSysInfo::WindowsVersion == QSysInfo::WV_2000 )
		{
			s_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		}
		else
		{
			s_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
		}
	}

	// Save the new status we've been given
	s_status.dwCurrentState = _state;
	s_status.dwWin32ExitCode = _exit_code;
	s_status.dwWaitHint = _wait_hint;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if( ( _state == SERVICE_RUNNING ) || ( _state == SERVICE_STOPPED ) )
	{
		s_status.dwCheckPoint = 0;
	}
	else
	{
		s_status.dwCheckPoint = checkpoint++;
	}

	// Tell the SCM our new status
	if( !( result = SetServiceStatus( s_statusHandle, &s_status ) ) )
	{
		qCritical( "SystemService::reportStatus(...): "
						"SetServiceStatus failed." );
	}

	return result;
}


#endif

