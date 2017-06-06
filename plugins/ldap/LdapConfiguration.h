/*
 * LdapConfiguration.h - LDAP-specific configuration values
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef LDAP_CONFIGURATION_H
#define LDAP_CONFIGURATION_H

#include "Configuration/Proxy.h"

#define FOREACH_LDAP_CONFIG_PROPERTY(OP) \
	OP( LdapConfiguration, m_configuration, STRING, ldapServerHost, setLdapServerHost, "ServerHost", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, INT, ldapServerPort, setLdapServerPort, "ServerPort", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, ldapUseBindCredentials, setLdapUseBindCredentials, "UseBindCredentials", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapBindDn, setLdapBindDn, "BindDN", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapBindPassword, setLdapBindPassword, "BindPassword", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, ldapQueryNamingContext, setLdapQueryNamingContext, "QueryNamingContext", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapBaseDn, setLdapBaseDn, "BaseDN", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapNamingContextAttribute, setLdapNamingContextAttribute, "NamingContextAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapUserTree, setLdapUserTree, "UserTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapGroupTree, setLdapGroupTree, "GroupTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputerTree, setLdapComputerTree, "ComputerTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputerGroupTree, setLdapComputerGroupTree, "ComputerGroupTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, ldapRecursiveSearchOperations, setLdapRecursiveSearchOperations, "RecursiveSearchOperations", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapUserLoginAttribute, setLdapUserLoginAttribute, "UserLoginAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapGroupMemberAttribute, setLdapGroupMemberAttribute, "GroupMemberAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputerHostNameAttribute, setLdapComputerHostNameAttribute, "ComputerHostNameAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, ldapComputerHostNameAsFQDN, setLdapComputerHostNameAsFQDN, "ComputerHostNameAsFQDN", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputerMacAddressAttribute, setLdapComputerMacAddressAttribute, "ComputerMacAddressAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapUsersFilter, setLdapUsersFilter, "UsersFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapUserGroupsFilter, setLdapUserGroupsFilter, "UserGroupsFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputersFilter, setLdapComputersFilter, "ComputersFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, ldapIdentifyGroupMembersByNameAttribute, setLdapIdentifyGroupMembersByNameAttribute, "IdentifyGroupMembersByNameAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputerGroupsFilter, setLdapComputerGroupsFilter, "ComputerGroupsFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, ldapComputerLabMembersByAttribute, setLdapComputerLabMembersByAttribute, "ComputerLabMembersByAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, ldapComputerLabAttribute, setLdapComputerLabAttribute, "ComputerLabAttribute", "LDAP" );	\


class LdapConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	LdapConfiguration();

	FOREACH_LDAP_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setLdapServerHost( const QString& );
	void setLdapServerPort( int );
	void setLdapUseBindCredentials( bool );
	void setLdapBindDn( const QString& );
	void setLdapBindPassword( const QString& );
	void setLdapBaseDn( const QString& );
	void setLdapQueryNamingContext( bool );
	void setLdapNamingContextAttribute( const QString& );
	void setLdapUserTree( const QString& );
	void setLdapGroupTree( const QString& );
	void setLdapComputerTree( const QString& );
	void setLdapComputerGroupTree( const QString& );
	void setLdapRecursiveSearchOperations( bool );
	void setLdapUserLoginAttribute( const QString& );
	void setLdapGroupMemberAttribute( const QString& );
	void setLdapComputerHostNameAttribute( const QString& );
	void setLdapComputerHostNameAsFQDN( bool );
	void setLdapComputerMacAddressAttribute( const QString& );
	void setLdapUsersFilter( const QString& );
	void setLdapUserGroupsFilter( const QString& );
	void setLdapComputersFilter( const QString& );
	void setLdapIdentifyGroupMembersByNameAttribute( bool );
	void setLdapComputerGroupsFilter( const QString& );
	void setLdapComputerLabMembersByAttribute( bool );
	void setLdapComputerLabAttribute( const QString& );

} ;

#endif
