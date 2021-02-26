/*
 * VncServer.cpp - implementation of VncServer, a VNC-server-
 *                      abstraction for platform independent VNC-server-usage
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "rfb/rfbproto.h"

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "VeyonConfiguration.h"
#include "PlatformSessionFunctions.h"
#include "PluginManager.h"
#include "VncServer.h"
#include "VncServerPluginInterface.h"


VncServer::VncServer( QObject* parent ) :
	QThread( parent ),
	m_pluginInterface( nullptr )
{
	const auto currentSessionType = VeyonCore::platform().sessionFunctions().currentSessionType();

	VeyonCore::authenticationCredentials().setInternalVncServerPassword(
				CryptoCore::generateChallenge().toBase64().left( MAXPWLEN ) );

	VncServerPluginInterfaceList defaultVncServerPlugins;

	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto vncServerPluginInterface = qobject_cast<VncServerPluginInterface *>( pluginObject );

		if( pluginInterface && vncServerPluginInterface )
		{
			// skip VNC server plugins which support certain session types only and do not support
			// the current session type
			if( vncServerPluginInterface->supportedSessionTypes().isEmpty() == false &&
				vncServerPluginInterface->supportedSessionTypes().contains( currentSessionType, Qt::CaseInsensitive ) == false )
			{
				continue;
			}

			if( pluginInterface->uid() == VeyonCore::config().vncServerPlugin() )
			{
				m_pluginInterface = vncServerPluginInterface;
			}
			else if( pluginInterface->flags().testFlag( Plugin::ProvidesDefaultImplementation ) )
			{
				defaultVncServerPlugins.append( vncServerPluginInterface ); // clazy:exclude=reserve-candidates
			}
		}
	}

	if( m_pluginInterface == nullptr )
	{
		if( defaultVncServerPlugins.isEmpty() )
		{
			vCritical() << "no VNC server plugins found!";
		}
		else
		{
			m_pluginInterface = defaultVncServerPlugins.first();
		}
	}
}



VncServer::~VncServer()
{
	vDebug();
}



void VncServer::prepare()
{
	vDebug();

	if( m_pluginInterface )
	{
		m_pluginInterface->prepareServer();
	}
}



int VncServer::serverBasePort() const
{

	if( m_pluginInterface && m_pluginInterface->configuredServerPort() > 0 )
	{
		return m_pluginInterface->configuredServerPort();
	}

	return VeyonCore::config().vncServerPort();
}



int VncServer::serverPort() const
{
	return serverBasePort() + VeyonCore::sessionId();
}



VncServer::Password VncServer::password() const
{
	if( m_pluginInterface && m_pluginInterface->configuredPassword().isEmpty() == false )
	{
		return m_pluginInterface->configuredPassword();
	}

	return VeyonCore::authenticationCredentials().internalVncServerPassword();
}



void VncServer::run()
{
	if( m_pluginInterface )
	{
		vDebug() << "running";

		if( m_pluginInterface->configuredServerPort() > 0 )
		{
			VeyonCore::config().setVncServerPort( m_pluginInterface->configuredServerPort() );
		}

		if( m_pluginInterface->configuredPassword().isEmpty() == false )
		{
			VeyonCore::authenticationCredentials().setInternalVncServerPassword( m_pluginInterface->configuredPassword() );
		}

		if( m_pluginInterface->runServer( serverPort(), password() ) == false )
		{
			vCritical() << "An error occurred while running the VNC server plugin";
		}

		vDebug() << "finished";
	}
}
