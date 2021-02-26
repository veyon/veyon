/*
 * NetworkObjectDirectoryManager.cpp - implementation of NetworkObjectDirectoryManager
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
		m_configuredDirectory = createDirectory( VeyonCore::config().networkObjectDirectoryPlugin(), this );
	}

	return m_configuredDirectory;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory( Plugin::Uid uid, QObject* parent )
{
	for( auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it )
	{
		if( it.key()->uid() == uid )
		{
			auto directory = it.value()->createNetworkObjectDirectory( parent );
			if( directory )
			{
				return directory;
			}
		}
	}

	for( auto it = m_directoryPluginInterfaces.constBegin(), end = m_directoryPluginInterfaces.constEnd(); it != end; ++it )
	{
		if( it.key()->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
		{
			auto defaultDirectory = it.value()->createNetworkObjectDirectory( parent );
			if( defaultDirectory )
			{
				return defaultDirectory;
			}
		}
	}

	vCritical() << "no default plugin available! requested plugin:" << uid;
	return nullptr;
}
