/*
 * ItalcConfiguration.h - a Configuration object storing everything
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

#ifndef _ITALC_CONFIGURATION_H
#define _ITALC_CONFIGURATION_H

#include "Configuration/Object.h"

class ItalcConfiguration : public Configuration::Object
{
	Q_OBJECT
public:
	ItalcConfiguration( Configuration::Store::Backend backend ) :
		Configuration::Object( backend, Configuration::Store::System )
	{
	}

	// iTALC Service

	MAP_CONFIG_BOOL_PROPERTY( isTrayIconHidden,
					setTrayIconHidden,
					"HideTrayIcon",
					"Service" );

	MAP_CONFIG_BOOL_PROPERTY( autostartService,
					setServiceAutostart,
					"Autostart",
					"Service" );

	MAP_CONFIG_PROPERTY( serviceArguments,
					setServiceArguments,
					"Arguments",
					"Service" );


	// Logging

	MAP_CONFIG_INT_PROPERTY( logLevel,
					setLogLevel,
					"LogLevel",
					"Logging" );

	MAP_CONFIG_BOOL_PROPERTY( limittedLogFileSize,
					setLimittedLogFileSize,
					"LimittedLogFileSize",
					"Logging" );

	MAP_CONFIG_INT_PROPERTY( logFileSizeLimit,
					setLogFileSizeLimit,
					"LogFileSizeLimit",
					"Logging" );

	MAP_CONFIG_PROPERTY( logFileDirectory,
					setLogFileDirectory,
					"LogFileDirectory",
					"Logging" );


	// VNC Server

	MAP_CONFIG_BOOL_PROPERTY( vncCaptureLayeredWindows,
					setVncCaptureLayeredWindows,
					"CaptureLayeredWindows",
					"VNC" );

	MAP_CONFIG_BOOL_PROPERTY( vncPollFullScreen,
					setVncPollFullScreen,
					"PollFullScreen",
					"VNC" );

	MAP_CONFIG_BOOL_PROPERTY( vncLowAccuracy,
					setVncLowAccuracy,
					"LowAccuracy",
					"VNC" );


	// Demo server

	MAP_CONFIG_BOOL_PROPERTY( isDemoServerMultithreaded,
					setDemoServerMultithreaded,
					"Multithreaded",
					"DemoServer" );


	// Network

	MAP_CONFIG_INT_PROPERTY( coreServerPort,
					setCoreServerPort,
					"CoreServerPort",
					"Network" );

	MAP_CONFIG_INT_PROPERTY( demoServerPort,
					setDemoServerPort,
					"DemoServerPort",
					"Network" );

	MAP_CONFIG_BOOL_PROPERTY( isFirewallExceptionEnabled,
					setFirewallExceptionEnabled,
					"FirewallExceptionEnabled",
					"Network" );


	// Configuration file paths

	MAP_CONFIG_PROPERTY( globalConfigurationPath,
					setGlobalConfigurationPath,
					"GlobalConfiguration",
					"Paths" );

	MAP_CONFIG_PROPERTY( personalConfigurationPath,
					setPersonalConfigurationPath,
					"PersonalConfiguration",
					"Paths" );


	// Data directories

	MAP_CONFIG_PROPERTY( snapshotDirectory,
					setSnapshotDirectory,
					"SnapshotDirectory",
					"Paths" );


	// Authentication

	MAP_CONFIG_BOOL_PROPERTY( isAuthenticationKeyBased,
					setAuthenticationKeyBased,
					"KeyBased",
					"Authentication" );

	MAP_CONFIG_BOOL_PROPERTY( isAuthenticationLogonBased,
					setAuthenticationLogonBased,
					"LogonBased",
					"Authentication" );

} ;

#endif
