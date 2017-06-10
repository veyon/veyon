/*
 * VeyonConfiguration.cpp - a Configuration object storing system wide
 *                          configuration values
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "LocalSystem.h"
#include "Logger.h"
#include "NetworkObjectDirectory.h"


VeyonConfiguration::VeyonConfiguration(
									Configuration::Store::Backend backend ) :
	Configuration::Object( backend, Configuration::Store::System )
{
}



VeyonConfiguration::VeyonConfiguration( Configuration::Store *store ) :
	Configuration::Object( store )
{
}



VeyonConfiguration::VeyonConfiguration( const VeyonConfiguration &ref ) :
	Configuration::Object( Configuration::Store::NoBackend,
							Configuration::Store::System )
{
	*this += ref;	// copy data
}




VeyonConfiguration VeyonConfiguration::defaultConfiguration()
{
	VeyonConfiguration c( Configuration::Store::NoBackend );

	c.setApplicationName( QString() );
	c.setHighDPIScalingEnabled( false );
	c.setUiLanguage( QString() );

	c.setNetworkObjectDirectoryUpdateInterval( NetworkObjectDirectory::DefaultUpdateInterval );

	c.setTrayIconHidden( false );
	c.setServiceAutostart( true );
	c.setServiceArguments( QString() );

	c.setLogLevel( Logger::LogLevelDefault );
	c.setLogFileSizeLimitEnabled( false );
	c.setLogFileRotationEnabled( false );
	c.setLogToStdErr( true );
	c.setLogToWindowsEventLog( false );
	c.setLogFileSizeLimit( 100 );
	c.setLogFileRotationCount( 10 );
	c.setLogFileDirectory( "$TEMP" );

	c.setComputerControlServerPort( PortOffsetComputerControlServer );
	c.setVncServerPort( PortOffsetVncServer );
	c.setFeatureWorkerManagerPort( PortOffsetFeatureManagerPort );
	c.setDemoServerPort( PortOffsetDemoServer );
	c.setFirewallExceptionEnabled( true );
	c.setSoftwareSASEnabled( true );

	c.setUserConfigurationDirectory( QDTNS( "$APPDATA/Config" ) );
	c.setScreenshotDirectory( QDTNS( "$APPDATA/Screenshots" ) );

	c.setKeyAuthenticationEnabled( true );
	c.setLogonAuthenticationEnabled( true );

	c.setPrivateKeyBaseDir( QDTNS( "$GLOBALAPPDATA/keys/private" ) );
	c.setPublicKeyBaseDir( QDTNS( "$GLOBALAPPDATA/keys/public" ) );

	c.setAuthorizedUserGroups( QStringList() );

	return c;
}



FOREACH_VEYON_CONFIG_PROPERTY(IMPLEMENT_CONFIG_SET_PROPERTY)

