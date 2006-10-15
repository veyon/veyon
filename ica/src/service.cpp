/*
 * service.cpp - implementation of service-functionalities
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtGui/QApplication>
#include <QtGui/QMessageBox>

#include "service.h"
#include "ica_main.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


int icaServiceReinstall( void )
{
	icaServiceRemove( 1 );
	icaServiceInstall( 0 );
	return( 0 );
}




#ifdef BUILD_LINUX


int icaServiceMain( void )
{
	return( 0 );
}

int icaServiceInstall( bool )
{
}
int icaServiceRemove( bool )
{
}

void icaServiceStop( bool )
{
}


#elif BUILD_WIN32


#include <windows.h>
#include <stdio.h>
#include "omnithread.h"

// Executable name
#define ICA_APPNAME            "ica"

// Internal service name
#define ICA_SERVICENAME        "icas"

// Displayed service name
#define ICA_SERVICEDISPLAYNAME "iTALC Client"

#define ICA_DEPENDENCIES       ""

// internal service state
static SERVICE_STATUS		g_srvstatus;	// current status of the service
static SERVICE_STATUS_HANDLE	g_hstatus;
static DWORD			g_error = 0;
static DWORD			g_servicethread = (DWORD) NULL;

static const char * szAppName = "iTALC Client";

// forward defines of internal service functions
void WINAPI ServiceMain( DWORD _argc, char * * _argv );

static void ServiceWorkThread( void * _arg );
static void ServiceStop( void );
void WINAPI ServiceCtrl( DWORD _ctrlcode );

bool WINAPI CtrlHandler( DWORD _ctrltype );

static BOOL ReportStatus( DWORD _state, DWORD _exitcode, DWORD _waithint );


void icaServiceStop( bool _silent )
{
	// Open the SCM
	SC_HANDLE hsrvmanager = OpenSCManager(
					NULL,	// machine (NULL == local)
					NULL,	// database (NULL == default)
				SC_MANAGER_ALL_ACCESS	// access required
							);
	if( hsrvmanager )
	{ 
		SC_HANDLE hservice = OpenService( hsrvmanager, ICA_SERVICENAME,
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
					if( !_silent )
					{
	QMessageBox::critical( NULL, szAppName,
		QApplication::tr( "The iTALC Client service could not be "
								"stopped." ) );
					}
				}
			}
		}
		else if( !_silent )
		{
	QMessageBox::critical( NULL, szAppName,
		QApplication::tr( "The iTALC Client service could not be "
								"found." ) );
		}
		CloseServiceHandle( hsrvmanager );
	}
	else if( !_silent )
	{
		QMessageBox::critical( NULL, szAppName,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the iTALC Client "
						"service was not stopped." ) );
	}
}



int icaServiceMain( void )
{
	typedef DWORD ( WINAPI * RegisterServiceProc )( DWORD, DWORD );

	// Create a service entry table
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ ICA_SERVICENAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain },
		{ NULL, NULL }
	} ;

	// call the service control dispatcher with our entry table
	if( !StartServiceCtrlDispatcher( dispatchTable ) )
	{
		printf( "StartServiceCtrlDispatcher failed\n." );
		return( 0 );
	}

	return( -1 );
}



void WINAPI ServiceMain( DWORD _argc, char * * _argv )
{
	// register the service control handler
	g_hstatus = RegisterServiceCtrlHandler( ICA_SERVICENAME, ServiceCtrl );

	if( g_hstatus == 0 )
	{
		return;
	}

	// Set up some standard service state values
	g_srvstatus.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;
	g_srvstatus.dwServiceSpecificExitCode = 0;

	// Give this status to the SCM
	if( !ReportStatus(
			SERVICE_START_PENDING,	// Service state
			NO_ERROR,		// Exit code type
			15000 ) )		// Hint as to how long WinVNC
						// should have hung before you
						// assume error
	{
		ReportStatus( SERVICE_STOPPED, g_error, 0 );
		return;
	}

	// Now start the service for real
	omni_thread::create( ServiceWorkThread );
	return;
}



// SERVICE START ROUTINE - thread that calls ICAMain
void ServiceWorkThread( void * _arg )
{
	// Save the current thread identifier
	g_servicethread = GetCurrentThreadId();
	
	// report the status to the service control manager.
	//
	if( !ReportStatus(
				SERVICE_RUNNING,	// service state
				NO_ERROR,		// exit code
				0 ) )			// wait hint
	{
		return;
	}

	// RUN!
	char * * v = new char *[1];
	v[0] = ICA_APPNAME;
	ICAMain( 1, v );
	delete[] v;

	// Mark that we're no longer running
	g_servicethread = (DWORD) NULL;

	// Tell the service manager that we've stopped.
	ReportStatus( SERVICE_STOPPED, g_error, 0 );
}


// SERVICE STOP ROUTINE - post a quit message to the relevant thread
void ServiceStop( void )
{
	// Post a quit message to the main service thread
	if( g_servicethread )
	{
		PostThreadMessage( g_servicethread, WM_QUIT, 0, 0 );
	}
}



int icaServiceInstall( bool _silent )
{
	const unsigned int pathlength = 2048;
	char path[pathlength];
	char servicecmd[pathlength];

	// Get the filename of this executable
	if( GetModuleFileName( NULL, path, pathlength -
				( strlen( SERVICE_ARG ) + 2 ) ) == 0 )
	{
		if( !_silent )
		{
			QMessageBox::critical( NULL, szAppName,
				QApplication::tr( "Unable to register iTALC "
						"Client as service." ) );
		}
		return( 0 );
	}

	// Append the service-start flag to the end of the path:
	if( strlen( path ) + 4 + strlen( SERVICE_ARG ) < pathlength )
	{
		sprintf( servicecmd, "\"%s\" %s", path, SERVICE_ARG );
	}
	else
	{
		return( 0 );
	}

	// Open the default, local Service Control Manager database
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL,
							SC_MANAGER_ALL_ACCESS );
	if( hsrvmanager == NULL )
	{
		if( !_silent )
		{
			QMessageBox::critical( NULL, szAppName,
				QApplication::tr(
					"The Service Control Manager could "
					"not be contacted (do you have the "
					"neccessary rights?!) - the iTALC "
					"Client was not registered as "
								"service." ) );
		}
		return( 0 );
	}

	// Create an entry for the WinVNC service
	SC_HANDLE hservice = CreateService(
			hsrvmanager,		// SCManager database
			ICA_SERVICENAME,	// name of service
			ICA_SERVICEDISPLAYNAME,	// name to display
			SERVICE_ALL_ACCESS,	// desired access
			SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
						// service type
			SERVICE_AUTO_START,	// start type
			SERVICE_ERROR_NORMAL,	// error control type
			servicecmd,		// service's binary
			NULL,			// no load ordering group
			NULL,			// no tag identifier
			ICA_DEPENDENCIES,	// dependencies
			NULL,			// LocalSystem account
			NULL );			// no password
	if( hservice == NULL)
	{
		DWORD error = GetLastError();
		if( !_silent )
		{
			if( error == ERROR_SERVICE_EXISTS )
			{
				QMessageBox::warning( NULL, szAppName,
					QApplication::tr(
						"The iTALC Client is already "
						"registered as service." ) );
			}
			else
			{
				QMessageBox::critical( NULL, szAppName,
					QApplication::tr(
						"The iTALC Client could not "
						"be registered as service." ) );
			}
		}
		CloseServiceHandle( hsrvmanager );
		return( 0 );
	}
	CloseServiceHandle( hsrvmanager );
	CloseServiceHandle( hservice );

	// Everything went fine
	if( !_silent )
	{
		QMessageBox::information( NULL, szAppName,
			QApplication::tr(
				"The iTALC Client was successfully registered "
				"as a service. It will automatically be run "
				"the next time you reboot this computer." ) );
	}

	return( 0 );
}



// SERVICE REMOVE ROUTINE
int icaServiceRemove(bool _silent)
{
	// Open the SCM
	SC_HANDLE hsrvmanager = OpenSCManager(
					NULL,	// machine (NULL == local)
					NULL,	// database (NULL == default)
				SC_MANAGER_ALL_ACCESS	// access required
							);
	if( hsrvmanager )
	{ 
		SC_HANDLE hservice = OpenService( hsrvmanager, ICA_SERVICENAME,
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
					if( !_silent )
					{
	QMessageBox::critical( NULL, szAppName,
		QApplication::tr( "The iTALC Client service could not be "
								"stopped." ) );
					}
				}
			}

			// Now remove the service from the SCM
			if( DeleteService( hservice ) )
			{
				if( !_silent )
				{
		QMessageBox::information( NULL, szAppName,
			QApplication::tr( "The iTALC Client service has been "
							"unregistered." ) );
				}
			}
			else
			{
				DWORD error = GetLastError();
				if( error == ERROR_SERVICE_MARKED_FOR_DELETE )
				{
					if( !_silent )
					{
	QMessageBox::critical( NULL, szAppName,
		QApplication::tr( "The iTALC Client isn't registered as "
					"service and therefore can't be "
							"unregistered." ) );
					}
				}
				else
				{
					if( !_silent )
					{
	QMessageBox::critical( NULL, szAppName,
		QApplication::tr( "The iTALC Client service could not be "
							"unregistered." ) );
					}
				}
			}
			CloseServiceHandle( hservice );
		}
		else if( !_silent )
		{
	QMessageBox::critical( NULL, szAppName,
		QApplication::tr( "The iTALC Client service could not be "
								"found." ) );
		}
		CloseServiceHandle( hsrvmanager );
	}
	else if( !_silent )
	{
		QMessageBox::critical( NULL, szAppName,
			QApplication::tr( "The Service Control Manager could "
						"not be contacted (do you have "
						"the neccessary rights?!) - "
						"the iTALC Client "
						"service was not stopped." ) );
	}
	return 0;
}




// Service control routine
void WINAPI ServiceCtrl( DWORD ctrlcode )
{
	// What control code have we been sent?
	switch( ctrlcode )
	{

		case SERVICE_CONTROL_STOP:
			// STOP : The service must stop
			g_srvstatus.dwCurrentState = SERVICE_STOP_PENDING;
			ServiceStop();
			break;

		case SERVICE_CONTROL_INTERROGATE:
			// Service control manager just wants to know our state
			break;

		default:
			// Control code not recognised
			break;
	}

	// Tell the control manager what we're up to.
	ReportStatus( g_srvstatus.dwCurrentState, NO_ERROR, 0 );
}



// Service manager status reporting
BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint )
{
	static DWORD checkpoint = 1;
	BOOL result = TRUE;

	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
  	if( state == SERVICE_START_PENDING )
	{
		g_srvstatus.dwControlsAccepted = 0;
	}
	else
	{
		g_srvstatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	// Save the new status we've been given
	g_srvstatus.dwCurrentState = state;
	g_srvstatus.dwWin32ExitCode = exitcode;
	g_srvstatus.dwWaitHint = waithint;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if( ( state == SERVICE_RUNNING ) || ( state == SERVICE_STOPPED ) )
	{
		g_srvstatus.dwCheckPoint = 0;
	}
	else
	{
		g_srvstatus.dwCheckPoint = checkpoint++;
	}

	// Tell the SCM our new status
	if( !( result = SetServiceStatus( g_hstatus, &g_srvstatus ) ) )
	{
		printf( "SetServiceStatus failed.\n" );
	}

	return( result );
}



#endif

