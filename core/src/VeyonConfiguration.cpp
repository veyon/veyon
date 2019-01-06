/*
 * VeyonConfiguration.cpp - a Configuration object storing system wide
 *                          configuration values
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

#include <QDir>

#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "VeyonRfbExt.h"
#include "Logger.h"
#include "NetworkObjectDirectory.h"


VeyonConfiguration::VeyonConfiguration() :
	Configuration::Object( Configuration::Store::LocalBackend,
						   Configuration::Store::System,
						   defaultConfiguration() )
{
}



VeyonConfiguration::VeyonConfiguration( Configuration::Store* store ) :
	Configuration::Object( store )
{
}



VeyonConfiguration VeyonConfiguration::defaultConfiguration()
{
	VeyonConfiguration c( nullptr );

	c.setApplicationName( QString() );
	c.setUiLanguage( QString() );

	c.setNetworkObjectDirectoryUpdateInterval( NetworkObjectDirectory::DefaultUpdateInterval );

	c.setTrayIconHidden( false );
	c.setFailedAuthenticationNotificationsEnabled( true );
	c.setRemoteConnectionNotificationsEnabled( false );
	c.setServiceAutostart( true );

	c.setLogLevel( Logger::LogLevelDefault );
	c.setLogFileSizeLimitEnabled( false );
	c.setLogFileRotationEnabled( false );
	c.setLogToStdErr( true );
	c.setLogToSystem( false );
	c.setLogFileSizeLimit( 100 );
	c.setLogFileRotationCount( 10 );
	c.setLogFileDirectory( QStringLiteral( "$TEMP" ) );

	c.setPrimaryServicePort( PortOffsetPrimaryServiceServer );
	c.setVncServerPort( PortOffsetVncServer );
	c.setFeatureWorkerManagerPort( PortOffsetFeatureManagerPort );
	c.setDemoServerPort( PortOffsetDemoServer );
	c.setFirewallExceptionEnabled( true );
	c.setSoftwareSASEnabled( true );

	c.setUserConfigurationDirectory( QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Config" ) ) );
	c.setScreenshotDirectory( QDir::toNativeSeparators( QStringLiteral( "%APPDATA%/Screenshots" ) ) );
	c.setComputerMonitoringUpdateInterval( 1000 );
	c.setComputerMonitoringBackgroundColor( Qt::white );
	c.setComputerMonitoringTextColor( Qt::black );

	c.setAuthenticationMethod( VeyonCore::LogonAuthentication );

	c.setPrivateKeyBaseDir( QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/private" ) ) );
	c.setPublicKeyBaseDir( QDir::toNativeSeparators( QStringLiteral( "%GLOBALAPPDATA%/keys/public" ) ) );

	c.setAuthorizedUserGroups( QStringList() );

	return c;
}



FOREACH_VEYON_CONFIG_PROPERTY(IMPLEMENT_CONFIG_SET_PROPERTY)
