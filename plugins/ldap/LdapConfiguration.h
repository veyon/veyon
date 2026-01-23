// Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "CryptoCore.h"
#include "LdapClient.h"
#include "LdapCommon.h"

#define FOREACH_LDAP_CONFIG_PROPERTY(OP) \
	OP( LdapConfiguration, m_configuration, QString, serverHost, setServerHost, "ServerHost", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, int, serverPort, setServerPort, "ServerPort", "LDAP", 389, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, int, connectionSecurity, setConnectionSecurity, "ConnectionSecurity", "LDAP", LdapClient::ConnectionSecurityNone, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, int, tlsVerifyMode, setTLSVerifyMode, "TLSVerifyMode", "LDAP", LdapClient::TLSVerifyDefault, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, tlsCACertificateFile, setTLSCACertificateFile, "TLSCACertificateFile", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, useBindCredentials, setUseBindCredentials, "UseBindCredentials", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, bindDn, setBindDn, "BindDN", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, Configuration::Password, bindPassword, setBindPassword, "BindPassword", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, queryNamingContext, setQueryNamingContext, "QueryNamingContext", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, int, queryTimeout, setQueryTimeout, "QueryTimeout", "LDAP", LdapClient::DefaultQueryTimeout, Configuration::Property::Flag::Advanced )	\
	OP( LdapConfiguration, m_configuration, QString, baseDn, setBaseDn, "BaseDN", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, namingContextAttribute, setNamingContextAttribute, "NamingContextAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, userTree, setUserTree, "UserTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, groupTree, setGroupTree, "GroupTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerTree, setComputerTree, "ComputerTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerGroupTree, setComputerGroupTree, "ComputerGroupTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, recursiveSearchOperations, setRecursiveSearchOperations, "RecursiveSearchOperations", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, userLoginNameAttribute, setUserLoginNameAttribute, "UserLoginNameAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, groupMemberAttribute, setGroupMemberAttribute, "GroupMemberAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerDisplayNameAttribute, setComputerDisplayNameAttribute, "ComputerDisplayNameAttribute", "LDAP", LdapClient::cn(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerHostNameAttribute, setComputerHostNameAttribute, "ComputerHostNameAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, computerHostNameAsFQDN, setComputerHostNameAsFQDN, "ComputerHostNameAsFQDN", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerMacAddressAttribute, setComputerMacAddressAttribute, "ComputerMacAddressAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, locationNameAttribute, setLocationNameAttribute, "LocationNameAttribute", "LDAP", LdapClient::cn(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, usersFilter, setUsersFilter, "UsersFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, userGroupsFilter, setUserGroupsFilter, "UserGroupsFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computersFilter, setComputersFilter, "ComputersFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, queryNestedUserGroups, setQueryNestedUserGroups, "QueryNestedUserGroups", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, mapContainerStructureToLocations, setMapContainerStructureToLocations, "MapContainerStructureToLocations", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, identifyGroupMembersByNameAttribute, setIdentifyGroupMembersByNameAttribute, "IdentifyGroupMembersByNameAttribute", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerGroupsFilter, setComputerGroupsFilter, "ComputerGroupsFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerContainersFilter, setComputerContainersFilter, "ComputerContainersFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, computerLocationsByContainer, setComputerLocationsByContainer, "ComputerLocationsByContainer", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, computerLocationsByAttribute, setComputerLocationsByAttribute, "ComputerLocationsByAttribute", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerLocationAttribute, setComputerLocationAttribute, "ComputerLocationAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\

#define FOREACH_LDAP_LEGACY_CONFIG_PROPERTY(OP) \
	OP( LdapConfiguration, m_configuration, QString, legacyUserLoginAttribute, setLegacyUserLoginAttribute, "UserLoginAttribute", "LDAP", QString(), Configuration::Property::Flag::Legacy )	\
	OP( LdapConfiguration, m_configuration, bool, legacyComputerRoomMembersByContainer, setCompatComputerRoomMembersByContainer, "ComputerRoomMembersByContainer", "LDAP", false, Configuration::Property::Flag::Legacy )	\
	OP( LdapConfiguration, m_configuration, bool, legacyComputerRoomMembersByAttribute, setCompatComputerRoomMembersByAttribute, "ComputerRoomMembersByAttribute", "LDAP", false, Configuration::Property::Flag::Legacy )	\
	OP( LdapConfiguration, m_configuration, QString, legacyComputerRoomAttribute, setCompatComputerRoomAttribute, "ComputerRoomAttribute", "LDAP", QString(), Configuration::Property::Flag::Legacy )	\
	OP( LdapConfiguration, m_configuration, QString, legacyComputerRoomNameAttribute, setCompatComputerRoomNameAttribute, "ComputerRoomNameAttribute", "LDAP", QString(), Configuration::Property::Flag::Legacy )	\

#define FOREACH_LDAP_PROXIES_CONFIG_PROPERTY(OP) \
	FOREACH_LDAP_CONFIG_PROPERTY(OP) \
	FOREACH_LDAP_LEGACY_CONFIG_PROPERTY(OP)

DECLARE_CONFIG_PROXY(LdapConfiguration, FOREACH_LDAP_PROXIES_CONFIG_PROPERTY)
