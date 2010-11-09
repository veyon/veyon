/*
 * ItalcConfiguration.h - a Configuration object storing system wide
 *                        configuration values
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
	ItalcConfiguration( Configuration::Store::Backend backend =
										Configuration::Store::LocalBackend );
	ItalcConfiguration( Configuration::Store *store );
	ItalcConfiguration( const ItalcConfiguration &ref );

	static ItalcConfiguration defaultConfiguration();

	static QString expandPath( QString path );


#define FOREACH_ITALC_CONFIG_PROPERTY(OP)												\
		/* iTALC Service */																\
		OP( ItalcConfiguration, BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service" );			\
		OP( ItalcConfiguration, BOOL, autostartService, setServiceAutostart, "Autostart", "Service" );			\
		OP( ItalcConfiguration, STRING, serviceArguments, setServiceArguments, "Arguments", "Service" );			\
		/* Logging */																	\
		OP( ItalcConfiguration, INT, logLevel, setLogLevel, "LogLevel", "Logging" );								\
		OP( ItalcConfiguration, BOOL, limittedLogFileSize, setLimittedLogFileSize, "LimittedLogFileSize", "Logging" );	\
		OP( ItalcConfiguration, INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging" );		\
		OP( ItalcConfiguration, STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging" );		\
		/* VNC Server */																\
		OP( ItalcConfiguration, BOOL, vncCaptureLayeredWindows, setVncCaptureLayeredWindows, "CaptureLayeredWindows", "VNC" );	\
		OP( ItalcConfiguration, BOOL, vncPollFullScreen, setVncPollFullScreen, "PollFullScreen", "VNC" );			\
		OP( ItalcConfiguration, BOOL, vncLowAccuracy, setVncLowAccuracy, "LowAccuracy", "VNC" );					\
		/* Demo server */																\
		OP( ItalcConfiguration, BOOL, isDemoServerMultithreaded, setDemoServerMultithreaded, "Multithreaded", "DemoServer" );		\
		/* Network */																	\
		OP( ItalcConfiguration, INT, coreServerPort, setCoreServerPort, "CoreServerPort", "Network" );			\
		OP( ItalcConfiguration, INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network" );			\
		OP( ItalcConfiguration, BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network" );	\
		/* Configuration file paths */													\
		OP( ItalcConfiguration, STRING, globalConfigurationPath, setGlobalConfigurationPath, "GlobalConfiguration", "Paths" );	\
		OP( ItalcConfiguration, STRING, personalConfigurationPath, setPersonalConfigurationPath, "PersonalConfiguration", "Paths" );	\
		/* Data directories */															\
		OP( ItalcConfiguration, STRING, snapshotDirectory, setSnapshotDirectory, "SnapshotDirectory", "Paths" );	\
		/* Authentication */															\
		OP( ItalcConfiguration, BOOL, isKeyAuthenticationEnabled, setKeyAuthenticationEnabled, "KeyAuthenticationEnabled", "Authentication" );	\
		OP( ItalcConfiguration, BOOL, isLogonAuthenticationEnabled, setLogonAuthenticationEnabled, "LogonAuthenticationEnabled", "Authentication" );	\
		OP( ItalcConfiguration, STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication" );	\
		OP( ItalcConfiguration, STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication" );	\

	FOREACH_ITALC_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

	// unluckily we have to declare slots manually as Qt's MOC doesn't do any
	// macro expansion :-(
public slots:
	void setTrayIconHidden( bool );
	void setServiceAutostart( bool );
	void setServiceArguments( const QString & );
	void setLogLevel( int );
	void setLimittedLogFileSize( bool );
	void setLogFileSizeLimit( int );
	void setLogFileDirectory( const QString & );
	void setVncCaptureLayeredWindows( bool );
	void setVncPollFullScreen( bool );
	void setVncLowAccuracy( bool );
	void setDemoServerMultithreaded( bool );
	void setCoreServerPort( int );
	void setDemoServerPort( int );
	void setFirewallExceptionEnabled( bool );
	void setGlobalConfigurationPath( const QString & );
	void setPersonalConfigurationPath( const QString & );
	void setSnapshotDirectory( const QString & );
	void setKeyAuthenticationEnabled( bool );
	void setLogonAuthenticationEnabled( bool );
	void setPrivateKeyBaseDir( const QString & );
	void setPublicKeyBaseDir( const QString & );

} ;

#endif
