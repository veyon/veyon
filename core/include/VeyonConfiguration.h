/*
 * VeyonConfiguration.h - a Configuration object storing system wide
 *                        configuration values
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QtCore/QStringList>

#include "VeyonCore.h"
#include "Configuration/Object.h"

#include "VeyonConfigurationProperties.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class VEYON_CORE_EXPORT VeyonConfiguration : public Configuration::Object
{
	Q_OBJECT
public:
	VeyonConfiguration( Configuration::Store::Backend backend =
										Configuration::Store::LocalBackend );
	VeyonConfiguration( Configuration::Store *store );
	VeyonConfiguration( const VeyonConfiguration &ref );

	static VeyonConfiguration defaultConfiguration();

	static QString expandPath( QString path );

	FOREACH_VEYON_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

	// unluckily we have to declare slots manually as Qt's MOC doesn't do any
	// macro expansion :-(
public slots:
	void setApplicationName( const QString& );
	void setUiLanguage( const QString& );
	void setHighDPIScalingEnabled( bool );
	void setTrayIconHidden( bool );
	void setServiceAutostart( bool );
	void setServiceArguments( const QString & );
	void setSoftwareSASEnabled( bool );
	void setLogLevel( int );
	void setLogToStdErr( bool );
	void setLogToWindowsEventLog( bool );
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
	void setKeyAuthenticationEnabled( bool );
	void setLogonAuthenticationEnabled( bool );
	void setPrivateKeyBaseDir( const QString & );
	void setPublicKeyBaseDir( const QString & );
	void setAccessControlDataBackend( QUuid );
	void setAccessRestrictedToUserGroups( bool );
	void setAccessControlRulesProcessingEnabled( bool );
	void setAuthorizedUserGroups( const QStringList& );
	void setAccessControlRules( const QJsonArray& );

} ;

#endif
