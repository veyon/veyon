/*
 * system_service.cpp - implementation of systemService-class
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *  
 * This file is part of iTALC - http://italc.sourceforge.net
 * This file is part of LUPUS - http://lupus.sourceforge.net
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

#include "system_service.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


systemService::systemService(
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
	m_running( FALSE ),
	m_argc( _argc ),
	m_argv( _argv )
{
}




bool systemService::evalArgs( int & _argc, char * * _argv )
{
	if( _argc > 1 )
	{
		const int oac = _argc;
		_argc = 0;
		const QString a = _argv[1];
		if( a == m_arg )
		{
			return( runAsService() );
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
				return( runAsService() );
			}
			else if( a == "-registerservice" )
			{
				return( install() );
			}
			else if( a == "-unregisterservice" )
			{
				return( remove() );
			}
			else if( a == "-startservice" )
			{
				return( start() );
			}
			else if( a == "-stopservice" )
			{
				return( stop() );
			}
		}
		_argc = oac;
	}
	return( FALSE );
}



#ifdef BUILD_LINUX


#include <QtCore/QFile>
#include <QtCore/QFileInfo>


inline bool isDebian( void )
{
	return( QFileInfo( "/etc/debian_version" ).exists() ||
		QFileInfo( "/etc/ubuntu_version" ).exists() );
}


bool systemService::install( void )
{
	const QString sn = "/etc/init.d/" + m_name;
	QFile f( sn );
	f.open( QFile::WriteOnly | QFile::Truncate );
	f.write( QString( "%1 %2\n" ).
			arg( QCoreApplication::applicationFilePath() ).
			arg( m_arg ).toAscii() );
	f.setPermissions( QFile::ReadOwner | QFile::WriteOwner |
							QFile::ExeOwner |
				QFile::ReadGroup | QFile::ExeGroup |
				QFile::ReadOther | QFile::ExeOther );
	f.close();
#ifdef NO_GUI
	if( isDebian() )
	{
		QProcess::execute( QString( "update-rc.d %1 defaults 50" ).
								arg( m_name ) );
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
			f.write( QString( "%1 %2" ).arg( sn ).arg( "start" ).
								toAscii() );
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
}


bool systemService::remove( void )
{
	// TODO
}




bool systemService::start( void )
{
	return( QProcess::execute( QString( "/etc/init.d/%1 start" ).
							arg( m_name ) ) == 0 );
}




bool systemService::stop( void )
{
	return( QProcess::execute( QString( "/etc/init.d/%1 stop" ).
							arg( m_name ) ) == 0 );
}




bool systemService::runAsService( void )
{
	if( m_running || m_serviceMain == NULL )
	{
		return( FALSE );
	}
	//new workThread( serviceMainThread );
	m_serviceMain( this );
	m_running = FALSE;
	return( TRUE );
}




void systemService::serviceMainThread( void * _arg )
{
	systemService * _this = static_cast<systemService *>( _arg );
	_this->m_serviceMain( _this );
}




#elif BUILD_WIN32


#include <stdio.h>
//#include "omnithread.h"


systemService * 	systemService::s_this = NULL;
SERVICE_STATUS		systemService::s_status;
SERVICE_STATUS_HANDLE	systemService::s_statusHandle;
DWORD			systemService::s_error = 0;
DWORD			systemService::s_serviceThread = (DWORD) NULL;



bool systemService::install( void )
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
		return( FALSE );
	}

	// Append the service-start flag to the end of the path:
	if( strlen( path ) + 4 + m_arg.length() < pathlength )
	{
		sprintf( servicecmd, "\"%s\" %s", path,
						m_arg.toAscii().constData() );
	}
	else
	{
		return( FALSE );
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
		return( FALSE );
	}

	// Create an entry for the WinVNC service
	SC_HANDLE hservice = CreateService(
			hsrvmanager,		// SCManager database
			m_name.toAscii().constData(),	// name of service
			m_displayName.toAscii().constData(),// name to display
			SERVICE_ALL_ACCESS,	// desired access
			SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
						// service type
			SERVICE_AUTO_START,	// start type
			SERVICE_ERROR_NORMAL,	// error control type
			servicecmd,		// service's binary
			NULL,			// no load ordering group
			NULL,			// no tag identifier
			m_dependencies.toAscii().constData(), // dependencies
			NULL,			// LocalSystem account
			NULL );			// no password
	if( hservice == NULL)
	{
#ifndef NO_GUI
		DWORD error = GetLastError();
		if( error == ERROR_SERVICE_EXISTS )
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
		return( FALSE );
	}
	CloseServiceHandle( hsrvmanager );
	CloseServiceHandle( hservice );

	QProcess::execute(
		QString( "sc failure %1 reset= 0 actions= restart/1000"
							).arg( m_name ) );
	// Everything went fine
#ifndef NO_GUI
	QMessageBox::information( NULL, __app_name,
		QApplication::tr(
				"The service '%1' was successfully registered. "
				"It will automatically be run the next time "
				"you start this computer." ).
							arg( m_displayName ) );
#endif

	return( TRUE );
}




bool systemService::remove( void )
{
	// Open the SCM
	SC_HANDLE hsrvmanager = OpenSCManager(
					NULL,	// machine (NULL == local)
					NULL,	// database (NULL == default)
				SC_MANAGER_ALL_ACCESS	// access required
							);
	bool suc = TRUE;

	if( hsrvmanager )
	{ 
		SC_HANDLE hservice = OpenService( hsrvmanager,
						m_name.toAscii().constData(),
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
					suc = FALSE;
				}
			}

			// Now remove the service from the SCM
			if( suc && DeleteService( hservice ) )
			{
#ifndef NO_GUI
	QMessageBox::information( NULL, __app_name,
		QApplication::tr( "The service '%1' has been unregistered." ).
							arg( m_displayName ) );
#endif
			}
			else
			{
				DWORD error = GetLastError();
				if( error == ERROR_SERVICE_MARKED_FOR_DELETE )
				{
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' isn't registered and "
					"therefore can't be unregistered." ).
							arg( m_displayName ) );
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
				suc = FALSE;
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
			suc = FALSE;
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
		suc = FALSE;
	}
	return( suc );
}



bool systemService::start( void )
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
						m_name.toAscii().constData(),
							SERVICE_ALL_ACCESS );

		if( hservice != NULL )
		{
			SERVICE_STATUS status;

			// Try to stop the service
			if( ControlService( hservice, SERVICE_START,
								&status ) )
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

				if( status.dwCurrentState != SERVICE_RUNNING )
				{
#ifndef NO_GUI
	QMessageBox::critical( NULL, __app_name,
		QApplication::tr( "The service '%1' could not be started." ).
							arg( m_displayName ) );
#endif
					CloseServiceHandle( hsrvmanager );
					return( FALSE );
				}
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
			return( FALSE );
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
						"started." ).
							arg( m_displayName )  );
#endif
		return( FALSE );
	}
	return( TRUE );
}




bool systemService::stop( void )
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
						m_name.toAscii().constData(),
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
					CloseServiceHandle( hsrvmanager );
					return( FALSE );
				}
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
			return( FALSE );
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
		return( FALSE );
	}
	return( TRUE );
}




bool systemService::runAsService( void )
{
	if( m_running || m_serviceMain == NULL )
	{
		return( FALSE );
	}
	m_running = TRUE;

	// Create a service entry table
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ m_name.toAscii().data(), (LPSERVICE_MAIN_FUNCTION) main },
		{ NULL, NULL }
	} ;

	s_this = this;

	// call the service control dispatcher with our entry table
	if( !StartServiceCtrlDispatcher( dispatchTable ) )
	{
		qCritical( "systemService::runAsService(): "
					"StartServiceCtrlDispatcher failed." );
		return( FALSE );
	}

	return( TRUE );
}






void systemService::serviceMainThread( void * _arg )
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

	systemService * _this = static_cast<systemService *>( _arg );
	_this->m_serviceMain( _this );

	// Mark that we're no longer running
	s_serviceThread = (DWORD) NULL;

	// Tell the service manager that we've stopped.
	reportStatus( SERVICE_STOPPED, s_error, 0 );
}



void WINAPI systemService::main( DWORD _argc, char * * _argv )
{
	// register the service control handler
	s_statusHandle = RegisterServiceCtrlHandler(
					s_this->m_name.toAscii().constData(),
								serviceCtrl );

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



// Service control routine
void WINAPI systemService::serviceCtrl( DWORD _ctrlcode )
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

		default:
			// Control code not recognised
			break;
	}

	// Tell the control manager what we're up to.
	reportStatus( s_status.dwCurrentState, NO_ERROR, 0 );
}



// Service manager status reporting
bool systemService::reportStatus( DWORD _state, DWORD _exit_code,
							DWORD _wait_hint )
{
	static DWORD checkpoint = 1;
	bool result = TRUE;

	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
  	if( _state == SERVICE_START_PENDING )
	{
		s_status.dwControlsAccepted = 0;
	}
	else
	{
		s_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
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
		qCritical( "systemService::reportStatus(...): "
						"SetServiceStatus failed." );
	}

	return( result );
}


#endif

