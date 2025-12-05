/*
 * VeyonConfigurationProperties.h - definition of every configuration property
 *                                  stored in global veyon configuration
 *
 * Copyright (c) 2016-2025 Tobias Junghans <tobydox@veyon.io>
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
#include "Computer.h"
#include "Logger.h"
#include "NetworkObjectDirectory.h"
#include "PlatformSessionFunctions.h"
#include "VncConnectionConfiguration.h"

#define FOREACH_VEYON_CORE_CONFIG_PROPERTIES(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), VeyonCore::ApplicationVersion, applicationVersion, setApplicationVersion, "ApplicationVersion", "Core", QVariant::fromValue(VeyonCore::ApplicationVersion::Version_4_0), Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), QJsonObject, pluginVersions, setPluginVersions, "PluginVersions", "Core", QJsonObject(), Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), QString, installationID, setInstallationID, "InstallationID", "Core", QString(), Configuration::Property::Flag::Hidden )			\
	OP(VeyonConfiguration, VeyonCore::config(), int, computerStatePollingInterval, setComputerStatePollingInterval, "ComputerStatePollingInterval", "Core", -1, Configuration::Property::Flag::Hidden)	\

#define FOREACH_VEYON_VNC_CONNECTION_CONFIG_PROPERTIES(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, useCustomVncConnectionSettings, setUseCustomVncConnectionSettings, "UseCustomSettings", "VncConnection", false, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionThreadTerminationTimeout, setVncConnectionThreadTerminationTimeout, "ThreadTerminationTimeout", "VncConnection", VncConnectionConfiguration::DefaultThreadTerminationTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionConnectTimeout, setVncConnectionConnectTimeout, "ConnectTimeout", "VncConnection", VncConnectionConfiguration::DefaultConnectTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionReadTimeout, setVncConnectionReadTimeout, "ReadTimeout", "VncConnection", VncConnectionConfiguration::DefaultReadTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionRetryInterval, setVncConnectionRetryInterval, "ConnectionRetryInterval", "VncConnection", VncConnectionConfiguration::DefaultConnectionRetryInterval, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionMessageWaitTimeout, setVncConnectionMessageWaitTimeout, "MessageWaitTimeout", "VncConnection", VncConnectionConfiguration::DefaultMessageWaitTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionFastFramebufferUpdateInterval, setVncConnectionFastFramebufferUpdateInterval, "FastFramebufferUpdateInterval", "VncConnection", VncConnectionConfiguration::DefaultFastFramebufferUpdateInterval, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionInitialFramebufferUpdateTimeout, setVncConnectionInitialFramebufferUpdateTimeout, "InitialFramebufferUpdateTimeout", "VncConnection", VncConnectionConfiguration::DefaultInitialFramebufferUpdateTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionFramebufferUpdateTimeout, setVncConnectionFramebufferUpdateTimeout, "FramebufferUpdateTimeout", "VncConnection", VncConnectionConfiguration::DefaultFramebufferUpdateTimeout, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionSocketKeepaliveIdleTime, setVncConnectionSocketKeepaliveIdleTime, "SocketKeepaliveIdleTime", "VncConnection", VncConnectionConfiguration::DefaultSocketKeepaliveIdleTime, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionSocketKeepaliveInterval, setVncConnectionSocketKeepaliveInterval, "SocketKeepaliveInterval", "VncConnection", VncConnectionConfiguration::DefaultSocketKeepaliveInterval, Configuration::Property::Flag::Hidden )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, vncConnectionSocketKeepaliveCount, setVncConnectionSocketKeepaliveCount, "SocketKeepaliveCount", "VncConnection", VncConnectionConfiguration::DefaultSocketKeepaliveCount, Configuration::Property::Flag::Hidden )			\

#define FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), QString, uiLanguage, setUiLanguage, "Language", "UI", QString(), Configuration::Property::Flag::Standard ) \
	OP( VeyonConfiguration, VeyonCore::config(), VeyonCore::UiStyle, uiStyle, setUiStyle, "Style", "UI", QVariant::fromValue(VeyonCore::UiStyle::Fusion), Configuration::Property::Flag::Standard ) \
	OP( VeyonConfiguration, VeyonCore::config(), VeyonCore::UiColorScheme, uiColorScheme, setUiColorScheme, "ColorScheme", "UI", QVariant::fromValue(VeyonCore::UiColorScheme::System), Configuration::Property::Flag::Standard )

#define FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service", false, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, failedAuthenticationNotificationsEnabled, setFailedAuthenticationNotificationsEnabled, "FailedAuthenticationNotifications", "Service", true, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, remoteConnectionNotificationsEnabled, setRemoteConnectionNotificationsEnabled, "RemoteConnectionNotifications", "Service", false, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, activeSessionModeEnabled, setActiveSessionModeEnabled, "ActiveSession", "Service", false, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, multiSessionModeEnabled, setMultiSessionModeEnabled, "MultiSession", "Service", false, Configuration::Property::Flag::Standard )			\
	OP( VeyonConfiguration, VeyonCore::config(), int, maximumSessionCount, setMaximumSessionCount, "MaximumSessionCount", "Service", 100, Configuration::Property::Flag::Standard ) \
	OP( VeyonConfiguration, VeyonCore::config(), bool, autostartService, setServiceAutostart, "Autostart", "Service", true, Configuration::Property::Flag::Advanced )			\
	OP( VeyonConfiguration, VeyonCore::config(), bool, clipboardSynchronizationDisabled, setClipboardSynchronizationDisabled, "ClipboardSynchronizationDisabled", "Service", false, Configuration::Property::Flag::Advanced )					\
	OP( VeyonConfiguration, VeyonCore::config(), PlatformSessionFunctions::SessionMetaDataContent, sessionMetaDataContent, setSessionMetaDataContent, "SessionMetaDataContent", "Service", QVariant::fromValue(PlatformSessionFunctions::SessionMetaDataContent::None), Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), QString, sessionMetaDataEnvironmentVariable, setSessionMetaDataEnvironmentVariable, "SessionMetaDataEnvironmentVariable", "Service", QString(), Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), QString, sessionMetaDataRegistryKey, setSessionMetaDataRegistryKey, "SessionMetaDataRegistryKey", "Service", QString(), Configuration::Property::Flag::Advanced )	\

#define FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), QStringList, enabledNetworkObjectDirectoryPlugins, setEnabledNetworkObjectDirectoryPlugins, "EnabledPlugins", "NetworkObjectDirectory", QStringList(), Configuration::Property::Flag::Standard ) \
	OP( VeyonConfiguration, VeyonCore::config(), int, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory", NetworkObjectDirectory::DefaultUpdateInterval, Configuration::Property::Flag::Standard )			\

#define FOREACH_VEYON_USER_GROUPS_BACKEND_CONFIG_PROPERTY(OP)				\
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, userGroupsBackend, setUserGroupsBackend, "Backend", "UserGroups", QUuid(), Configuration::Property::Flag::Standard )		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, useDomainUserGroups, setUseDomainUserGroups, "UseDomainUserGroups", "UserGroups", false, Configuration::Property::Flag::Standard )		\

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

#define FOREACH_VEYON_TLS_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), bool, tlsUseCertificateAuthority, setTlsUseCertificateAuthority, "UseCertificateAuthority", "TLS", false, Configuration::Property::Flag::Advanced ) \
	OP( VeyonConfiguration, VeyonCore::config(), QString, tlsCaCertificateFile, setTlsCaCertificateFile, "CaCertificateFile", "TLS", QStringLiteral("%GLOBALAPPDATA%/tls/ca.pem"), Configuration::Property::Flag::Standard )  \
	OP( VeyonConfiguration, VeyonCore::config(), QString, tlsHostCertificateFile, setTlsHostCertificateFile, "HostCertificateFile", "TLS", QStringLiteral("%GLOBALAPPDATA%/tls/%HOSTNAME%/cert.pem"), Configuration::Property::Flag::Standard )  \
	OP( VeyonConfiguration, VeyonCore::config(), QString, tlsHostPrivateKeyFile, setTlsHostPrivateKeyFile, "HostPrivateKeyFile", "TLS", QStringLiteral("%GLOBALAPPDATA%/tls/%HOSTNAME%/private.key"), Configuration::Property::Flag::Standard ) \

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
	OP( VeyonConfiguration, VeyonCore::config(), QString, configurationTemplatesDirectory, setConfigurationTemplatesDirectory, "ConfigurationTemplates", "Directories", QDir::toNativeSeparators(QStringLiteral("%GLOBALAPPDATA%/Config/Templates")), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QString, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Config" ) ), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QString, screenshotDirectory, setScreenshotDirectory, "Screenshots", "Directories", QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Screenshots" ) ), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), bool, modernUserInterface, setModernUserInterface, "ModernUserInterface", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), VncConnectionConfiguration::Quality, computerMonitoringImageQuality, setComputerMonitoringImageQuality, "ComputerMonitoringImageQuality", "Master", QVariant::fromValue(VncConnectionConfiguration::Quality::Medium), Configuration::Property::Flag::Standard )    \
	OP( VeyonConfiguration, VeyonCore::config(), VncConnectionConfiguration::Quality, remoteAccessImageQuality, setRemoteAccessImageQuality, "RemoteAccessImageQuality", "Master", QVariant::fromValue(VncConnectionConfiguration::Quality::Highest), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), int, computerMonitoringUpdateInterval, setComputerMonitoringUpdateInterval, "ComputerMonitoringUpdateInterval", "Master", 1000, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), int, computerMonitoringThumbnailSpacing, setComputerMonitoringThumbnailSpacing, "ComputerMonitoringThumbnailSpacing", "Master", 5, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::DisplayRoleContent, computerDisplayRoleContent, setComputerDisplayRoleContent, "ComputerDisplayRoleContent", "Master", QVariant::fromValue(ComputerListModel::DisplayRoleContent::UserAndComputerName), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::UidRoleContent, computerUidRoleContent, setComputerUidRoleContent, "ComputerUidRoleContent", "Master", QVariant::fromValue(ComputerListModel::UidRoleContent::NetworkObjectUid), Configuration::Property::Flag::Advanced)	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::SortOrder, computerMonitoringSortOrder, setComputerMonitoringSortOrder, "ComputerMonitoringSortOrder", "Master", QVariant::fromValue(ComputerListModel::SortOrder::ComputerAndUserName), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), ComputerListModel::AspectRatio, computerMonitoringAspectRatio, setComputerMonitoringAspectRatio, "ComputerMonitoringAspectRatio", "Master", QVariant::fromValue(ComputerListModel::AspectRatio::Auto), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QColor, computerMonitoringBackgroundColor, setComputerMonitoringBackgroundColor, "ComputerMonitoringBackgroundColor", "Master", QColor(Qt::white), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QColor, computerMonitoringTextColor, setComputerMonitoringTextColor, "ComputerMonitoringTextColor", "Master", QColor(Qt::black), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, accessControlForMasterEnabled, setAccessControlForMasterEnabled, "AccessControlForMasterEnabled", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, autoAdjustMonitoringIconSize, setAutoAdjustMonitoringIconSize, "AutoAdjustMonitoringIconSize", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, autoSelectCurrentLocation, setAutoSelectCurrentLocation, "AutoSelectCurrentLocation", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, showCurrentLocationOnly, setShowCurrentLocationOnly, "ShowCurrentLocationOnly", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, allowAddingHiddenLocations, setAllowAddingHiddenLocations, "AllowAddingHiddenLocations", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, expandLocations, setExpandLocations, "ExpandLocations", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideLocalComputer, setHideLocalComputer, "HideLocalComputer", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideOwnSession, setHideOwnSession, "HideOwnSession", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideEmptyLocations, setHideEmptyLocations, "HideEmptyLocations", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, hideComputerFilter, setHideComputerFilter, "HideComputerFilter", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master", QUuid(QStringLiteral("a18e545b-1321-4d4e-ac34-adc421c6e9c8")), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, enforceSelectedModeForClients, setEnforceSelectedModeForClients, "EnforceSelectedModeForClients", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, autoOpenComputerSelectPanel, setAutoOpenComputerSelectPanel, "AutoOpenComputerSelectPanel", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, confirmUnsafeActions, setConfirmUnsafeActions, "ConfirmUnsafeActions", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, showFeatureWindowsOnSameScreen, setShowFeatureWindowsOnSameScreen, "ShowFeatureWindowsOnSameScreen", "Master", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), Computer::NameSource, computerNameSource, setComputerNameSource, "ComputerNameSource", "Master", QVariant::fromValue(Computer::NameSource::Default), Configuration::Property::Flag::Advanced )	\
	OP( VeyonConfiguration, VeyonCore::config(), bool, identifyUsersInGuestSessions, setIdentifyUsersInGuestSessions, "IdentifyUsersInGuestSessions", "Master", false, Configuration::Property::Flag::Advanced ) \
	OP( VeyonConfiguration, VeyonCore::config(), QString, guestUserLoginName, setGuestUserLoginName, "GuestUserLoginName", "Master", VeyonCore::tr("Guest"), Configuration::Property::Flag::Advanced ) \

#define FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), QStringList, enabledAuthenticationPlugins, setEnabledAuthenticationPlugins, "EnabledPlugins", "Authentication", QStringList(), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl", false , Configuration::Property::Flag::Standard )		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl", false, Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QStringList, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl", QStringList(), Configuration::Property::Flag::Standard )	\
	OP( VeyonConfiguration, VeyonCore::config(), QJsonArray, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl", QJsonArray(), Configuration::Property::Flag::Standard )	\

#define FOREACH_VEYON_LEGACY_CONFIG_PROPERTY(OP) \
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, legacyAccessControlUserGroupsBackend, setLegacyAccessControlUserGroupsBackend, "UserGroupsBackend", "AccessControl", QUuid(), Configuration::Property::Flag::Standard )		\
	OP( VeyonConfiguration, VeyonCore::config(), bool, legacyDomainGroupsForAccessControlEnabled, setLegacyDomainGroupsForAccessControlEnabled, "DomainGroupsEnabled", "AccessControl", false, Configuration::Property::Flag::Standard )		\
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
	OP( VeyonConfiguration, VeyonCore::config(), QUuid, legacyNetworkObjectDirectoryPlugin, setLegacyNetworkObjectDirectoryPlugin, "Plugin", "NetworkObjectDirectory", QUuid(), Configuration::Property::Flag::Legacy )			\

#define FOREACH_VEYON_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_CORE_CONFIG_PROPERTIES(OP)			\
	FOREACH_VEYON_VNC_CONNECTION_CONFIG_PROPERTIES(OP)	\
	FOREACH_VEYON_UI_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(OP)			\
	FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(OP)			\
	FOREACH_VEYON_TLS_CONFIG_PROPERTY(OP)				\
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)\
	FOREACH_VEYON_USER_GROUPS_BACKEND_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_FEATURES_CONFIG_PROPERTY(OP)\
	FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(OP)		\
	FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(OP)			\
	FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_MASTER_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(OP)	\
	FOREACH_VEYON_ACCESS_CONTROL_CONFIG_PROPERTY(OP) \
	FOREACH_VEYON_LEGACY_CONFIG_PROPERTY(OP)
