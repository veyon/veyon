/*
 * AuthenticationManager.cpp - implementation of AuthenticationManager
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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
	QObject( parent ),
	m_configuredPlugin( nullptr ),
	m_dummyAuthentication()
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

	reloadConfiguration();
}



AuthenticationManager::Types AuthenticationManager::availableTypes() const
{
	Types types;

	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		types[it.key()] = it.value()->authenticationTypeName();
	}

	return types;
}



void AuthenticationManager::reloadConfiguration()
{
	m_configuredPlugin = m_plugins.value( VeyonCore::config().authenticationPlugin() );

	if( m_configuredPlugin == nullptr )
	{
		m_configuredPlugin = &m_dummyAuthentication;
	}
}
