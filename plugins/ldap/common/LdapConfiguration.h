/*
 * LdapConfiguration.h - LDAP-specific configuration values
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "Configuration/Proxy.h"
#include "CryptoCore.h"
#include "LdapClient.h"

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
	OP( LdapConfiguration, m_configuration, QString, baseDn, setBaseDn, "BaseDN", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, namingContextAttribute, setNamingContextAttribute, "NamingContextAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, userTree, setUserTree, "UserTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, groupTree, setGroupTree, "GroupTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerTree, setComputerTree, "ComputerTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerGroupTree, setComputerGroupTree, "ComputerGroupTree", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, recursiveSearchOperations, setRecursiveSearchOperations, "RecursiveSearchOperations", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, userLoginNameAttribute, setUserLoginNameAttribute, "UserLoginNameAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, groupMemberAttribute, setGroupMemberAttribute, "GroupMemberAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerDisplayNameAttribute, setComputerDisplayNameAttribute, "ComputerDisplayNameAttribute", "LDAP", QStringLiteral("cn"), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerHostNameAttribute, setComputerHostNameAttribute, "ComputerHostNameAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, bool, computerHostNameAsFQDN, setComputerHostNameAsFQDN, "ComputerHostNameAsFQDN", "LDAP", false, Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computerMacAddressAttribute, setComputerMacAddressAttribute, "ComputerMacAddressAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, locationNameAttribute, setLocationNameAttribute, "LocationNameAttribute", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, usersFilter, setUsersFilter, "UsersFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, userGroupsFilter, setUserGroupsFilter, "UserGroupsFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
	OP( LdapConfiguration, m_configuration, QString, computersFilter, setComputersFilter, "ComputersFilter", "LDAP", QString(), Configuration::Property::Flag::Standard )	\
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
