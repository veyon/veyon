/*
 * NetworkObjectDirectoryManager.cpp - implementation of NetworkObjectDirectoryManager
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
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginManager.h"


NetworkObjectDirectoryManager::NetworkObjectDirectoryManager( QObject* parent ) :
	QObject( parent ),
	m_directoryPluginInterfaces(),
	m_configuredDirectory( nullptr )
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

	for( auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it )
	{
		items[it.key()->uid()] = it.value()->directoryName();
	}

	return items;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::configuredDirectory()
{
	if( m_configuredDirectory == nullptr )
	{
		m_configuredDirectory = createDirectory();
	}

	return m_configuredDirectory;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory()
{
	const auto configuredPluginUuid = VeyonCore::config().networkObjectDirectoryPlugin();
	NetworkObjectDirectoryPluginInterface* defaultPluginInterface = nullptr;

	for( auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it )
	{
		const auto pluginInterface = it.key();

		if( pluginInterface->uid() == configuredPluginUuid )
		{
			return it.value()->createNetworkObjectDirectory( this );
		}
		else if( pluginInterface->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
		{
			defaultPluginInterface = it.value();
		}
	}

	if( defaultPluginInterface == nullptr )
	{
		qCritical() << "NetworkObjectDirectoryManager: no default plugin available! configured plugin:" << configuredPluginUuid;
		return nullptr;
	}

	return defaultPluginInterface->createNetworkObjectDirectory( this );
}
