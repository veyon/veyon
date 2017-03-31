/*
 * UsersAndGroupsManager.cpp - implementation of UsersAndGroupsManager
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ItalcConfiguration.h"
#include "PluginManager.h"
#include "UsersAndGroupsBackendManager.h"


UsersAndGroupsBackendManager::UsersAndGroupsBackendManager() :
	m_configuredBackend( nullptr ),
	m_usersAndGroupsPluginInterfaces(),
	m_localUsersAndGroupsBuiltin()
{
	m_usersAndGroupsPluginInterfaces[Plugin::Uid("4826d624-4a04-4429-9412-2df4af4df695")] = &m_localUsersAndGroupsBuiltin;

	for( auto pluginObject : ItalcCore::pluginManager().pluginObjects() )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto usersAndGroupsInterface = qobject_cast<UsersAndGroupsPluginInterface *>( pluginObject );

		if( pluginInterface && usersAndGroupsInterface )
		{
			m_usersAndGroupsPluginInterfaces[pluginInterface->uid()] = usersAndGroupsInterface;
		}
	}

	m_configuredBackend = m_usersAndGroupsPluginInterfaces.value( ItalcCore::config().usersAndGroupsPlugin() );

	if( m_configuredBackend == nullptr )
	{
		m_configuredBackend = &m_localUsersAndGroupsBuiltin;
	}
}



QMap<Plugin::Uid, QString> UsersAndGroupsBackendManager::availableBackends()
{
	QMap<Plugin::Uid, QString> items;

	for( auto pluginUid : m_usersAndGroupsPluginInterfaces.keys() )
	{
		items[pluginUid] = m_usersAndGroupsPluginInterfaces[pluginUid]->usersAndGroupsBackendName();
	}

	return items;
}
