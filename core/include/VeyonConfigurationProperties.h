/*
 * VeyonConfigurationProperties.h - definition of every configuration property
 *                                  stored in global veyon configuration
 *
 * Copyright (c) 2016-2018 Tobias Junghans <tobydox@veyon.io>
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
 * License along with this program (see COPYING) if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QDir>

#include "Logger.h"
#include "NetworkObjectDirectory.h"

#define FOREACH_VEYON_CORE_CONFIG_PROPERTIES(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), JSONOBJECT, pluginVersions, setPluginVersions, "PluginVersions", "Core", QVariant() )			\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, installationID, setInstallationID, "InstallationID", "Core", QString() )			\

#define FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, applicationName, setApplicationName, "ApplicationName", "UI", QStringLiteral("Veyon") )			\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, uiLanguage, setUiLanguage, "Language", "UI", QString() )

#define FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service", false )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, failedAuthenticationNotificationsEnabled, setFailedAuthenticationNotificationsEnabled, "FailedAuthenticationNotifications", "Service", true )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, remoteConnectionNotificationsEnabled, setRemoteConnectionNotificationsEnabled, "RemoteConnectionNotifications", "Service", false )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isMultiSessionServiceEnabled, setMultiSessionServiceEnabled, "MultiSession", "Service", false )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autostartService, setServiceAutostart, "Autostart", "Service", true )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isSoftwareSASEnabled, setSoftwareSASEnabled, "SoftwareSASEnabled", "Service", true )			\

#define FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, networkObjectDirectoryPlugin, setNetworkObjectDirectoryPlugin, "Plugin", "NetworkObjectDirectory", QUuid() )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory", NetworkObjectDirectory::DefaultUpdateInterval )			\

#define FOREACH_VEYON_FEATURES_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), STRINGLIST, disabledFeatures, setDisabledFeatures, "DisabledFeatures", "Features", QStringList() )			\

#define FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, logLevel, setLogLevel, "LogLevel", "Logging", Logger::LogLevelDefault )								\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logFileSizeLimitEnabled, setLogFileSizeLimitEnabled, "LogFileSizeLimitEnabled", "Logging", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logFileRotationEnabled, setLogFileRotationEnabled, "LogFileRotationEnabled", "Logging", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging", true )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logToSystem, setLogToSystem, "LogToSystem", "Logging", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging", Logger::DefaultFileSizeLimit )		\
	OP( VeyonConfiguration, VeyonCore::config(), INT, logFileRotationCount, setLogFileRotationCount, "LogFileRotationCount", "Logging", Logger::DefaultFileRotationCount )		\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging", QLatin1String(Logger::DefaultLogFileDirectory) )		\

#define FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), UUID, vncServerPlugin, setVncServerPlugin, "Plugin", "VncServer", QUuid() )	\

#define FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, primaryServicePort, setPrimaryServicePort, "PrimaryServicePort", "Network", 11100 )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, vncServerPort, setVncServerPort, "VncServerPort", "Network", 11200 )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, featureWorkerManagerPort, setFeatureWorkerManagerPort, "FeatureWorkerManagerPort", "Network", 11300 )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network", 11400 )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network", true )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network", false )					\

#define FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), STRING, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Config" ) ) )	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, screenshotDirectory, setScreenshotDirectory, "Screenshots", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Screenshots" ) ) )	\

#define FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, computerMonitoringUpdateInterval, setComputerMonitoringUpdateInterval, "ComputerMonitoringUpdateInterval", "Master", 1000 )	\
	OP( VeyonConfiguration, VeyonCore::config(), INT, computerDisplayRoleContent, setComputerDisplayRoleContent, "ComputerDisplayRoleContent", "Master", QVariant() )	\
	OP( VeyonConfiguration, VeyonCore::config(), COLOR, computerMonitoringBackgroundColor, setComputerMonitoringBackgroundColor, "ComputerMonitoringBackgroundColor", "Master", QColor(Qt::white) )	\
	OP( VeyonConfiguration, VeyonCore::config(), COLOR, computerMonitoringTextColor, setComputerMonitoringTextColor, "ComputerMonitoringTextColor", "Master", QColor(Qt::black) )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, accessControlForMasterEnabled, setAccessControlForMasterEnabled, "AccessControlForMasterEnabled", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autoAdjustGridSize, setAutoAdjustGridSize, "AutoAdjustGridSize", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autoSwitchToCurrentRoom, setAutoSwitchToCurrentRoom, "AutoSwitchToCurrentRoom", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, onlyCurrentRoomVisible, setOnlyCurrentRoomVisible, "OnlyCurrentRoomVisible", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, manualRoomAdditionAllowed, setManualRoomAdditionAllowed, "ManualRoomAdditionAllowed", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, localComputerHidden, setLocalComputerHidden, "LocalComputerHidden", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, emptyRoomsHidden, setEmptyRoomsHidden, "EmptyRoomsHidden", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, computerFilterHidden, setComputerFilterHidden, "ComputerFilterHidden", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master", QUuid() )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, enforceSelectedModeForClients, setEnforceSelectedModeForClients, "EnforceSelectedModeForClients", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, openComputerManagementAtStart, setOpenComputerManagementAtStart, "OpenComputerManagementAtStart", "Master", false )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, confirmDangerousActions, setConfirmDangerousActions, "ConfirmDangerousActions", "Master", false )	\

#define FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, authenticationMethod, setAuthenticationMethod, "Method", "Authentication", VeyonCore::LogonAuthentication )	\

#define FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication", QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/private" ) ) )	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication", QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/public" ) ) )	\

#define FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, accessControlUserGroupsBackend, setAccessControlUserGroupsBackend, "UserGroupsBackend", "AccessControl", QUuid())		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, domainGroupsForAccessControlEnabled, setDomainGroupsForAccessControlEnabled, "DomainGroupsEnabled", "AccessControl", false)		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl", false)		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl", false)	\
	OP( VeyonConfiguration, VeyonCore::config(), STRINGLIST, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl", QStringList() )	\
	OP( VeyonConfiguration, VeyonCore::config(), JSONARRAY, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl", QVariant() )	\

#define FOREACH_VEYON_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_CORE_CONFIG_PROPERTIES(OP)			\
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
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)
