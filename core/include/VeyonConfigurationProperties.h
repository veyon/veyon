/*
 * VeyonConfigurationProperties.h - definition of every configuration property
 *                                  stored in global veyon configuration
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
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
	OP( VeyonConfiguration, VeyonCore::config(), JSONOBJECT, pluginVersions, setPluginVersions, "PluginVersions", "Core", QVariant(), Configuration::Object::HiddenProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, installationID, setInstallationID, "InstallationID", "Core", QString(), Configuration::Object::HiddenProperty )			\

#define FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, applicationName, setApplicationName, "ApplicationName", "UI", QStringLiteral("Veyon"), Configuration::Object::HiddenProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, uiLanguage, setUiLanguage, "Language", "UI", QString(), Configuration::Object::StandardProperty )

#define FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service", false, Configuration::Object::AdvancedProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, failedAuthenticationNotificationsEnabled, setFailedAuthenticationNotificationsEnabled, "FailedAuthenticationNotifications", "Service", true, Configuration::Object::StandardProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, remoteConnectionNotificationsEnabled, setRemoteConnectionNotificationsEnabled, "RemoteConnectionNotifications", "Service", false, Configuration::Object::StandardProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isMultiSessionServiceEnabled, setMultiSessionServiceEnabled, "MultiSession", "Service", false, Configuration::Object::StandardProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autostartService, setServiceAutostart, "Autostart", "Service", true, Configuration::Object::StandardProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isSoftwareSASEnabled, setSoftwareSASEnabled, "SoftwareSASEnabled", "Service", true, Configuration::Object::AdvancedProperty )			\

#define FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, networkObjectDirectoryPlugin, setNetworkObjectDirectoryPlugin, "Plugin", "NetworkObjectDirectory", QUuid(), Configuration::Object::StandardProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory", NetworkObjectDirectory::DefaultUpdateInterval, Configuration::Object::StandardProperty )			\

#define FOREACH_VEYON_FEATURES_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), STRINGLIST, disabledFeatures, setDisabledFeatures, "DisabledFeatures", "Features", QStringList(), Configuration::Object::StandardProperty )			\

#define FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, logLevel, setLogLevel, "LogLevel", "Logging", Logger::LogLevelDefault, Configuration::Object::StandardProperty )								\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logFileSizeLimitEnabled, setLogFileSizeLimitEnabled, "LogFileSizeLimitEnabled", "Logging", false, Configuration::Object::AdvancedProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logFileRotationEnabled, setLogFileRotationEnabled, "LogFileRotationEnabled", "Logging", false, Configuration::Object::AdvancedProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging", true, Configuration::Object::AdvancedProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, logToSystem, setLogToSystem, "LogToSystem", "Logging", false, Configuration::Object::AdvancedProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging", Logger::DefaultFileSizeLimit, Configuration::Object::AdvancedProperty )		\
	OP( VeyonConfiguration, VeyonCore::config(), INT, logFileRotationCount, setLogFileRotationCount, "LogFileRotationCount", "Logging", Logger::DefaultFileRotationCount, Configuration::Object::AdvancedProperty )		\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging", QLatin1String(Logger::DefaultLogFileDirectory), Configuration::Object::StandardProperty )		\

#define FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), UUID, vncServerPlugin, setVncServerPlugin, "Plugin", "VncServer", QUuid(), Configuration::Object::StandardProperty )	\

#define FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, primaryServicePort, setPrimaryServicePort, "PrimaryServicePort", "Network", 11100, Configuration::Object::AdvancedProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, vncServerPort, setVncServerPort, "VncServerPort", "Network", 11200, Configuration::Object::AdvancedProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, featureWorkerManagerPort, setFeatureWorkerManagerPort, "FeatureWorkerManagerPort", "Network", 11300, Configuration::Object::AdvancedProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network", 11400, Configuration::Object::AdvancedProperty )			\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network", true, Configuration::Object::AdvancedProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network", false, Configuration::Object::AdvancedProperty )					\

#define FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), STRING, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Config" ) ), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, screenshotDirectory, setScreenshotDirectory, "Screenshots", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Screenshots" ) ), Configuration::Object::StandardProperty )	\

#define FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, computerMonitoringUpdateInterval, setComputerMonitoringUpdateInterval, "ComputerMonitoringUpdateInterval", "Master", 1000, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), INT, computerDisplayRoleContent, setComputerDisplayRoleContent, "ComputerDisplayRoleContent", "Master", QVariant(), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), INT, computerMonitoringSortOrder, setComputerMonitoringSortOrder, "ComputerMonitoringSortOrder", "Master", QVariant(), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), COLOR, computerMonitoringBackgroundColor, setComputerMonitoringBackgroundColor, "ComputerMonitoringBackgroundColor", "Master", QColor(Qt::white), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), COLOR, computerMonitoringTextColor, setComputerMonitoringTextColor, "ComputerMonitoringTextColor", "Master", QColor(Qt::black), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, accessControlForMasterEnabled, setAccessControlForMasterEnabled, "AccessControlForMasterEnabled", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autoAdjustGridSize, setAutoAdjustGridSize, "AutoAdjustGridSize", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, autoSwitchToCurrentRoom, setAutoSwitchToCurrentRoom, "AutoSwitchToCurrentRoom", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, onlyCurrentRoomVisible, setOnlyCurrentRoomVisible, "OnlyCurrentRoomVisible", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, manualRoomAdditionAllowed, setManualRoomAdditionAllowed, "ManualRoomAdditionAllowed", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, localComputerHidden, setLocalComputerHidden, "LocalComputerHidden", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, emptyRoomsHidden, setEmptyRoomsHidden, "EmptyRoomsHidden", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, computerFilterHidden, setComputerFilterHidden, "ComputerFilterHidden", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master", QUuid(), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, enforceSelectedModeForClients, setEnforceSelectedModeForClients, "EnforceSelectedModeForClients", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, openComputerManagementAtStart, setOpenComputerManagementAtStart, "OpenComputerManagementAtStart", "Master", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, confirmDangerousActions, setConfirmDangerousActions, "ConfirmDangerousActions", "Master", false, Configuration::Object::StandardProperty )	\

#define FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), INT, authenticationMethod, setAuthenticationMethod, "Method", "Authentication", VeyonCore::LogonAuthentication, Configuration::Object::StandardProperty )	\

#define FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication", QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/private" ) ), Configuration::Object::AdvancedProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication", QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/public" ) ), Configuration::Object::AdvancedProperty )	\

#define FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), UUID, accessControlUserGroupsBackend, setAccessControlUserGroupsBackend, "UserGroupsBackend", "AccessControl", QUuid(), Configuration::Object::StandardProperty )		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, domainGroupsForAccessControlEnabled, setDomainGroupsForAccessControlEnabled, "DomainGroupsEnabled", "AccessControl", false, Configuration::Object::StandardProperty )		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl", false , Configuration::Object::StandardProperty )		\
	OP( VeyonConfiguration, VeyonCore::config(), BOOL, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl", false, Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), STRINGLIST, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl", QStringList(), Configuration::Object::StandardProperty )	\
	OP( VeyonConfiguration, VeyonCore::config(), JSONARRAY, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl", QVariant(), Configuration::Object::StandardProperty )	\

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
