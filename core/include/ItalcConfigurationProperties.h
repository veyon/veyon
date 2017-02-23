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
	OP( ItalcConfiguration, ItalcCore::config, STRING, applicationName, setApplicationName, "ApplicationName", "UI" );			\
	OP( ItalcConfiguration, ItalcCore::config, STRING, uiLanguage, setUiLanguage, "Language", "UI" );			\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isHighDPIScalingEnabled, setHighDPIScalingEnabled, "EnableHighDPIScaling", "UI" );			\

#define FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(OP)				\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isTrayIconHidden, setTrayIconHidden, "HideTrayIcon", "Service" );			\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, lockWithDesktopSwitching, setLockWithDesktopSwitching, "LockWithDesktopSwitching", "Service" );			\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, autostartService, setServiceAutostart, "Autostart", "Service" );			\
	OP( ItalcConfiguration, ItalcCore::config, STRING, serviceArguments, setServiceArguments, "Arguments", "Service" );			\

#define FOREACH_ITALC_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)				\
	OP( ItalcConfiguration, ItalcCore::config, INT, networkObjectDirectoryBackend, setNetworkObjectDirectoryBackend, "Backend", "NetworkObjectDirectory" );			\
	OP( ItalcConfiguration, ItalcCore::config, INT, networkObjectDirectoryUpdateInterval, setNetworkObjectDirectoryUpdateInterval, "UpdateInterval", "NetworkObjectDirectory" );			\

#define FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, INT, logLevel, setLogLevel, "LogLevel", "Logging" );								\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, limittedLogFileSize, setLimittedLogFileSize, "LimittedLogFileSize", "Logging" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, logToStdErr, setLogToStdErr, "LogToStdErr", "Logging" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, logToWindowsEventLog, setLogToWindowsEventLog, "LogToWindowsEventLog", "Logging" );	\
	OP( ItalcConfiguration, ItalcCore::config, INT, logFileSizeLimit, setLogFileSizeLimit, "LogFileSizeLimit", "Logging" );		\
	OP( ItalcConfiguration, ItalcCore::config, STRING, logFileDirectory, setLogFileDirectory, "LogFileDirectory", "Logging" );		\

#define FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, BOOL, vncCaptureLayeredWindows, setVncCaptureLayeredWindows, "CaptureLayeredWindows", "VNC" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, vncPollFullScreen, setVncPollFullScreen, "PollFullScreen", "VNC" );			\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, vncLowAccuracy, setVncLowAccuracy, "LowAccuracy", "VNC" );					\

#define FOREACH_ITALC_DEMO_SERVER_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, INT, demoServerBackend, setDemoServerBackend, "Backend", "DemoServer" );		\

#define FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, INT, coreServerPort, setCoreServerPort, "CoreServerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config, INT, featureWorkerManagerPort, setFeatureWorkerManagerPort, "FeatureWorkerManagerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config, INT, demoServerPort, setDemoServerPort, "DemoServerPort", "Network" );			\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isFirewallExceptionEnabled, setFirewallExceptionEnabled, "FirewallExceptionEnabled", "Network" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, localConnectOnly, setLocalConnectOnly, "LocalConnectOnly", "Network" );					\

#define FOREACH_ITALC_DIRECTORIES_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, STRING, userConfigurationDirectory, setUserConfigurationDirectory, "UserConfiguration", "Directories" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, snapshotDirectory, setSnapshotDirectory, "Snapshots", "Directories" );	\

#define FOREACH_ITALC_MASTER_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, BOOL, onlyCurrentRoomVisible, setOnlyCurrentRoomVisible, "OnlyCurrentRoomVisible", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, manualRoomAdditionAllowed, setManualRoomAdditionAllowed, "ManualRoomAdditionAllowed", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, balloonTooltipsDisabled, setBalloonTooltipsDisabled, "BalloonTooltipsDisabled", "Master" );	\
	OP( ItalcConfiguration, ItalcCore::config, UUID, computerDoubleClickFeature, setComputerDoubleClickFeature, "ComputerDoubleClickFeature", "Master" );	\

#define FOREACH_ITALC_AUTHENTICATION_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isKeyAuthenticationEnabled, setKeyAuthenticationEnabled, "KeyAuthenticationEnabled", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isLogonAuthenticationEnabled, setLogonAuthenticationEnabled, "LogonAuthenticationEnabled", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isPermissionRequiredWithKeyAuthentication, setPermissionRequiredWithKeyAuthentication, "PermissionRequiredWithKeyAuthentication", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, privateKeyBaseDir, setPrivateKeyBaseDir, "PrivateKeyBaseDir", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, publicKeyBaseDir, setPublicKeyBaseDir, "PublicKeyBaseDir", "Authentication" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isPermissionRequiredWithLogonAuthentication, setPermissionRequiredWithLogonAuthentication, "PermissionRequiredWithLogonAuthentication", "Authentication" );	\

#define FOREACH_ITALC_ACCESS_CONTROL_CONFIG_PROPERTY(OP)		\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isAccessRestrictedToUserGroups, setAccessRestrictedToUserGroups, "AccessRestrictedToUserGroups", "AccessControl");		\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isAccessControlRulesProcessingEnabled, setAccessControlRulesProcessingEnabled, "AccessControlRulesProcessingEnabled", "AccessControl");	\
	OP( ItalcConfiguration, ItalcCore::config, STRINGLIST, authorizedUserGroups, setAuthorizedUserGroups, "AuthorizedUserGroups", "AccessControl" );	\
	OP( ItalcConfiguration, ItalcCore::config, JSONARRAY, accessControlRules, setAccessControlRules, "AccessControlRules", "AccessControl" );	\

#define FOREACH_ITALC_LDAP_CONFIG_PROPERTY(OP) \
	OP( ItalcConfiguration, ItalcCore::config, BOOL, isLdapIntegrationEnabled, setLdapIntegrationEnabled, "LdapIntegrationEnabled", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapServerHost, setLdapServerHost, "ServerHost", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, INT, ldapServerPort, setLdapServerPort, "ServerPort", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, ldapUseBindCredentials, setLdapUseBindCredentials, "UseBindCredentials", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapBindDn, setLdapBindDn, "BindDN", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapBindPassword, setLdapBindPassword, "BindPassword", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, ldapQueryNamingContext, setLdapQueryNamingContext, "QueryNamingContext", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapBaseDn, setLdapBaseDn, "BaseDN", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapNamingContextAttribute, setLdapNamingContextAttribute, "NamingContextAttribute", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapUserTree, setLdapUserTree, "UserTree", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapGroupTree, setLdapGroupTree, "GroupTree", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapComputerTree, setLdapComputerTree, "ComputerTree", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, ldapRecursiveSearchOperations, setLdapRecursiveSearchOperations, "RecursiveSearchOperations", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapUserLoginAttribute, setLdapUserLoginAttribute, "UserLoginAttribute", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapGroupMemberAttribute, setLdapGroupMemberAttribute, "GroupMemberAttribute", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapComputerHostNameAttribute, setLdapComputerHostNameAttribute, "ComputerHostNameAttribute", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, ldapComputerHostNameAsFQDN, setLdapComputerHostNameAsFQDN, "ComputerHostNameAsFQDN", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapUsersFilter, setLdapUsersFilter, "UsersFilter", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapUserGroupsFilter, setLdapUserGroupsFilter, "UserGroupsFilter", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, ldapIdentifyGroupMembersByNameAttribute, setLdapIdentifyGroupMembersByNameAttribute, "IdentifyGroupMembersByNameAttribute", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapComputerGroupsFilter, setLdapComputerGroupsFilter, "ComputerGroupsFilter", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, BOOL, ldapComputerLabMembersByAttribute, setLdapComputerLabMembersByAttribute, "ComputerLabMembersByAttribute", "LDAP" );	\
	OP( ItalcConfiguration, ItalcCore::config, STRING, ldapComputerLabAttribute, setLdapComputerLabAttribute, "ComputerLabAttribute", "LDAP" );	\

#define FOREACH_ITALC_CONFIG_PROPERTY(OP)				\
	FOREACH_ITALC_UI_CONFIG_PROPERTY(OP)				\
	FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(OP)			\
	FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(OP)			\
	FOREACH_ITALC_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(OP)\
	FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(OP)		\
	FOREACH_ITALC_DEMO_SERVER_CONFIG_PROPERTY(OP)		\
	FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(OP)			\
	FOREACH_ITALC_DIRECTORIES_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_MASTER_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_AUTHENTICATION_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_ACCESS_CONTROL_CONFIG_PROPERTY(OP)	\
	FOREACH_ITALC_LDAP_CONFIG_PROPERTY(OP)				\

#endif
