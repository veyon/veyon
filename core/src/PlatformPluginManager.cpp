/*
 * PlatformPluginManager.cpp - implementation of PlatformPluginManager
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

#include "PluginManager.h"
#include "PlatformPluginManager.h"


PlatformPluginManager::PlatformPluginManager( PluginManager& pluginManager, QObject* parent ) :
	QObject( parent ),
	m_platformPlugin( nullptr )
{
	for( auto pluginObject : qAsConst( pluginManager.pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto platformPluginInterface = qobject_cast<PlatformPluginInterface *>( pluginObject );

		if( pluginInterface && platformPluginInterface )
		{
			m_platformPlugin = platformPluginInterface;
		}
	}

	if( m_platformPlugin == nullptr )
	{
		qFatal( "PlatformPluginManager: no platform plugin available!" );
	}
}
