/*
 * WebApiPlugin.cpp - implementation of WebApiPlugin class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "CommandLineIO.h"
#include "WebApiConfigurationPage.h"
#include "WebApiHttpServer.h"
#include "WebApiPlugin.h"


WebApiPlugin::WebApiPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() ),
	m_httpServerThread( this ),
	m_commands( {
		{ QStringLiteral("runserver"), tr( "Run WebAPI server" ) }
	} )
{
	if( VeyonCore::component() == VeyonCore::Component::Service &&
		m_configuration.httpServerEnabled() )
	{
		connect( VeyonCore::instance(), &VeyonCore::initialized,
				 this, &WebApiPlugin::startHttpServerThread );
	}
}



WebApiPlugin::~WebApiPlugin()
{
	if( m_httpServerThread.isRunning() )
	{
		m_httpServerThread.quit();
		m_httpServerThread.wait( ServerThreadTerminationTimeout );
	}
}



ConfigurationPage* WebApiPlugin::createConfigurationPage()
{
	return new WebApiConfigurationPage( m_configuration );
}



CommandLinePluginInterface::RunResult WebApiPlugin::handle_runserver( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	m_httpServer = new WebApiHttpServer{ m_configuration };

	if( m_httpServer->start() == false )
	{
		CommandLineIO::error( tr( "Failed to start WebAPI server at port %1" ).arg( m_configuration.httpServerPort() ) );

		return Failed;
	}

	CommandLineIO::info( tr("WebAPI server running at port %1").arg( m_configuration.httpServerPort() ) );

	return VeyonCore::instance()->exec() == 0 ? Successful : Failed;
}



void WebApiPlugin::startHttpServerThread()
{
	m_httpServer = new WebApiHttpServer{ m_configuration };
	m_httpServer->moveToThread( &m_httpServerThread );

	connect( &m_httpServerThread, &QThread::started, m_httpServer, &WebApiHttpServer::start );

	m_httpServerThread.start();
}


IMPLEMENT_CONFIG_PROXY(WebApiConfiguration)
