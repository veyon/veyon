/*
 * VncServer.cpp - implementation of VncServer, a VNC-server-
 *                      abstraction for platform independent VNC-server-usage
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "ItalcCore.h"

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "ItalcConfiguration.h"
#include "PluginManager.h"
#include "VncServer.h"
#include "VncServerPluginInterface.h"


VncServer::VncServer( int serverPort ) :
	QThread(),
	m_password( CryptoCore::generateChallenge().toBase64().left( MAXPWLEN ) ),
	m_serverPort( serverPort ),
	m_pluginInterface( nullptr )
{
	ItalcCore::authenticationCredentials().setInternalVncServerPassword( m_password );

	VncServerPluginInterfaceList availableVncServerPlugins;

	for( auto pluginObject : ItalcCore::pluginManager().pluginObjects() )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto vncServerPluginInterface = qobject_cast<VncServerPluginInterface *>( pluginObject );

		if( pluginInterface && vncServerPluginInterface )
		{
			availableVncServerPlugins.append( vncServerPluginInterface );

			if( pluginInterface->uid() == ItalcCore::config().vncServerPlugin() )
			{
				m_pluginInterface = vncServerPluginInterface;
			}
		}
	}

	if( m_pluginInterface == nullptr )
	{
		if( availableVncServerPlugins.isEmpty() )
		{
			qCritical( "VncServer::VncServer(): no VNC server plugins found!" );
		}
		else
		{
			m_pluginInterface = availableVncServerPlugins.first();
		}
	}
}



VncServer::~VncServer()
{
}


void VncServer::run()
{
	if( m_pluginInterface )
	{
		m_pluginInterface->run( serverPort(), password() );
	}
}
