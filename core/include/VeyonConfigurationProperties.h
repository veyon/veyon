/*
 * VeyonConfigurationProperties.h - definition of every configuration property
 *                                  stored in global veyon configuration
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QColor>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

#include "ComputerListModel.h"
#include "Logger.h"
#include "NetworkObjectDirectory.h"
#include "VncConnection.h"

#define FOREACH_VEYON_CORE_CONFIG_PROPERTIES(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), VeyonCore::ApplicationVersion, applicationVersion, setApplicationVersion, "ApplicationVersion", "Core", QVariant::fromValue(VeyonCore::ApplicationVersion::Version_4_0), Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), QJsonObject, pluginVersions, setPluginVersions, "PluginVersions", "Core", QVariant(), Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), QString, installationID, setInstallationID, "InstallationID", "Core", QString(), Configuration::Property::Flag::Hidden )			\

#define FOREACH_VEYON_VNC_CONNECTION_CONFIG_PROPERTIES(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, useCustomVncConnectionSettings, setUseCustomVncConnectionSettings, "UseCustomSettings", "VncConnection", false, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionThreadTerminationTimeout, setVncConnectionThreadTerminationTimeout, "ThreadTerminationTimeout", "VncConnection", VncConnection::DefaultThreadTerminationTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionConnectTimeout, setVncConnectionConnectTimeout, "ConnectTimeout", "VncConnection", VncConnection::DefaultConnectTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionReadTimeout, setVncConnectionReadTimeout, "ReadTimeout", "VncConnection", VncConnection::DefaultReadTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionRetryInterval, setVncConnectionRetryInterval, "ConnectionRetryInterval", "VncConnection", VncConnection::DefaultConnectionRetryInterval, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionMessageWaitTimeout, setVncConnectionMessageWaitTimeout, "MessageWaitTimeout", "VncConnection", VncConnection::DefaultMessageWaitTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionFastFramebufferUpdateInterval, setVncConnectionFastFramebufferUpdateInterval, "FastFramebufferUpdateInterval", "VncConnection", VncConnection::DefaultFastFramebufferUpdateInterval, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionFramebufferUpdateWatchdogTimeout, setVncConnectionFramebufferUpdateWatchdogTimeout, "FramebufferUpdateWatchdogTimeout", "VncConnection", VncConnection::DefaultFramebufferUpdateWatchdogTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionSocketKeepaliveIdleTime, setVncConnectionSocketKeepaliveIdleTime, "SocketKeepaliveIdleTime", "VncConnection", VncConnection::DefaultSocketKeepaliveIdleTime, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionSocketKeepaliveInterval, setVncConnectionSocketKeepaliveInterval, "SocketKeepaliveInterval", "VncConnection", VncConnection::DefaultSocketKeepaliveInterval, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionSocketKeepaliveCount, setVncConnectionSocketKeepaliveCount, "SocketKeepaliveCount", "VncConnection", VncConnection::DefaultSocketKeepaliveCount, Configuration::Property::Flag::Hidden )			\

#define FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), QString, applicationName, setApplicationName, "ApplicationName", "UI", QStringLiteral("Veyon"), Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), QString, uiLanguage, setUiLanguage, "Language", "UI", QString(), Configuration::Property::Flag::Standard )

#define FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service", false, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, failedAuthenticationNotificationsEnabled, setFailedAuthenticationNotificationsEnabled, "FailedAuthenticationNotifications", "Service", true, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, remoteConnectionNotificationsEnabled, setRemoteConnectionNotificationsEnabled, "RemoteConnectionNotifications", "Service", false, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, multiSessionModeEnabled, setMultiSessionModeEnabled, "MultiSession", "Service", false, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, maximumSessionCount, setMaximumSessionCount, "MaximumSessionCount", "Service", 100, Configuration::Property::Flag::Standard ) \
	OP( VeyonConfiguration, VeyonCore::config(), bool, autostartService, setServiceAutostart, "Autostart", "Service", true, Configuration::Property::Flag::Advanced )			\

#define FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, networkObjectDirectoryPlugin, setNetworkObjectDirectoryPlugin, "Plugin", "NetworkObjectDirectory", QUuid(), Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory", NetworkObjectDirectory::DefaultUpdateInterval, Configuration::Property::Flag::Standard )			\

#define FOREACH_VEYON_FEATURES_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), QStringList, disabledFeatures, setDisabledFeatures, "DisabledFeatures", "Features", QStringList(), Configuration::Property::Flag::Standard )			\

#define FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), Logger::LogLevel, logLevel, setLogLevel, "LogLevel", "Logging", QVariant::fromValue(Logger::LogLevel::Default), Configuration::Property::Flag::Standard )								\
	OP( VeyonConfiguration, VeyonCore::config(), bool, logFileSizeLimitEnabled, setLogFileSizeLimitEnabled, "LogFileSizeLimitEnabled", "Logging", false, Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, logFileRotationEnabled, setLogFileRotationEnabled, "LogFileRotationEnabled", "Logging", false, Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging", true, Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, logToSystem, setLogToSystem, "LogToSystem", "Logging", false, Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), int, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging", Logger::DefaultFileSizeLimit, Configuration::Property::Flag::Advanced )		\
	OP( VeyonConfiguration, VeyonCore::config(), int, logFileRotationCount, setLogFileRotationCount, "LogFileRotationCount", "Logging", Logger::DefaultFileRotationCount, Configuration::Property::Flag::Advanced )		\
	OP( VeyonConfiguration, VeyonCore::config(), QString, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging", QLatin1String(Logger::DefaultLogFileDirectory), Configuration::Property::Flag::Standard )		\

#define FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, vncServerPlugin, setVncServerPlugin, "Plugin", "VncServer", QUuid(), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), int, veyonServerPort, setVeyonServerPort, "VeyonServerPort", "Network", 11100, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncServerPort, setVncServerPort, "VncServerPort", "Network", 11200, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, featureWorkerManagerPort, setFeatureWorkerManagerPort, "FeatureWorkerManagerPort", "Network", 11300, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, demoServerPort, setDemoServerPort, "DemoServerPort", "Network", 11400, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network", true, Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network", false, Configuration::Property::Flag::Advanced )					\

#define FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), QString, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Config" ) ), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QString, screenshotDirectory, setScreenshotDirectory, "Screenshots", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Screenshots" ) ), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), int, computerMonitoringUpdateInterval, setComputerMonitoringUpdateInterval, "ComputerMonitoringUpdateInterval", "Master", 1000, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), int, computerMonitoringThumbnailSpacing, setComputerMonitoringThumbnailSpacing, "ComputerMonitoringThumbnailSpacing", "Master", 5, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::DisplayRoleContent, computerDisplayRoleContent, setComputerDisplayRoleContent, "ComputerDisplayRoleContent", "Master", QVariant::fromValue(ComputerListModel::DisplayRoleContent::UserAndComputerName), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::SortOrder, computerMonitoringSortOrder, setComputerMonitoringSortOrder, "ComputerMonitoringSortOrder", "Master", QVariant::fromValue(ComputerListModel::SortOrder::ComputerAndUserName), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::AspectRatio, computerMonitoringAspectRatio, setComputerMonitoringAspectRatio, "ComputerMonitoringAspectRatio", "Master", QVariant::fromValue(ComputerListModel::AspectRatio::Auto), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QColor, computerMonitoringBackgroundColor, setComputerMonitoringBackgroundColor, "ComputerMonitoringBackgroundColor", "Master", QColor(Qt::white), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QColor, computerMonitoringTextColor, setComputerMonitoringTextColor, "ComputerMonitoringTextColor", "Master", QColor(Qt::black), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, accessControlForMasterEnabled, setAccessControlForMasterEnabled, "AccessControlForMasterEnabled", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, autoAdjustMonitoringIconSize, setAutoAdjustMonitoringIconSize, "AutoAdjustMonitoringIconSize", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, autoSelectCurrentLocation, setAutoSelectCurrentLocation, "AutoSelectCurrentLocation", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, showCurrentLocationOnly, setShowCurrentLocationOnly, "ShowCurrentLocationOnly", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, allowAddingHiddenLocations, setAllowAddingHiddenLocations, "AllowAddingHiddenLocations", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideLocalComputer, setHideLocalComputer, "HideLocalComputer", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideOwnSession, setHideOwnSession, "HideOwnSession", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideEmptyLocations, setHideEmptyLocations, "HideEmptyLocations", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideComputerFilter, setHideComputerFilter, "HideComputerFilter", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master", QUuid(), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, enforceSelectedModeForClients, setEnforceSelectedModeForClients, "EnforceSelectedModeForClients", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, autoOpenComputerSelectPanel, setAutoOpenComputerSelectPanel, "AutoOpenComputerSelectPanel", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, confirmUnsafeActions, setConfirmUnsafeActions, "ConfirmUnsafeActions", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, showFeatureWindowsOnSameScreen, setShowFeatureWindowsOnSameScreen, "ShowFeatureWindowsOnSameScreen", "Master", false, Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), VeyonCore::AuthenticationMethod, authenticationMethod, setAuthenticationMethod, "Method", "Authentication", QVariant::fromValue(VeyonCore::AuthenticationMethod::LogonAuthentication), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), QString, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication", QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/private" ) ), Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), QString, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication", QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/public" ) ), Configuration::Property::Flag::Advanced )	\

#define FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, accessControlUserGroupsBackend, setAccessControlUserGroupsBackend, "UserGroupsBackend", "AccessControl", QUuid(), Configuration::Property::Flag::Standard )		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, domainGroupsForAccessControlEnabled, setDomainGroupsForAccessControlEnabled, "DomainGroupsEnabled", "AccessControl", false, Configuration::Property::Flag::Standard )		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl", false , Configuration::Property::Flag::Standard )		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QStringList, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl", QStringList(), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QJsonArray, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl", QVariant(), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_LEGACY_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyAutoAdjustGridSize, setLegacyAutoAdjustGridSize, "AutoAdjustGridSize", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyOpenComputerManagementAtStart, setLegacyOpenComputerManagementAtStart, "OpenComputerManagementAtStart", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyAutoSwitchToCurrentRoom, setLegacyAutoSwitchToCurrentRoom, "AutoSwitchToCurrentRoom", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyOnlyCurrentRoomVisible, setLegacyOnlyCurrentRoomVisible, "OnlyCurrentRoomVisible", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyManualRoomAdditionAllowed, setLegacyManualRoomAdditionAllowed, "ManualRoomAdditionAllowed", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyEmptyRoomsHidden, setLegacyEmptyRoomsHidden, "EmptyRoomsHidden", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyConfirmDangerousActions, setLegacyConfirmDangerousActions, "ConfirmDangerousActions", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyIsSoftwareSASEnabled, setLegacySoftwareSASEnabled, "SoftwareSASEnabled", "Service", true, Configuration::Property::Flag::Legacy )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyLocalComputerHidden, setLegacyLocalComputerHidden, "LocalComputerHidden", "Master", false, Configuration::Property::Flag::Legacy )       \
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyComputerFilterHidden, setLegacyComputerFilterHidden, "ComputerFilterHidden", "Master", false, Configuration::Property::Flag::Legacy )	\
	OP( VeyonConfiguration, VeyonCore::config(), int, legacyPrimaryServicePort, setLegacyPrimaryServicePort, "PrimaryServicePort", "Network", 11100, Configuration::Property::Flag::Legacy )			\

#define FOREACH_VEYON_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_CORE_CONFIG_PROPERTIES(OP)			\
	FOREACH_VEYON_VNC_CONNECTION_CONFIG_PROPERTIES(OP)	\
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
	FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP) \
	FOREACH_VEYON_LEGACY_CONFIG_PROPERTY(OP)
