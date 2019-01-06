/*
 * ConfigurationManager.cpp - class for managing Veyon's configuration
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

#include "Configuration/LocalStore.h"
#include "ConfigurationManager.h"
#include "Filesystem.h"
#include "PlatformInputDeviceFunctions.h"
#include "PlatformNetworkFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonServiceControl.h"



ConfigurationManager::ConfigurationManager( QObject* parent ) :
	QObject( parent ),
	m_configuration( VeyonCore::config() )
{
}



bool ConfigurationManager::clearConfiguration()
{
	Configuration::LocalStore( Configuration::LocalStore::System ).clear();

	return true;
}



bool ConfigurationManager::applyConfiguration()
{
	// update Veyon Service configuration
	if( VeyonServiceControl().setAutostart( m_configuration.autostartService() ) == false )
	{
		m_errorString =  tr( "Could not modify the autostart property for the %1 Service." ).arg( VeyonCore::applicationName() );
		return false;
	}

	auto& network = VeyonCore::platform().networkFunctions();

	if( network.configureFirewallException( VeyonCore::filesystem().serverFilePath(),
											QStringLiteral("Veyon Server"),
											m_configuration.isFirewallExceptionEnabled() ) == false )
	{
		m_errorString = tr( "Could not configure the firewall configuration for the %1 Server." ).arg( VeyonCore::applicationName() );
		return false;
	}

	if( network.configureFirewallException( VeyonCore::filesystem().workerFilePath(),
											QStringLiteral("Veyon Worker"),
											m_configuration.isFirewallExceptionEnabled() ) == false )
	{
		m_errorString = tr( "Could not configure the firewall configuration for the %1 Worker." ).arg( VeyonCore::applicationName() );
		return false;
	}

	if( VeyonCore::platform().inputDeviceFunctions().configureSoftwareSAS( VeyonCore::config().isSoftwareSASEnabled() ) == false )
	{
		m_errorString =  tr( "Could not change the setting for SAS generation by software. "
							 "Sending Ctrl+Alt+Del via remote control will not work!" );
		return false;
	}

	return true;
}



bool ConfigurationManager::saveConfiguration()
{
	// write global configuration
	Configuration::LocalStore localStore( Configuration::LocalStore::System );
	if( localStore.isWritable() == false )
	{
		m_errorString = tr( "Configuration is not writable. Please check your permissions!" );
		return false;
	}

	localStore.flush( &m_configuration );
	return true;
}
