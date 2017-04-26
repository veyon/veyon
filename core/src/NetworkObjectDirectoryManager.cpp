/*
 * NetworkObjectDirectoryManager.cpp - implementation of NetworkObjectDirectoryManager
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
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginManager.h"


NetworkObjectDirectoryManager::NetworkObjectDirectoryManager() :
	m_directoryPluginInterfaces()
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
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

	for( auto it = m_directoryPluginInterfaces.keyBegin(), end = m_directoryPluginInterfaces.keyEnd(); it != end; ++it )
	{
		const auto pluginInterface = *it;
		items[pluginInterface->uid()] = m_directoryPluginInterfaces[pluginInterface]->directoryName();
	}

	return items;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory( QObject* parent )
{
	const auto configuredPluginUuid = VeyonCore::config().networkObjectDirectoryPlugin();
	NetworkObjectDirectoryPluginInterface* defaultPluginInterface = nullptr;

	for( auto it = m_directoryPluginInterfaces.keyBegin(), end = m_directoryPluginInterfaces.keyEnd(); it != end; ++it )
	{
		const auto pluginInterface = *it;

		if( pluginInterface->uid() == configuredPluginUuid )
		{
			return m_directoryPluginInterfaces[pluginInterface]->createNetworkObjectDirectory( parent );
		}
		else if( pluginInterface->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
		{
			defaultPluginInterface = m_directoryPluginInterfaces[pluginInterface];
		}
	}

	if( defaultPluginInterface == nullptr )
	{
		qCritical() << "NetworkObjectDirectoryManager: no default plugin available! configured plugin:" << configuredPluginUuid;
		return nullptr;
	}

	return defaultPluginInterface->createNetworkObjectDirectory( parent );
}
