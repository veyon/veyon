/*
 * AuthenticationManager.cpp - implementation of AuthenticationManager
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "PluginManager.h"
#include "AuthenticationManager.h"
#include "VeyonConfiguration.h"

AuthenticationManager::AuthenticationManager( QObject* parent ) :
	QObject( parent )
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto authenticationPluginInterface = qobject_cast<AuthenticationPluginInterface *>( pluginObject );

		if( pluginInterface && authenticationPluginInterface )
		{
			m_plugins[pluginInterface->uid()] = authenticationPluginInterface;
		}
	}

	if( m_plugins.isEmpty() )
	{
		qFatal( "AuthenticationManager: no authentication plugins available!" );
	}
}



Plugin::Uid AuthenticationManager::toUid( AuthenticationPluginInterface* authPlugin ) const
{
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		if( it.value() == authPlugin )
		{
			return it.key();
		}
	}

	return {};
}



AuthenticationManager::Types AuthenticationManager::availableMethods() const
{
	Types types;
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		types[it.key()] = it.value()->authenticationMethodName();
	}

	return types;
}



void AuthenticationManager::setEnabled( Plugin::Uid uid, bool enabled )
{
	const auto formattedUid = VeyonCore::formattedUuid(uid);

	auto plugins = VeyonCore::config().enabledAuthenticationPlugins();

	if( enabled )
	{
		plugins.append( formattedUid );
		plugins.removeDuplicates();
	}
	else
	{
		plugins.removeAll( formattedUid );
	}

	VeyonCore::config().setEnabledAuthenticationPlugins( plugins );
}



bool AuthenticationManager::isEnabled( Plugin::Uid uid ) const
{
	return VeyonCore::config().enabledAuthenticationPlugins().contains( VeyonCore::formattedUuid(uid) );
}



void AuthenticationManager::setEnabled( AuthenticationPluginInterface* authPlugin, bool enabled )
{
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		if( it.value() == authPlugin )
		{
			setEnabled( it.key(), enabled );
		}
	}
}



bool AuthenticationManager::isEnabled( AuthenticationPluginInterface* authPlugin ) const
{
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		if( it.value() == authPlugin && isEnabled( it.key() ) )
		{
			return true;
		}
	}

	return false;
}



bool AuthenticationManager::initializeCredentials()
{
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		if( isEnabled( it.key() ) )
		{
			if( it.value()->initializeCredentials() )
			{
				m_initializedPlugin = it.value();
				return true;
			}
		}
	}

	return false;
}
