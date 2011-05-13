/*
 * ItalcConfiguration.h - a Configuration object storing system wide
 *                        configuration values
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QStringList>

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
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service" );			\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, lockWithDesktopSwitching, setLockWithDesktopSwitching, "LockWithDesktopSwitching", "Service" );			\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, autostartService, setServiceAutostart, "Autostart", "Service" );			\
		OP( ItalcConfiguration, ItalcCore::config, STRING, serviceArguments, setServiceArguments, "Arguments", "Service" );			\
		/* Logging */																	\
		OP( ItalcConfiguration, ItalcCore::config, INT, logLevel, setLogLevel, "LogLevel", "Logging" );								\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, limittedLogFileSize, setLimittedLogFileSize, "LimittedLogFileSize", "Logging" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, logToWindowsEventLog, setLogToWindowsEventLog, "LogToWindowsEventLog", "Logging" );	\
		OP( ItalcConfiguration, ItalcCore::config, INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging" );		\
		OP( ItalcConfiguration, ItalcCore::config, STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging" );		\
		/* VNC Server */																\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, vncCaptureLayeredWindows, setVncCaptureLayeredWindows, "CaptureLayeredWindows", "VNC" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, vncPollFullScreen, setVncPollFullScreen, "PollFullScreen", "VNC" );			\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, vncLowAccuracy, setVncLowAccuracy, "LowAccuracy", "VNC" );					\
		/* Demo server */																\
		OP( ItalcConfiguration, ItalcCore::config, INT, demoServerBackend, setDemoServerBackend, "Backend", "DemoServer" );		\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isDemoServerMultithreaded, setDemoServerMultithreaded, "Multithreaded", "DemoServer" );		\
		/* Network */																	\
		OP( ItalcConfiguration, ItalcCore::config, INT, coreServerPort, setCoreServerPort, "CoreServerPort", "Network" );			\
		OP( ItalcConfiguration, ItalcCore::config, INT, httpServerPort, setHttpServerPort, "HttpServerPort", "Network" );			\
		OP( ItalcConfiguration, ItalcCore::config, INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network" );			\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isHttpServerEnabled, setHttpServerEnabled, "HttpServerEnabled", "Network" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network" );					\
		/* Configuration file paths */													\
		OP( ItalcConfiguration, ItalcCore::config, STRING, globalConfigurationPath, setGlobalConfigurationPath, "GlobalConfiguration", "Paths" );	\
		OP( ItalcConfiguration, ItalcCore::config, STRING, personalConfigurationPath, setPersonalConfigurationPath, "PersonalConfiguration", "Paths" );	\
		/* Data directories */															\
		OP( ItalcConfiguration, ItalcCore::config, STRING, snapshotDirectory, setSnapshotDirectory, "SnapshotDirectory", "Paths" );	\
		/* Authentication */															\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isKeyAuthenticationEnabled, setKeyAuthenticationEnabled, "KeyAuthenticationEnabled", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isLogonAuthenticationEnabled, setLogonAuthenticationEnabled, "LogonAuthenticationEnabled", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isPermissionRequiredWithKeyAuthentication, setPermissionRequiredWithKeyAuthentication, "PermissionRequiredWithKeyAuthentication", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isPermissionRequiredWithLogonAuthentication, setPermissionRequiredWithLogonAuthentication, "PermissionRequiredWithLogonAuthentication", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, BOOL, isSameUserConfirmationDisabled, setSameUserConfirmationDisabled, "SameUserConfirmationDisabled", "Authentication" );	\
		OP( ItalcConfiguration, ItalcCore::config, STRINGLIST, logonGroups, setLogonGroups, "LogonGroups", "Authentication" );	\

	FOREACH_ITALC_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

	// unluckily we have to declare slots manually as Qt's MOC doesn't do any
	// macro expansion :-(
public slots:
	void setTrayIconHidden( bool );
	void setLockWithDesktopSwitching( bool );
	void setServiceAutostart( bool );
	void setServiceArguments( const QString & );
	void setLogLevel( int );
	void setLogToStdErr( bool );
	void setLogToWindowsEventLog( bool );
	void setLimittedLogFileSize( bool );
	void setLogFileSizeLimit( int );
	void setLogFileDirectory( const QString & );
	void setVncCaptureLayeredWindows( bool );
	void setVncPollFullScreen( bool );
	void setVncLowAccuracy( bool );
	void setDemoServerBackend( int );
	void setDemoServerMultithreaded( bool );
	void setCoreServerPort( int );
	void setDemoServerPort( int );
	void setHttpServerPort( int );
	void setFirewallExceptionEnabled( bool );
	void setLocalConnectOnly( bool );
	void setHttpServerEnabled( bool );
	void setGlobalConfigurationPath( const QString & );
	void setPersonalConfigurationPath( const QString & );
	void setSnapshotDirectory( const QString & );
	void setKeyAuthenticationEnabled( bool );
	void setLogonAuthenticationEnabled( bool );
	void setPermissionRequiredWithKeyAuthentication( bool );
	void setPrivateKeyBaseDir( const QString & );
	void setPublicKeyBaseDir( const QString & );
	void setPermissionRequiredWithLogonAuthentication( bool );
	void setSameUserConfirmationDisabled( bool );
	void setLogonGroups( const QStringList & );

} ;

#endif
