/*
 * UserGroupsBackendManager.cpp - implementation of UserGroupsBackendManager
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

#include "VeyonConfiguration.h"
#include "PluginManager.h"
#include "UserGroupsBackendManager.h"


UserGroupsBackendManager::UserGroupsBackendManager( QObject* parent ) :
	QObject( parent ),
	m_backends(),
	m_defaultBackend( nullptr ),
	m_accessControlBackend( nullptr )
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto userGroupsBackendInterface = qobject_cast<UserGroupsBackendInterface *>( pluginObject );

		if( pluginInterface && userGroupsBackendInterface )
		{
			m_backends[pluginInterface->uid()] = userGroupsBackendInterface;

			if( pluginInterface->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
			{
				m_defaultBackend = userGroupsBackendInterface;
			}
		}
	}

	if( m_defaultBackend == nullptr )
	{
		qCritical( "UserGroupsBackendManager: no default plugin available!" );
	}

	reloadConfiguration();
}



QMap<Plugin::Uid, QString> UserGroupsBackendManager::availableBackends()
{
	QMap<Plugin::Uid, QString> items;

	for( auto it = m_backends.constBegin(), end = m_backends.constEnd(); it != end; ++it )
	{
		items[it.key()] = it.value()->userGroupsBackendName();
	}

	return items;
}



UserGroupsBackendInterface* UserGroupsBackendManager::accessControlBackend()
{
	if( m_accessControlBackend == nullptr )
	{
		reloadConfiguration();
	}

	return m_accessControlBackend;
}



void UserGroupsBackendManager::reloadConfiguration()
{
	m_accessControlBackend = m_backends.value( VeyonCore::config().accessControlUserGroupsBackend() );

	if( m_accessControlBackend == nullptr )
	{
		m_accessControlBackend = m_defaultBackend;
	}
}
