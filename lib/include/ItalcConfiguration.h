/*
 * ItalcConfiguration.h - a Configuration object storing system wide
 *                        configuration values
 *
 * Copyright (c) 2010-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef ITALC_CONFIGURATION_H
#define ITALC_CONFIGURATION_H

#include <QtCore/QStringList>

#include "Configuration/Object.h"

#include "ItalcConfigurationProperties.h"

class ItalcConfiguration : public Configuration::Object
{
	Q_OBJECT
public:
	ItalcConfiguration( Configuration::Store::Backend backend =
										Configuration::Store::LocalBackend );
	ItalcConfiguration( Configuration::Store *store );
	ItalcConfiguration( const ItalcConfiguration &ref );

	static ItalcConfiguration defaultConfiguration();

	static QString expandPath( QString path );

	FOREACH_ITALC_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

	// unluckily we have to declare slots manually as Qt's MOC doesn't do any
	// macro expansion :-(
public slots:
	void setUiLanguage( const QString& );
	void setHighDPIScalingEnabled( bool );
	void setTrayIconHidden( bool );
	void setLockWithDesktopSwitching( bool );
	void setServiceAutostart( bool );
	void setServiceArguments( const QString & );
	void setLogLevel( int );
	void setLogToStdErr( bool );
	void setLogToWindowsEventLog( bool );
	void setLimittedLogFileSize( bool );
	void setLogFileSizeLimit( int );
	void setLogFileDirectory( const QString & );
	void setVncCaptureLayeredWindows( bool );
	void setVncPollFullScreen( bool );
	void setVncLowAccuracy( bool );
	void setDemoServerBackend( int );
	void setCoreServerPort( int );
	void setDemoServerPort( int );
	void setHttpServerPort( int );
	void setFirewallExceptionEnabled( bool );
	void setLocalConnectOnly( bool );
	void setHttpServerEnabled( bool );
	void setGlobalConfigurationPath( const QString & );
	void setPersonalConfigurationPath( const QString & );
	void setSnapshotDirectory( const QString & );
	void setKeyAuthenticationEnabled( bool );
	void setLogonAuthenticationEnabled( bool );
	void setPermissionRequiredWithKeyAuthentication( bool );
	void setPrivateKeyBaseDir( const QString & );
	void setPublicKeyBaseDir( const QString & );
	void setPermissionRequiredWithLogonAuthentication( bool );
	void setSameUserConfirmationDisabled( bool );
	void setLogonGroups( const QStringList & );
	void setAccessRestrictedToUserGroups( bool );
	void setAccessControlRulesProcessingEnabled( bool );
	void setAuthorizedUserGroups( const QStringList& );
	void setAccessControlRules( const QStringList& );
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
	void setLdapRecursiveSearchOperations( bool );
	void setLdapUserLoginAttribute( const QString& );
	void setLdapGroupMemberAttribute( const QString& );
	void setLdapComputerHostNameAttribute( const QString& );
	void setLdapUsersFilter( const QString& );
	void setLdapUserGroupsFilter( const QString& );
	void setLdapIdentifyGroupMembersByNameAttribute( bool );
	void setLdapComputerGroupsFilter( const QString& );
	void setLdapComputerPoolMembersByAttribute( bool );
	void setLdapComputerPoolAttribute( const QString& );

} ;

#endif
