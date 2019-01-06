/*
 * WindowsServiceControl.h - class for managing a Windows service
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "WindowsCoreFunctions.h"
#include "WindowsServiceControl.h"


WindowsServiceControl::WindowsServiceControl( const QString& name ) :
	m_name( name ),
	m_serviceManager( nullptr ),
	m_serviceHandle( nullptr )
{
	m_serviceManager = OpenSCManager( nullptr, nullptr, SC_MANAGER_ALL_ACCESS );

	if( m_serviceManager )
	{
		m_serviceHandle = OpenService( m_serviceManager, WindowsCoreFunctions::toConstWCharArray( m_name ), SERVICE_ALL_ACCESS );
	}
	else
	{
		qCritical( "WindowsServiceControl: the Service Control Manager could not be contacted - service '%s' hasn't been started.", qUtf8Printable( m_name) );
	}
}



WindowsServiceControl::~WindowsServiceControl()
{
	CloseServiceHandle( m_serviceHandle );
	CloseServiceHandle( m_serviceManager );
}



bool WindowsServiceControl::isRegistered()
{
	return m_serviceHandle != nullptr;
}



bool WindowsServiceControl::isRunning()
{
	if( checkService() == false )
	{
		return false;
	}

	SERVICE_STATUS status;
	if( QueryServiceStatus( m_serviceHandle, &status ) )
	{
		return status.dwCurrentState == SERVICE_RUNNING;
	}

	return false;
}



bool WindowsServiceControl::start()
{
	if( checkService() == false )
	{
		return false;
	}

	SERVICE_STATUS status;
	status.dwCurrentState = SERVICE_START_PENDING;

	if( StartService( m_serviceHandle, 0, NULL ) )
	{
		while( QueryServiceStatus( m_serviceHandle, &status ) )
		{
			if( status.dwCurrentState == SERVICE_START_PENDING )
			{
				Sleep( 1000 );
			}
			else
			{
				break;
			}
		}
	}

	if( status.dwCurrentState != SERVICE_RUNNING )
	{
		qWarning( "WindowsServiceControl: '%s' could not be started.", qUtf8Printable( m_name ) );
		return false;
	}

	return true;
}




bool WindowsServiceControl::stop()
{
	if( checkService() == false )
	{
		return false;
	}

	SERVICE_STATUS status;

	// Try to stop the service
	if( ControlService( m_serviceHandle, SERVICE_CONTROL_STOP, &status ) )
	{
		while( QueryServiceStatus( m_serviceHandle, &status ) )
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
			qWarning( "WindowsServiceControl: '%s' could not be stopped.", qUtf8Printable( m_name ) );
			return false;
		}
	}

	return true;
}



bool WindowsServiceControl::install( const QString& filePath, const QString& displayName  )
{
	m_serviceHandle = CreateService(
				m_serviceManager,		// SCManager database
				WindowsCoreFunctions::toConstWCharArray( m_name ),	// name of service
				WindowsCoreFunctions::toConstWCharArray( displayName ),// name to display
				SERVICE_ALL_ACCESS,	// desired access
				SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
				// service type
				SERVICE_AUTO_START,	// start type
				SERVICE_ERROR_NORMAL,	// error control type
				WindowsCoreFunctions::toConstWCharArray( filePath ),		// service's binary
				NULL,			// no load ordering group
				NULL,			// no tag identifier
				NULL,			// dependencies
				NULL,			// LocalSystem account
				NULL );			// no password

	if( m_serviceHandle == nullptr )
	{
		const auto error = GetLastError();
		if( error == ERROR_SERVICE_EXISTS )
		{
			qCritical( qUtf8Printable( tr( "WindowsServiceControl: the service \"%1\" is already installed." ).arg( m_name ) ) );
		}
		else
		{
			qCritical( qUtf8Printable( tr( "WindowsServiceControl: the service \"%1\" could not be installed." ).arg( m_name ) ) );
		}

		return false;
	}

	SC_ACTION serviceActions;
	serviceActions.Delay = 10000;
	serviceActions.Type = SC_ACTION_RESTART;

	SERVICE_FAILURE_ACTIONS serviceFailureActions;
	serviceFailureActions.dwResetPeriod = 0;
	serviceFailureActions.lpRebootMsg = NULL;
	serviceFailureActions.lpCommand = NULL;
	serviceFailureActions.lpsaActions = &serviceActions;
	serviceFailureActions.cActions = 1;
	ChangeServiceConfig2( m_serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &serviceFailureActions );

	// Everything went fine
	qInfo( qUtf8Printable( tr( "WindowsServiceControl: the service \"%1\" has been installed successfully." ).arg( m_name ) ) );

	return true;
}



bool WindowsServiceControl::uninstall()
{
	if( checkService() == false )
	{
		return false;
	}

	if( stop() == false )
	{
		return false;
	}

	if( DeleteService( m_serviceHandle ) == false )
	{
		qCritical( qUtf8Printable( tr( "WindowsServiceControl: the service \"%1\" could not be uninstalled." ).arg( m_name ) ) );
		return false;
	}

	qInfo( qUtf8Printable( tr( "WindowsServiceControl: the service \"%1\" has been uninstalled successfully." ).arg( m_name ) ) );

	return true;
}



bool WindowsServiceControl::setStartType( int startType )
{
	if( checkService() == false )
	{
		return false;
	}

	if( ChangeServiceConfig( m_serviceHandle,
							 SERVICE_NO_CHANGE,	// dwServiceType
							 startType,
							 SERVICE_NO_CHANGE,	// dwErrorControl
							 NULL,	// lpBinaryPathName
							 NULL,	// lpLoadOrderGroup
							 NULL,	// lpdwTagId
							 NULL,	// lpDependencies
							 NULL,	// lpServiceStartName
							 NULL,	// lpPassword
							 NULL	// lpDisplayName
							 ) == false )
	{
		qCritical( qUtf8Printable( tr( "WindowsServiceControl: the start type of service \"%1\" could not be changed." ).arg( m_name ) ) );
		return false;
	}

	return true;
}



bool WindowsServiceControl::checkService() const
{
	if( m_serviceHandle == nullptr )
	{
		qCritical( qUtf8Printable( tr( "WindowsServiceControl: service \"%1\" could not be found." ).arg( m_name ) ) );
		return false;
	}

	return true;
}
