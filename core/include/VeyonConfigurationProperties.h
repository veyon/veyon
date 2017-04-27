/*
 * VeyonConfigurationProperties.h - definition of every configuration property
 *                                  stored in global veyon configuration
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef VEYON_CONFIGURATION_PROPERTIES_H
#define VEYON_CONFIGURATION_PROPERTIES_H

#define FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, applicationName, setApplicationName, "ApplicationName", "UI" );			\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, uiLanguage, setUiLanguage, "Language", "UI" );			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isHighDPIScalingEnabled, setHighDPIScalingEnabled, "EnableHighDPIScaling", "UI" );			\

#define FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service" );			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autostartService, setServiceAutostart, "Autostart", "Service" );			\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, serviceArguments, setServiceArguments, "Arguments", "Service" );			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isSoftwareSASEnabled, setSoftwareSASEnabled, "SoftwareSASEnabled", "Service" );			\

#define FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, networkObjectDirectoryPlugin, setNetworkObjectDirectoryPlugin, "Plugin", "NetworkObjectDirectory" );			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory" );			\

#define FOREACH_VEYON_FEATURES_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), STRINGLIST, disabledFeatures, setDisabledFeatures, "DisabledFeatures", "Features" );			\

#define FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, logLevel, setLogLevel, "LogLevel", "Logging" );								\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logFileSizeLimitEnabled, setLogFileSizeLimitEnabled, "LogFileSizeLimitEnabled", "Logging" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logToWindowsEventLog, setLogToWindowsEventLog, "LogToWindowsEventLog", "Logging" );	\
	OP( VeyonConfiguration, VeyonCore::config(), INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging" );		\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging" );		\

#define FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), UUID, vncServerPlugin, setVncServerPlugin, "Plugin", "VncServer" );	\

#define FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, computerControlServerPort, setComputerControlServerPort, "ComputerControlServerPort", "Network" );			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, vncServerPort, setVncServerPort, "VncServerPort", "Network" );			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, featureWorkerManagerPort, setFeatureWorkerManagerPort, "FeatureWorkerManagerPort", "Network" );			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network" );			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network" );					\

#define FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), STRING, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories" );	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, snapshotDirectory, setSnapshotDirectory, "Snapshots", "Directories" );	\

#define FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autoSwitchToCurrentRoom, setAutoSwitchToCurrentRoom, "AutoSwitchToCurrentRoom", "Master" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, onlyCurrentRoomVisible, setOnlyCurrentRoomVisible, "OnlyCurrentRoomVisible", "Master" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, manualRoomAdditionAllowed, setManualRoomAdditionAllowed, "ManualRoomAdditionAllowed", "Master" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, localComputerHidden, setLocalComputerHidden, "LocalComputerHidden", "Master" );	\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master" );	\

#define FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isKeyAuthenticationEnabled, setKeyAuthenticationEnabled, "KeyAuthenticationEnabled", "Authentication" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isLogonAuthenticationEnabled, setLogonAuthenticationEnabled, "LogonAuthenticationEnabled", "Authentication" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isPermissionRequiredWithKeyAuthentication, setPermissionRequiredWithKeyAuthentication, "PermissionRequiredWithKeyAuthentication", "Authentication" );	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication" );	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication" );	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isPermissionRequiredWithLogonAuthentication, setPermissionRequiredWithLogonAuthentication, "PermissionRequiredWithLogonAuthentication", "Authentication" );	\

#define FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, accessControlDataBackend, setAccessControlDataBackend, "DataBackend", "AccessControl");		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl");		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl");	\
	OP( VeyonConfiguration, VeyonCore::config(), STRINGLIST, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl" );	\
	OP( VeyonConfiguration, VeyonCore::config(), JSONARRAY, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl" );	\

#define FOREACH_VEYON_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)			\
	FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(OP)			\
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)\
	FOREACH_VEYON_FEATURES_CONFIG_PROPERTY(OP)\
	FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(OP)		\
	FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(OP)			\
	FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)	\

#endif
