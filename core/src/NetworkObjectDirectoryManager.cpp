/*
 * NetworkObjectDirectoryManager.cpp - implementation of NetworkObjectDirectoryManager
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
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginManager.h"


NetworkObjectDirectoryManager::NetworkObjectDirectoryManager( PluginManager& pluginManager ) :
    m_pluginManager( pluginManager ),
    m_directoryPluginInterfaces(),
	m_defaultDirectoryUid( "0b386701-64d5-4a29-bffe-5e91d3962157" ),
	m_defaultDirectory( this )
{
	for( auto pluginObject : m_pluginManager.pluginObjects() )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto directoryPluginInterface = qobject_cast<NetworkObjectDirectoryPluginInterface *>( pluginObject );

		if( pluginInterface && directoryPluginInterface )
		{
			m_directoryPluginInterfaces[pluginInterface] = directoryPluginInterface;
		}
	}
}


QMap<Plugin::Uid, QString> NetworkObjectDirectoryManager::availableDirectories()
{
	QMap<Plugin::Uid, QString> items;
	items[m_defaultDirectoryUid] = tr( "Default (store objects in configuration)" );

	for( auto pluginInterface : m_directoryPluginInterfaces.keys() )
	{
		items[pluginInterface->uid()] = m_directoryPluginInterfaces[pluginInterface]->directoryName();
	}

	return items;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory( QObject* parent )
{
	auto configuredPluginUuid = ItalcCore::config().networkObjectDirectoryPlugin();

	for( auto pluginInterface : m_directoryPluginInterfaces.keys() )
	{
		if( pluginInterface->uid() == configuredPluginUuid )
		{
			return m_directoryPluginInterfaces[pluginInterface]->createNetworkObjectDirectory( parent );
		}
	}

	return &m_defaultDirectory;
}
