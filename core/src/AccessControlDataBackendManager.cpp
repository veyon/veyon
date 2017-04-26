/*
 * AccessControlDataBackendManager.cpp - implementation of AccessControlDataBackendManager
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "VeyonConfiguration.h"
#include "PluginManager.h"
#include "AccessControlDataBackendManager.h"


AccessControlDataBackendManager::AccessControlDataBackendManager( PluginManager& pluginManager ) :
	m_backends(),
	m_defaultBackend( nullptr ),
	m_configuredBackend( nullptr )
{
	for( auto pluginObject : qAsConst( pluginManager.pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto accessControlDataBackendInterface = qobject_cast<AccessControlDataBackendInterface *>( pluginObject );

		if( pluginInterface && accessControlDataBackendInterface )
		{
			m_backends[pluginInterface->uid()] = accessControlDataBackendInterface;

			if( pluginInterface->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
			{
				m_defaultBackend = accessControlDataBackendInterface;
			}
		}
	}

	if( m_defaultBackend == nullptr )
	{
		qCritical( "AccessControlDataBackendManager: no default plugin available!" );
	}

	reloadConfiguration();
}



QMap<Plugin::Uid, QString> AccessControlDataBackendManager::availableBackends()
{
	QMap<Plugin::Uid, QString> items;

	for( auto it = m_backends.keyBegin(), end = m_backends.keyEnd(); it != end; ++it )
	{
		items[*it] = m_backends[*it]->accessControlDataBackendName();
	}

	return items;
}



void AccessControlDataBackendManager::reloadConfiguration()
{
	m_configuredBackend = m_backends.value( VeyonCore::config().accessControlDataBackend() );

	if( m_configuredBackend == nullptr )
	{
		m_configuredBackend = m_defaultBackend;
	}
}
