/*
 * VncProxyServer.cpp - a VNC proxy implementation for intercepting VNC connections
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

#include <QTcpServer>
#include <QTcpSocket>

#include "VncProxyServer.h"
#include "VncProxyClient.h"
#include "VncProxyClientFactory.h"


VncProxyServer::VncProxyServer( int vncServerPort, int listenPort, VncProxyClientFactory* clientFactory, QObject* parent ) :
	QObject( parent ),
	m_vncServerPort( vncServerPort ),
	m_listenPort( listenPort ),
	m_server( new QTcpServer( this ) ),
	m_clientFactory( clientFactory )
{
	connect( m_server, &QTcpServer::newConnection, this, &VncProxyServer::acceptConnection );
}



VncProxyServer::~VncProxyServer()
{
	for( auto client : m_clients )
	{
		delete client;
	}

	delete m_server;
}



void VncProxyServer::start()
{
	if( m_server->listen( QHostAddress::Any, m_listenPort ) == false )
	{
		qWarning() << "VncProxyServer: could not listen on port" << m_listenPort << m_server->errorString();
	}

	qDebug( "VncProxyServer started on port %d", m_listenPort );

}



void VncProxyServer::acceptConnection()
{
	VncProxyClient* client = m_clientFactory->createVncProxyClient( m_server->nextPendingConnection(), m_vncServerPort, this );

	connect( client, &VncProxyClient::clientConnectionClosed, [=]() { closeConnection( client ); } );
	connect( client, &VncProxyClient::serverConnectionClosed, [=]() { closeConnection( client ); } );

	m_clients += client;
}



void VncProxyServer::closeConnection( VncProxyClient* client )
{
	m_clients.removeAll( client );

	client->deleteLater();
}
