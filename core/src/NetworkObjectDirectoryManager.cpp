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
#include "NestedNetworkObjectDirectory.h"


NetworkObjectDirectoryManager::NetworkObjectDirectoryManager( QObject* parent ) :
	QObject( parent )
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto directoryPluginInterface = qobject_cast<NetworkObjectDirectoryPluginInterface *>( pluginObject );

		if( pluginInterface && directoryPluginInterface )
		{
			m_plugins[pluginInterface->uid()] = directoryPluginInterface;
		}
	}
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::configuredDirectory()
{
	if( m_configuredDirectory == nullptr )
	{
		const auto enabledDirectories = VeyonCore::config().enabledNetworkObjectDirectoryPlugins();
		if( enabledDirectories.count() == 1 )
		{
			m_configuredDirectory = createDirectory( Plugin::Uid{enabledDirectories.constFirst()}, this );
		}
		else if( enabledDirectories.count() > 1 )
		{
			const auto nestedDirectory = new NestedNetworkObjectDirectory( this );
			for( const auto& directoryUid : enabledDirectories )
			{
				nestedDirectory->addSubDirectory( createDirectory( Plugin::Uid{directoryUid}, this ) );
			}
			m_configuredDirectory = nestedDirectory;
		}
	}

	return m_configuredDirectory;
}



NetworkObjectDirectory* NetworkObjectDirectoryManager::createDirectory( Plugin::Uid uid, QObject* parent )
{
	const auto plugin = m_plugins.value( uid );
	if( plugin )
	{
		auto directory = plugin->createNetworkObjectDirectory( parent );
		if( directory )
		{
			return directory;
		}
	}

	const auto defaultPlugin = VeyonCore::pluginManager().find<NetworkObjectDirectoryPluginInterface, PluginInterface>(
		[]( const PluginInterface* plugin ) {
			return plugin->flags().testFlag( Plugin::ProvidesDefaultImplementation );
		} );

	if( defaultPlugin )
	{
		const auto defaultDirectory = defaultPlugin->createNetworkObjectDirectory( parent );
		if( defaultDirectory )
		{
			return defaultDirectory;
		}
	}

	vCritical() << "no default plugin available! requested plugin:" << uid;
	return nullptr;
}



void NetworkObjectDirectoryManager::setEnabled( Plugin::Uid uid, bool enabled )
{
	const auto formattedUid = VeyonCore::formattedUuid(uid);

	auto plugins = VeyonCore::config().enabledNetworkObjectDirectoryPlugins();

	if( enabled )
	{
		plugins.append( formattedUid );
		plugins.removeDuplicates();
	}
	else
	{
		plugins.removeAll( formattedUid );
	}

	VeyonCore::config().setEnabledNetworkObjectDirectoryPlugins( plugins );
}



bool NetworkObjectDirectoryManager::isEnabled( Plugin::Uid uid ) const
{
	return VeyonCore::config().enabledNetworkObjectDirectoryPlugins().contains( VeyonCore::formattedUuid(uid) );
}



void NetworkObjectDirectoryManager::setEnabled( NetworkObjectDirectoryPluginInterface* plugin, bool enabled )
{
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		if( it.value() == plugin )
		{
			setEnabled( it.key(), enabled );
		}
	}
}



bool NetworkObjectDirectoryManager::isEnabled( NetworkObjectDirectoryPluginInterface* plugin ) const
{
	for( auto it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it )
	{
		if( it.value() == plugin && isEnabled( it.key() ) )
		{
			return true;
		}
	}

	return false;
}
