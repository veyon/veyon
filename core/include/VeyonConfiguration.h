/*
 * VeyonConfiguration.h - a Configuration object storing system wide
 *                        configuration values
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef VEYON_CONFIGURATION_H
#define VEYON_CONFIGURATION_H

#include <QStringList>

#include "VeyonCore.h"
#include "Configuration/Object.h"

#include "VeyonConfigurationProperties.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class VEYON_CORE_EXPORT VeyonConfiguration : public Configuration::Object
{
	Q_OBJECT
public:
	VeyonConfiguration();
	VeyonConfiguration( Configuration::Store* store );

	static VeyonConfiguration defaultConfiguration();

	static QString expandPath( QString path );

	FOREACH_VEYON_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

	// unluckily we have to declare slots manually as Qt's MOC doesn't do any
	// macro expansion :-(
public slots:
	void setPluginVersions( const QJsonObject& );
	void setInstallationID( const QString& );
	void setApplicationName( const QString& );
	void setUiLanguage( const QString& );
	void setTrayIconHidden( bool );
	void setFailedAuthenticationNotificationsEnabled( bool );
	void setRemoteConnectionNotificationsEnabled( bool );
	void setServiceAutostart( bool );
	void setMultiSessionServiceEnabled( bool );
	void setSoftwareSASEnabled( bool );
	void setLogLevel( int );
	void setLogToStdErr( bool );
	void setLogToSystem( bool );
	void setLogFileSizeLimitEnabled( bool );
	void setLogFileRotationEnabled( bool );
	void setLogFileSizeLimit( int );
	void setLogFileRotationCount( int );
	void setLogFileDirectory( const QString & );
	void setNetworkObjectDirectoryPlugin( QUuid );
	void setNetworkObjectDirectoryUpdateInterval( int );
	void setDisabledFeatures( const QStringList& );
	void setVncServerPlugin( QUuid );
	void setPrimaryServicePort( int );
	void setVncServerPort( int );
	void setFeatureWorkerManagerPort( int );
	void setDemoServerPort( int );
	void setFirewallExceptionEnabled( bool );
	void setLocalConnectOnly( bool );
	void setUserConfigurationDirectory( const QString & );
	void setScreenshotDirectory( const QString & );
	void setComputerMonitoringUpdateInterval( int );
	void setComputerDisplayRoleContent( int );
	void setComputerMonitoringBackgroundColor( const QColor& );
	void setComputerMonitoringTextColor( const QColor& );
	void setAccessControlForMasterEnabled( bool );
	void setAutoAdjustGridSize( bool );
	void setAutoSwitchToCurrentRoom( bool );
	void setOnlyCurrentRoomVisible( bool );
	void setManualRoomAdditionAllowed( bool );
	void setLocalComputerHidden( bool );
	void setEmptyRoomsHidden( bool );
	void setComputerFilterHidden( bool );
	void setComputerDoubleClickFeature( QUuid );
	void setEnforceSelectedModeForClients( bool );
	void setOpenComputerManagementAtStart( bool );
	void setConfirmDangerousActions( bool );
	void setAuthenticationMethod( int );
	void setPrivateKeyBaseDir( const QString & );
	void setPublicKeyBaseDir( const QString & );
	void setAccessControlUserGroupsBackend( QUuid );
	void setDomainGroupsForAccessControlEnabled( bool );
	void setAccessRestrictedToUserGroups( bool );
	void setAccessControlRulesProcessingEnabled( bool );
	void setAuthorizedUserGroups( const QStringList& );
	void setAccessControlRules( const QJsonArray& );

} ;

#endif
