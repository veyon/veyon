/*
 * ItalcConfigurationProperties.h - definition of every configuration property
 *                                  stored in global iTALC configuration
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef ITALC_CONFIGURATION_PROPERTIES_H
#define ITALC_CONFIGURATION_PROPERTIES_H

#define FOREACH_ITALC_UI_CONFIG_PROPERTY(OP)				\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, applicationName, setApplicationName, "ApplicationName", "UI" );			\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, uiLanguage, setUiLanguage, "Language", "UI" );			\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isHighDPIScalingEnabled, setHighDPIScalingEnabled, "EnableHighDPIScaling", "UI" );			\

#define FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service" );			\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, autostartService, setServiceAutostart, "Autostart", "Service" );			\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, serviceArguments, setServiceArguments, "Arguments", "Service" );			\

#define FOREACH_ITALC_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( ItalcConfiguration, ItalcCore::config(), UUID, networkObjectDirectoryPlugin, setNetworkObjectDirectoryPlugin, "Plugin", "NetworkObjectDirectory" );			\
	OP( ItalcConfiguration, ItalcCore::config(), INT, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory" );			\

#define FOREACH_ITALC_FEATURES_CONFIG_PROPERTY(OP)				\
	OP( ItalcConfiguration, ItalcCore::config(), STRINGLIST, disabledFeatures, setDisabledFeatures, "DisabledFeatures", "Features" );			\

#define FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config(), INT, logLevel, setLogLevel, "LogLevel", "Logging" );								\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, limittedLogFileSize, setLimittedLogFileSize, "LimittedLogFileSize", "Logging" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, logToWindowsEventLog, setLogToWindowsEventLog, "LogToWindowsEventLog", "Logging" );	\
	OP( ItalcConfiguration, ItalcCore::config(), INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging" );		\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging" );		\

#define FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config(), UUID, vncServerPlugin, setVncServerPlugin, "Plugin", "VncServer" );	\

#define FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config(), INT, computerControlServerPort, setComputerControlServerPort, "ComputerControlServerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config(), INT, vncServerPort, setVncServerPort, "VncServerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config(), INT, featureWorkerManagerPort, setFeatureWorkerManagerPort, "FeatureWorkerManagerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config(), INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network" );					\

#define FOREACH_ITALC_DIRECTORIES_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config(), STRING, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories" );	\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, snapshotDirectory, setSnapshotDirectory, "Snapshots", "Directories" );	\

#define FOREACH_ITALC_MASTER_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, autoSwitchToCurrentRoom, setAutoSwitchToCurrentRoom, "AutoSwitchToCurrentRoom", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, onlyCurrentRoomVisible, setOnlyCurrentRoomVisible, "OnlyCurrentRoomVisible", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, manualRoomAdditionAllowed, setManualRoomAdditionAllowed, "ManualRoomAdditionAllowed", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, localComputerHidden, setLocalComputerHidden, "LocalComputerHidden", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config(), UUID, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master" );	\

#define FOREACH_ITALC_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isKeyAuthenticationEnabled, setKeyAuthenticationEnabled, "KeyAuthenticationEnabled", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isLogonAuthenticationEnabled, setLogonAuthenticationEnabled, "LogonAuthenticationEnabled", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isPermissionRequiredWithKeyAuthentication, setPermissionRequiredWithKeyAuthentication, "PermissionRequiredWithKeyAuthentication", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config(), STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isPermissionRequiredWithLogonAuthentication, setPermissionRequiredWithLogonAuthentication, "PermissionRequiredWithLogonAuthentication", "Authentication" );	\

#define FOREACH_ITALC_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( ItalcConfiguration, ItalcCore::config(), UUID, accessControlDataBackend, setAccessControlDataBackend, "DataBackend", "AccessControl");		\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl");		\
	OP( ItalcConfiguration, ItalcCore::config(), BOOL, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl");	\
	OP( ItalcConfiguration, ItalcCore::config(), STRINGLIST, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl" );	\
	OP( ItalcConfiguration, ItalcCore::config(), JSONARRAY, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl" );	\

#define FOREACH_ITALC_CONFIG_PROPERTY(OP)				\
	FOREACH_ITALC_UI_CONFIG_PROPERTY(OP)				\
	FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(OP)			\
	FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(OP)			\
	FOREACH_ITALC_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)\
	FOREACH_ITALC_FEATURES_CONFIG_PROPERTY(OP)\
	FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(OP)		\
	FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(OP)			\
	FOREACH_ITALC_DIRECTORIES_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_MASTER_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_AUTHENTICATION_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_ACCESS_CONTROL_CONFIG_PROPERTY(OP)	\

#endif
