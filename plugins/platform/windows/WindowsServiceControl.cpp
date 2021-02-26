/*
 * WindowsServiceControl.h - class for managing a Windows service
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
		m_serviceHandle = OpenService( m_serviceManager, WindowsCoreFunctions::toConstWCharArray( m_name ),
									   SERVICE_ALL_ACCESS );
		if( m_serviceHandle == nullptr )
		{
			vCritical() << "could not open service" << m_name;
		}
	}
	else
	{
		vCritical() << "the Service Control Manager could not be contacted - service " << m_name << "can't be controlled.";
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

	if( StartService( m_serviceHandle, 0, nullptr ) )
	{
		while( QueryServiceStatus( m_serviceHandle, &status ) )
		{
			if( status.dwCurrentState == SERVICE_START_PENDING )
			{
				Sleep( ServiceWaitSleepInterval );
			}
			else
			{
				break;
			}
		}
	}

	if( status.dwCurrentState != SERVICE_RUNNING )
	{
		vWarning() << "service" << m_name << "could not be started.";
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
				Sleep( ServiceWaitSleepInterval );
			}
			else
			{
				break;
			}
		}

		if( status.dwCurrentState != SERVICE_STOPPED )
		{
			vWarning() << "service" << m_name << "could not be stopped.";
			return false;
		}
	}

	return true;
}



bool WindowsServiceControl::install( const QString& filePath, const QString& displayName  )
{
	const auto binaryPath = QStringLiteral("\"%1\"").arg( QString( filePath ).replace( QLatin1Char('"'), QString() ) );

	m_serviceHandle = CreateService(
				m_serviceManager,		// SCManager database
				WindowsCoreFunctions::toConstWCharArray( m_name ),	// name of service
				WindowsCoreFunctions::toConstWCharArray( displayName ),// name to display
				SERVICE_ALL_ACCESS,	// desired access
				SERVICE_WIN32_OWN_PROCESS,
				// service type
				SERVICE_AUTO_START,	// start type
				SERVICE_ERROR_NORMAL,	// error control type
				WindowsCoreFunctions::toConstWCharArray( binaryPath ),		// service's binary
				nullptr,			// no load ordering group
				nullptr,			// no tag identifier
				L"Tcpip\0RpcSs\0\0",		// dependencies
				nullptr,			// LocalSystem account
				nullptr );			// no password

	if( m_serviceHandle == nullptr )
	{
		const auto error = GetLastError();
		if( error == ERROR_SERVICE_EXISTS )
		{
			vCritical() << qUtf8Printable( tr( "The service \"%1\" is already installed." ).arg( m_name ) );
		}
		else
		{
			vCritical() << qUtf8Printable( tr( "The service \"%1\" could not be installed." ).arg( m_name ) );
		}

		return false;
	}

	SC_ACTION serviceActions;
	serviceActions.Delay = ServiceActionDelay;
	serviceActions.Type = SC_ACTION_RESTART;

	SERVICE_FAILURE_ACTIONS serviceFailureActions{};
	serviceFailureActions.lpsaActions = &serviceActions;
	serviceFailureActions.cActions = 1;
	ChangeServiceConfig2( m_serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &serviceFailureActions );

	// Everything went fine
	vInfo() << qUtf8Printable( tr( "The service \"%1\" has been installed successfully." ).arg( m_name ) );

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
		vCritical() << qUtf8Printable( tr( "The service \"%1\" could not be uninstalled." ).arg( m_name ) );
		return false;
	}

	vInfo() << qUtf8Printable( tr( "The service \"%1\" has been uninstalled successfully." ).arg( m_name ) );

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
							 static_cast<DWORD>( startType ),
							 SERVICE_NO_CHANGE,	// dwErrorControl
							 nullptr,	// lpBinaryPathName
							 nullptr,	// lpLoadOrderGroup
							 nullptr,	// lpdwTagId
							 nullptr,	// lpDependencies
							 nullptr,	// lpServiceStartName
							 nullptr,	// lpPassword
							 nullptr	// lpDisplayName
							 ) == false )
	{
		vCritical() << qUtf8Printable( tr( "The start type of service \"%1\" could not be changed." ).arg( m_name ) );
		return false;
	}

	return true;
}



bool WindowsServiceControl::checkService() const
{
	if( m_serviceHandle == nullptr )
	{
		vCritical() << qUtf8Printable( tr( "Service \"%1\" could not be found." ).arg( m_name ) );
		return false;
	}

	return true;
}
