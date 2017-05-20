/*
 * VncProxyConnection.cpp - class representing a connection within VncProxyServer
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#include "VncProxyConnection.h"

VncProxyConnection::VncProxyConnection( QTcpSocket* clientSocket, int vncServerPort, QObject* parent ) :
	QObject( parent ),
	m_proxyClientSocket( clientSocket ),
	m_vncServerSocket( new QTcpSocket( this ) )
{
	connect( m_proxyClientSocket, &QTcpSocket::readyRead, this, &VncProxyConnection::readFromClient );
	connect( m_vncServerSocket, &QTcpSocket::readyRead, this, &VncProxyConnection::readFromServer );

	connect( m_vncServerSocket, &QTcpSocket::disconnected, this, &VncProxyConnection::clientConnectionClosed );
	connect( m_proxyClientSocket, &QTcpSocket::disconnected, this, &VncProxyConnection::serverConnectionClosed );

	m_vncServerSocket->connectToHost( QHostAddress::LocalHost, vncServerPort );
}



VncProxyConnection::~VncProxyConnection()
{
	delete m_vncServerSocket;
	delete m_proxyClientSocket;
}



bool VncProxyConnection::forwardDataToClient( qint64 size )
{
	if( size <= 0 )
	{
		return m_proxyClientSocket->write( m_vncServerSocket->readAll() ) > 0;
	}
	else if( m_vncServerSocket->bytesAvailable() >= size )
	{
		return m_proxyClientSocket->write( m_vncServerSocket->read( size ) ) == size;
	}

	return false;
}



bool VncProxyConnection::forwardDataToServer( qint64 size )
{
	if( size <= 0 )
	{
		return m_vncServerSocket->write( m_proxyClientSocket->readAll() ) > 0;
	}
	else if( m_proxyClientSocket->bytesAvailable() >= size )
	{
		return m_vncServerSocket->write( m_proxyClientSocket->read( size ) ) == size;
	}

	return false;
}



void VncProxyConnection::readFromServerLater()
{
	QTimer::singleShot( ProtocolRetryTime, this, &VncProxyConnection::readFromServer );
}



void VncProxyConnection::readFromClientLater()
{
	QTimer::singleShot( ProtocolRetryTime, this, &VncProxyConnection::readFromClient );
}
