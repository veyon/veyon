/*
 * WindowsService.h - class for managing a Windows service
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "WindowsService.h"

WindowsService::WindowsService( const QString& name ) :
    m_name( name ),
    m_serviceManager( nullptr ),
    m_serviceHandle( nullptr )
{
	m_serviceManager = OpenSCManager( nullptr, nullptr, SC_MANAGER_ALL_ACCESS );

	if( m_serviceManager )
	{
		m_serviceHandle = OpenService( m_serviceManager, (LPCWSTR) m_name.utf16(), SERVICE_ALL_ACCESS );
	}
	else
	{
		qCritical( "WindowsService: the Service Control Manager could not be contacted - service '%s' hasn't been started.", qPrintable( m_name) );
	}
}



WindowsService::~WindowsService()
{
	CloseServiceHandle( m_serviceHandle );
	CloseServiceHandle( m_serviceManager );
}



bool WindowsService::isRegistered()
{
	return m_serviceHandle != nullptr;
}



bool WindowsService::isRunning()
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



bool WindowsService::start()
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
		qWarning( "WindowsService: '%s' could not be started.", qPrintable( m_name ) );
		return false;
	}

	return true;
}




bool WindowsService::stop()
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
			qWarning( "WindowsService: '%s' could not be stopped.", qPrintable( m_name ) );
			return false;
		}
	}

	return true;
}



bool WindowsService::install( const QString& filePath, const QString& arguments, const QString& displayName  )
{
	const auto serviceCmd = QStringLiteral("\"%1\" %2" ).arg( filePath, arguments );

	m_serviceHandle = CreateService(
	            m_serviceManager,		// SCManager database
	            (LPCWSTR) m_name.utf16(),	// name of service
	            (LPCWSTR) displayName.utf16(),// name to display
	            SERVICE_ALL_ACCESS,	// desired access
	            SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
	            // service type
	            SERVICE_AUTO_START,	// start type
	            SERVICE_ERROR_NORMAL,	// error control type
	            (LPCWSTR) serviceCmd.utf16(),		// service's binary
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
			qCritical( qPrintable( tr( "WindowsService: the service \"%1\" is already installed." ).arg( m_name ) ) );
		}
		else
		{
			qCritical( qPrintable( tr( "WindowsService: the service \"%1\" could not be installed." ).arg( m_name ) ) );
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
	qInfo( qPrintable( tr( "WindowsService: the service \"%1\" has been installed successfully." ).arg( m_name ) ) );

	return true;
}



bool WindowsService::uninstall()
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
		qCritical( qPrintable( tr( "WindowsService: the service \"%1\" could not be uninstalled." ).arg( m_name ) ) );
		return false;
	}

	qInfo( qPrintable( tr( "WindowsService: the service \"%1\" has been uninstalled successfully." ).arg( m_name ) ) );

	return true;
}



bool WindowsService::setFilePathAndArguments( const QString& filePath, const QString& arguments )
{
	if( checkService() == false )
	{
		return false;
	}

	const auto serviceCmd = QStringLiteral("\"%1\" %2" ).arg( filePath, arguments );

	if( ChangeServiceConfig( m_serviceHandle,
	                         SERVICE_NO_CHANGE,	// dwServiceType
	                         SERVICE_NO_CHANGE,	// dwStartType
	                         SERVICE_NO_CHANGE,	// dwErrorControl
	                         (wchar_t *) serviceCmd.utf16(),	// lpBinaryPathName
	                         NULL,	// lpLoadOrderGroup
	                         NULL,	// lpdwTagId
	                         NULL,	// lpDependencies
	                         NULL,	// lpServiceStartName
	                         NULL,	// lpPassword
	                         NULL	// lpDisplayName
	                         ) == false )
	{
		qCritical( qPrintable( tr( "WindowsService: the arguments of service \"%1\" could not be changed." ).arg( m_name ) ) );
		return false;
	}

	return true;
}



bool WindowsService::setStartType( int startType )
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
		qCritical( qPrintable( tr( "WindowsService: the start type of service \"%1\" could not be changed." ).arg( m_name ) ) );
		return false;
	}

	return true;
}



bool WindowsService::checkService() const
{
	if( m_serviceHandle == nullptr )
	{
		qCritical( qPrintable( tr( "WindowsService: service \"%1\" could not be found." ).arg( m_name ) ) );
		return false;
	}

	return true;
}
