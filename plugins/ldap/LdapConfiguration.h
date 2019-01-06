/*
 * LdapConfiguration.h - LDAP-specific configuration values
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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
#include "CryptoCore.h"

#define FOREACH_LDAP_CONFIG_PROPERTY(OP) \
	OP( LdapConfiguration, m_configuration, STRING, serverHost, setServerHost, "ServerHost", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, INT, serverPort, setServerPort, "ServerPort", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, INT, connectionSecurity, setConnectionSecurity, "ConnectionSecurity", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, INT, tlsVerifyMode, setTLSVerifyMode, "TLSVerifyMode", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, tlsCACertificateFile, setTLSCACertificateFile, "TLSCACertificateFile", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, useBindCredentials, setUseBindCredentials, "UseBindCredentials", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, bindDn, setBindDn, "BindDN", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, PASSWORD, bindPassword, setBindPassword, "BindPassword", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, queryNamingContext, setQueryNamingContext, "QueryNamingContext", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, baseDn, setBaseDn, "BaseDN", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, namingContextAttribute, setNamingContextAttribute, "NamingContextAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, userTree, setUserTree, "UserTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, groupTree, setGroupTree, "GroupTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerTree, setComputerTree, "ComputerTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerGroupTree, setComputerGroupTree, "ComputerGroupTree", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, recursiveSearchOperations, setRecursiveSearchOperations, "RecursiveSearchOperations", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, userLoginAttribute, setUserLoginAttribute, "UserLoginAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, groupMemberAttribute, setGroupMemberAttribute, "GroupMemberAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerHostNameAttribute, setComputerHostNameAttribute, "ComputerHostNameAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, computerHostNameAsFQDN, setComputerHostNameAsFQDN, "ComputerHostNameAsFQDN", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerMacAddressAttribute, setComputerMacAddressAttribute, "ComputerMacAddressAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerRoomNameAttribute, setComputerRoomNameAttribute, "ComputerRoomNameAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, usersFilter, setUsersFilter, "UsersFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, userGroupsFilter, setUserGroupsFilter, "UserGroupsFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computersFilter, setComputersFilter, "ComputersFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, identifyGroupMembersByNameAttribute, setIdentifyGroupMembersByNameAttribute, "IdentifyGroupMembersByNameAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerGroupsFilter, setComputerGroupsFilter, "ComputerGroupsFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerContainersFilter, setComputerContainersFilter, "ComputerContainersFilter", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, computerRoomMembersByContainer, setComputerRoomMembersByContainer, "ComputerRoomMembersByContainer", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, BOOL, computerRoomMembersByAttribute, setComputerRoomMembersByAttribute, "ComputerRoomMembersByAttribute", "LDAP" );	\
	OP( LdapConfiguration, m_configuration, STRING, computerRoomAttribute, setComputerRoomAttribute, "ComputerRoomAttribute", "LDAP" );	\


class LdapConfiguration : public Configuration::Proxy
{
	Q_OBJECT
public:
	enum ConnectionSecurity
	{
		ConnectionSecurityNone,
		ConnectionSecurityTLS,
		ConnectionSecuritySSL,
		ConnectionSecurityCount,
	};

	enum TLSVerifyMode
	{
		TLSVerifyDefault,
		TLSVerifyNever,
		TLSVerifyCustomCert,
		TLSVerifyModeCount
	};
	LdapConfiguration( QObject* parent = nullptr );

	FOREACH_LDAP_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

public slots:
	void setServerHost( const QString& );
	void setServerPort( int );
	void setConnectionSecurity( int );
	void setTLSVerifyMode( int );
	void setTLSCACertificateFile( const QString& );
	void setUseBindCredentials( bool );
	void setBindDn( const QString& );
	void setBindPassword( const QString& );
	void setBaseDn( const QString& );
	void setQueryNamingContext( bool );
	void setNamingContextAttribute( const QString& );
	void setUserTree( const QString& );
	void setGroupTree( const QString& );
	void setComputerTree( const QString& );
	void setComputerGroupTree( const QString& );
	void setRecursiveSearchOperations( bool );
	void setUserLoginAttribute( const QString& );
	void setGroupMemberAttribute( const QString& );
	void setComputerHostNameAttribute( const QString& );
	void setComputerHostNameAsFQDN( bool );
	void setComputerMacAddressAttribute( const QString& );
	void setComputerRoomNameAttribute( const QString& );
	void setUsersFilter( const QString& );
	void setUserGroupsFilter( const QString& );
	void setComputersFilter( const QString& );
	void setIdentifyGroupMembersByNameAttribute( bool );
	void setComputerGroupsFilter( const QString& );
	void setComputerContainersFilter( const QString& );
	void setComputerRoomMembersByContainer( bool );
	void setComputerRoomMembersByAttribute( bool );
	void setComputerRoomAttribute( const QString& );

} ;

#endif
