/*
 * VncServer.cpp - implementation of VncServer, a VNC-server-
 *                      abstraction for platform independent VNC-server-usage
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "VeyonConfiguration.h"
#include "PluginManager.h"
#include "VncServer.h"
#include "VncServerPluginInterface.h"

#include "rfb/rfbproto.h"


VncServer::VncServer( QObject* parent ) :
	QThread( parent ),
	m_pluginInterface( nullptr )
{
	VeyonCore::authenticationCredentials().setInternalVncServerPassword(
				CryptoCore::generateChallenge().toBase64().left( MAXPWLEN ) );

	VncServerPluginInterfaceList defaultVncServerPlugins;

	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto vncServerPluginInterface = qobject_cast<VncServerPluginInterface *>( pluginObject );

		if( pluginInterface && vncServerPluginInterface )
		{
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
			qCritical( "VncServer::VncServer(): no VNC server plugins found!" );
		}
		else
		{
			m_pluginInterface = defaultVncServerPlugins.first();
		}
	}
}



VncServer::~VncServer()
{
	qDebug(Q_FUNC_INFO);
}



void VncServer::prepare()
{
	qDebug(Q_FUNC_INFO);

	if( m_pluginInterface )
	{
		m_pluginInterface->prepareServer();
	}
}



int VncServer::serverPort() const
{
	if( m_pluginInterface && m_pluginInterface->configuredServerPort() > 0 )
	{
		return m_pluginInterface->configuredServerPort() + VeyonCore::sessionId();
	}

	return VeyonCore::config().vncServerPort() + VeyonCore::sessionId();
}



QString VncServer::password() const
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
		qDebug() << Q_FUNC_INFO << "running";

		if( m_pluginInterface->configuredServerPort() > 0 )
		{
			VeyonCore::config().setVncServerPort( m_pluginInterface->configuredServerPort() );
		}

		if( m_pluginInterface->configuredPassword().isEmpty() == false )
		{
			VeyonCore::authenticationCredentials().setInternalVncServerPassword( m_pluginInterface->configuredPassword() );
		}

		m_pluginInterface->runServer( serverPort(), password() );

		qDebug() << Q_FUNC_INFO << "finished";
	}
}
