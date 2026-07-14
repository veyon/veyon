/*
 * VncProxyServer.cpp - a VNC proxy implementation for intercepting VNC connections
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QTcpServer>
#include <QTcpSocket>

#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "VncProxyServer.h"
#include "VncProxyConnection.h"
#include "VncProxyConnectionFactory.h"


VncProxyServer::VncProxyServer( const QHostAddress& listenAddress,
								int listenPort,
								VncProxyConnectionFactory* connectionFactory,
								QObject* parent ) :
	QObject( parent ),
	m_vncServerPort( -1 ),
	m_vncServerPassword(),
	m_listenAddress( listenAddress ),
	m_listenPort( listenPort ),
	m_server( new QTcpServer( this ) ),
	m_connectionFactory( connectionFactory )
{
	connect( m_server, &QTcpServer::newConnection, this, &VncProxyServer::acceptConnection );
	connect( m_server, &QTcpServer::acceptError, this, &VncProxyServer::handleAcceptError );
}



VncProxyServer::~VncProxyServer()
{
	stop();
}



bool VncProxyServer::start( int vncServerPort, const Password& vncServerPassword )
{
	if( m_server->isListening() )
	{
		vWarning() << "VNC proxy server is already listening";
		return false;
	}

	m_vncServerPort = vncServerPort;
	m_vncServerPassword = vncServerPassword;

	if( m_listenPort < 0 ||
			m_server->listen( m_listenAddress, static_cast<quint16>( m_listenPort ) ) == false )
	{
		vWarning() << "could not listen on port" << m_listenPort << m_server->errorString();
		return false;
	}

	vDebug() << "started on port" << m_listenPort;
	return true;
}



void VncProxyServer::stop()
{
	if( m_server )
	{
		m_server->close();
	}

	for( auto connection : std::as_const( m_connections ) )
	{
		delete connection;
	}

	m_connections.clear();
}



void VncProxyServer::acceptConnection()
{
	while( m_server->hasPendingConnections() )
	{
		auto clientSocket = m_server->nextPendingConnection();
		if( clientSocket == nullptr )
		{
			vCritical() << "ignoring invalid client socket";
			continue;
		}

		const auto connectionLimit = qMax(1, VeyonCore::config().serverConnectionLimit());
		if( m_connections.size() >= connectionLimit )
		{
			vWarning() << "rejecting connection from" << clientSocket->peerAddress().toString()
					   << "because the connection limit of" << connectionLimit << "has been reached";
			clientSocket->abort();
			clientSocket->deleteLater();
			continue;
		}

		auto connection = m_connectionFactory->createVncProxyConnection( clientSocket,
														 m_vncServerPort,
														 m_vncServerPassword,
														 this );
		if( connection == nullptr )
		{
			vCritical() << "connection factory returned no VNC proxy connection";
			clientSocket->abort();
			clientSocket->deleteLater();
			continue;
		}

		connect(connection, &VncProxyConnection::serverMessageProcessed, this,
			[=, this]() { Q_EMIT serverMessageProcessed(connection); }, Qt::DirectConnection);

		connect(connection, &VncProxyConnection::clientConnectionClosed, this, [=, this]() { closeConnection(connection); });
		connect(connection, &VncProxyConnection::serverConnectionClosed, this, [=, this]() { closeConnection(connection); });

		m_connections += connection;

		connection->start();
	}
}



void VncProxyServer::closeConnection( VncProxyConnection* connection )
{
	if( m_connections.removeAll(connection) == 0 )
	{
		return;
	}

	Q_EMIT connectionClosed( connection );

	connection->deleteLater();
}



void VncProxyServer::handleAcceptError( QAbstractSocket::SocketError socketError )
{
	vCritical() << "error while accepting connection" << socketError;
}
