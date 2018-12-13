/*
 * LdapConfiguration.h - LDAP-specific configuration values
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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
	OP( LdapConfiguration, m_configuration, STRING, serverHost, setServerHost, "ServerHost", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, INT, serverPort, setServerPort, "ServerPort", "LDAP", 389 )	\
	OP( LdapConfiguration, m_configuration, INT, connectionSecurity, setConnectionSecurity, "ConnectionSecurity", "LDAP", LdapClient::ConnectionSecurityNone )	\
	OP( LdapConfiguration, m_configuration, INT, tlsVerifyMode, setTLSVerifyMode, "TLSVerifyMode", "LDAP", LdapClient::TLSVerifyDefault )	\
	OP( LdapConfiguration, m_configuration, STRING, tlsCACertificateFile, setTLSCACertificateFile, "TLSCACertificateFile", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, BOOL, useBindCredentials, setUseBindCredentials, "UseBindCredentials", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, STRING, bindDn, setBindDn, "BindDN", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, PASSWORD, bindPassword, setBindPassword, "BindPassword", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, BOOL, queryNamingContext, setQueryNamingContext, "QueryNamingContext", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, STRING, baseDn, setBaseDn, "BaseDN", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, namingContextAttribute, setNamingContextAttribute, "NamingContextAttribute", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, userTree, setUserTree, "UserTree", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, groupTree, setGroupTree, "GroupTree", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, computerTree, setComputerTree, "ComputerTree", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, computerGroupTree, setComputerGroupTree, "ComputerGroupTree", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, BOOL, recursiveSearchOperations, setRecursiveSearchOperations, "RecursiveSearchOperations", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, STRING, userLoginAttribute, setUserLoginAttribute, "UserLoginAttribute", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, groupMemberAttribute, setGroupMemberAttribute, "GroupMemberAttribute", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, computerHostNameAttribute, setComputerHostNameAttribute, "ComputerHostNameAttribute", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, BOOL, computerHostNameAsFQDN, setComputerHostNameAsFQDN, "ComputerHostNameAsFQDN", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, STRING, computerMacAddressAttribute, setComputerMacAddressAttribute, "ComputerMacAddressAttribute", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, computerRoomNameAttribute, setComputerRoomNameAttribute, "ComputerRoomNameAttribute", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, usersFilter, setUsersFilter, "UsersFilter", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, userGroupsFilter, setUserGroupsFilter, "UserGroupsFilter", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, computersFilter, setComputersFilter, "ComputersFilter", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, BOOL, identifyGroupMembersByNameAttribute, setIdentifyGroupMembersByNameAttribute, "IdentifyGroupMembersByNameAttribute", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, STRING, computerGroupsFilter, setComputerGroupsFilter, "ComputerGroupsFilter", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, STRING, computerContainersFilter, setComputerContainersFilter, "ComputerContainersFilter", "LDAP", QString() )	\
	OP( LdapConfiguration, m_configuration, BOOL, computerRoomMembersByContainer, setComputerRoomMembersByContainer, "ComputerRoomMembersByContainer", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, BOOL, computerRoomMembersByAttribute, setComputerRoomMembersByAttribute, "ComputerRoomMembersByAttribute", "LDAP", false )	\
	OP( LdapConfiguration, m_configuration, STRING, computerRoomAttribute, setComputerRoomAttribute, "ComputerRoomAttribute", "LDAP", QString() )	\


DECLARE_CONFIG_PROXY(LdapConfiguration, FOREACH_LDAP_CONFIG_PROPERTY)
