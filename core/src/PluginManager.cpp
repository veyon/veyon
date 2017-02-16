/*
 * PluginManager.cpp - implementation of the PluginManager class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

#include "PluginManager.h"

#ifdef ITALC_BUILD_WIN32
static const QStringList nameFilters("*.dll");
#else
static const QStringList nameFilters("*.so");
#endif


PluginManager::PluginManager( QObject* parent ) :
	QObject( parent ),
	m_pluginInterfaces()
{
	// adds a search path relative to the main executable to if the path exists.
	auto addRelativeIfExists = [this]( const QString& path )
	{
		QDir dir(qApp->applicationDirPath());
		if( !path.isEmpty() && dir.cd( path ) )
		{
			QDir::addSearchPath( "plugins", dir.absolutePath() );
		}
	};

#ifdef Q_OS_LINUX
	addRelativeIfExists( "../plugins" );
#else
	addRelativeIfExists( "plugins" );
#endif

	for( auto fileInfo : QDir( "plugins:" ).entryInfoList( nameFilters ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( QPluginLoader( fileInfo.filePath() ).instance() );

		if( pluginInterface )
		{
			qDebug() << "PluginManager: discovered plugin" << pluginInterface->name() << "at" << fileInfo.filePath();
			m_pluginInterfaces += pluginInterface;
		}
	}
}



void PluginManager::registerExtraPluginInterface( PluginInterface* pluginInterface )
{
	m_pluginInterfaces += pluginInterface;
}



PluginUidList PluginManager::pluginUids() const
{
	PluginUidList pluginUidList;

	for( auto pluginInterface : m_pluginInterfaces )
	{
		pluginUidList += pluginInterface->uid();
	}

	std::sort( pluginUidList.begin(), pluginUidList.end() );

	return pluginUidList;
}



PluginInterface* PluginManager::pluginInterface( const Plugin::Uid& pluginUid )
{
	for( auto pluginInterface : m_pluginInterfaces )
	{
		if( pluginInterface->uid() == pluginUid )
		{
			return pluginInterface;
		}
	}

	return nullptr;
}



QString PluginManager::pluginName( const Plugin::Uid& pluginUid ) const
{
	for( auto pluginInterface : m_pluginInterfaces )
	{
		if( pluginInterface->uid() == pluginUid )
		{
			return pluginInterface->name();
		}
	}

	return QString();
}
