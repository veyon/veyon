/*
 * SystemConfigurationModifier.cpp - class for easy modification of iTALC-related
 *                                 settings in the operating system
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include "SystemConfigurationModifier.h"
#include "ImcCore.h"
#include "Logger.h"


bool SystemConfigurationModifier::setServiceAutostart( bool enabled )
{
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "icas", SERVICE_ALL_ACCESS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	if( !ChangeServiceConfig( hservice,
					SERVICE_NO_CHANGE,	// dwServiceType
					enabled ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
					SERVICE_NO_CHANGE,	// dwErrorControl
					NULL,	// lpBinaryPathName
					NULL,	// lpLoadOrderGroup
					NULL,	// lpdwTagId
					NULL,	// lpDependencies
					NULL,	// lpServiceStartName
					NULL,	// lpPassword
					NULL	// lpDisplayName
				) )
	{
		ilog_failed( "ChangeServiceConfig()" );
		CloseServiceHandle( hservice );
		CloseServiceHandle( hsrvmanager );

		return false;
	}

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );
#endif

	return true;
}




bool SystemConfigurationModifier::setServiceArguments( const QString &serviceArgs )
{
	bool err = false;
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "icas", SERVICE_ALL_ACCESS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	QString binaryPath = QString( "\"%1\" -service %2" ).
								arg( ImcCore::icaFilePath() ).
								arg( serviceArgs );

	if( !ChangeServiceConfig( hservice,
					SERVICE_NO_CHANGE,	// dwServiceType
					SERVICE_NO_CHANGE,	// dwStartType
					SERVICE_NO_CHANGE,	// dwErrorControl
					binaryPath.toUtf8().constData(),	// lpBinaryPathName
					NULL,	// lpLoadOrderGroup
					NULL,	// lpdwTagId
					NULL,	// lpDependencies
					NULL,	// lpServiceStartName
					NULL,	// lpPassword
					NULL	// lpDisplayName
				) )
	{
		ilog_failed( "ChangeServiceConfig()" );
		err = true;
	}

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );
#endif

	return !err;
}


bool SystemConfigurationModifier::enableFirewallException( bool enabled )
{
}

