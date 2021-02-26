/*
 * PluginManager.cpp - implementation of the PluginManager class
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

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>

#include "Logger.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"


PluginManager::PluginManager( QObject* parent ) :
	QObject( parent ),
	m_pluginInterfaces(),
	m_pluginObjects(),
	m_pluginLoaders(),
	m_noDebugMessages( qEnvironmentVariableIsSet( Logger::logLevelEnvironmentVariable() ) )
{
	initPluginSearchPath();
}



PluginManager::~PluginManager()
{
	vDebug();

	for( auto pluginLoader : qAsConst(m_pluginLoaders) )
	{
		pluginLoader->unload();
	}

	m_pluginInterfaces.clear();
	m_pluginObjects.clear();
}



void PluginManager::loadPlatformPlugins()
{
	loadPlugins( QStringLiteral("*-platform") + VeyonCore::sharedLibrarySuffix() );
}



void PluginManager::loadPlugins()
{
	loadPlugins( QStringLiteral("*") + VeyonCore::sharedLibrarySuffix() );
}



void PluginManager::upgradePlugins()
{
	auto versions = VeyonCore::config().pluginVersions();

	for( auto pluginInterface : qAsConst( m_pluginInterfaces ) )
	{
		const auto pluginUid = pluginInterface->uid().toString();
		auto previousPluginVersion = QVersionNumber::fromString( versions.value( pluginUid ).toString() );
		if( previousPluginVersion.isNull() )
		{
			previousPluginVersion = QVersionNumber( 1, 1 );
		}
		const auto currentPluginVersion = pluginInterface->version();
		if( currentPluginVersion > previousPluginVersion )
		{
			vDebug() << "Upgrading plugin" << pluginInterface->name()
					 << "from" << previousPluginVersion.toString()
					 << "to" << currentPluginVersion.toString();
			pluginInterface->upgrade( previousPluginVersion );
		}

		versions[pluginUid] = currentPluginVersion.toString();
	}

	VeyonCore::config().setPluginVersions( versions );
}



void PluginManager::registerExtraPluginInterface( QObject* pluginObject )
{
	auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
	if( pluginInterface )
	{
		m_pluginInterfaces += pluginInterface;
		m_pluginObjects += pluginObject;
	}
}



PluginUidList PluginManager::pluginUids() const
{
	PluginUidList pluginUidList;

	pluginUidList.reserve( m_pluginInterfaces.size() );

	for( auto pluginInterface : qAsConst( m_pluginInterfaces ) )
	{
		pluginUidList += pluginInterface->uid();
	}

	std::sort( pluginUidList.begin(), pluginUidList.end() );

	return pluginUidList;
}



PluginInterface* PluginManager::pluginInterface( Plugin::Uid pluginUid )
{
	for( auto pluginInterface : qAsConst( m_pluginInterfaces ) )
	{
		if( pluginInterface->uid() == pluginUid )
		{
			return pluginInterface;
		}
	}

	return nullptr;
}



QString PluginManager::pluginName( Plugin::Uid pluginUid ) const
{
	for( auto pluginInterface : m_pluginInterfaces )
	{
		if( pluginInterface->uid() == pluginUid )
		{
			return pluginInterface->name();
		}
	}

	return {};
}



void PluginManager::initPluginSearchPath()
{
	QDir dir( QCoreApplication::applicationDirPath() );
	if( dir.cd( VeyonCore::pluginDir() ) )
	{
		const auto pluginSearchPath = dir.absolutePath();
		if( m_noDebugMessages == false )
		{
			vDebug() << "Adding plugin search path" << pluginSearchPath;
		}
		QDir::addSearchPath( QStringLiteral( "plugins" ), pluginSearchPath );
		QCoreApplication::addLibraryPath( pluginSearchPath );
	}
}



void PluginManager::loadPlugins( const QString& nameFilter )
{
	const auto plugins = QDir( QStringLiteral( "plugins:" ) ).entryInfoList( { nameFilter } );
	for( const auto& fileInfo : plugins )
	{
		const auto fileName = fileInfo.fileName();

		// skip simple shared libraries
		if( fileName.startsWith( QLatin1String("lib") ) &&
			fileName.startsWith( QLatin1String("libveyon") ) == false )
		{
			continue;
		}

		auto pluginLoader = new QPluginLoader( fileInfo.filePath(), this );
		auto pluginObject = pluginLoader->instance();
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );

		if( pluginObject && pluginInterface &&
			m_pluginInterfaces.contains( pluginInterface ) == false )
		{
			if( m_noDebugMessages == false )
			{
				vDebug() << "discovered plugin" << pluginInterface->name() << "at" << fileInfo.filePath();
			}
			m_pluginInterfaces += pluginInterface;	// clazy:exclude=reserve-candidates
			m_pluginObjects += pluginObject;		// clazy:exclude=reserve-candidates
			m_pluginLoaders += pluginLoader;			// clazy:exclude=reserve-candidates
		}
		else
		{
			delete pluginLoader;
		}
	}
}
