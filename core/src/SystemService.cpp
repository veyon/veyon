/*
 * SystemService.h - class for managing a system service
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

#include "SystemService.h"

SystemService::SystemService( const QString& name ) :
	m_name( name )
{
#ifdef VEYON_BUILD_WIN32
	m_serviceHandle = nullptr;

	m_serviceManager = OpenSCManager( nullptr, nullptr, SC_MANAGER_ALL_ACCESS );

	if( m_serviceManager )
	{
		m_serviceHandle = OpenService( m_serviceManager, (LPCWSTR) m_name.utf16(), SERVICE_ALL_ACCESS );

		if( m_serviceHandle == nullptr )
		{
			qCritical( "SystemService: '%s' could not be found.", qPrintable( m_name ) );
		}
	}
	else
	{
		qCritical( "SystemService: the Service Control Manager could not be contacted - service '%s' hasn't been started.", qPrintable( m_name) );
	}
#endif
}



SystemService::~SystemService()
{
#ifdef VEYON_BUILD_WIN32
	CloseServiceHandle( m_serviceHandle );
	CloseServiceHandle( m_serviceManager );
#endif
}



bool SystemService::isRunning()
{
#ifdef VEYON_BUILD_LINUX
	return false;
#endif

#ifdef VEYON_BUILD_WIN32
	SERVICE_STATUS status;
	if( QueryServiceStatus( m_serviceHandle, &status ) )
	{
		return status.dwCurrentState == SERVICE_RUNNING;
	}

	return false;
#endif

	return false;
}



bool SystemService::start()
{
#ifdef VEYON_BUILD_LINUX
	return true;
#endif

#ifdef VEYON_BUILD_WIN32
	if( m_serviceManager == nullptr || m_serviceHandle == nullptr )
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
		qWarning( "SystemService: '%s' could not be started.", qPrintable( m_name ) );
		return false;
	}

	return true;
#endif

	return false;
}




bool SystemService::stop()
{
#ifdef VEYON_BUILD_LINUX
	return true;
#endif

#ifdef VEYON_BUILD_WIN32
	if( m_serviceManager == nullptr || m_serviceHandle == nullptr )
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
			qWarning( "SystemService: '%s' could not be stopped.", qPrintable( m_name ) );
			return false;
		}
	}

	return true;
#endif

	return false;
}
