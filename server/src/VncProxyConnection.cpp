/*
 * VncProxyConnection.cpp - class representing a connection within VncProxyServer
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

#include <QBuffer>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#include "VncClientProtocol.h"
#include "VncProxyConnection.h"
#include "VncServerProtocol.h"

VncProxyConnection::VncProxyConnection( QTcpSocket* clientSocket,
										int vncServerPort,
										QObject* parent ) :
	QObject( parent ),
	m_vncServerPort( vncServerPort ),
	m_proxyClientSocket( clientSocket ),
	m_vncServerSocket( new QTcpSocket( this ) ),
	m_rfbClientToServerMessageSizes( {
		{ rfbSetPixelFormat, sz_rfbSetPixelFormatMsg },
		{ rfbFramebufferUpdateRequest, sz_rfbFramebufferUpdateRequestMsg },
		{ rfbKeyEvent, sz_rfbKeyEventMsg },
		{ rfbPointerEvent, sz_rfbPointerEventMsg },
		{ rfbXvp, sz_rfbXvpMsg },
		} )
{
	connect( m_proxyClientSocket, &QTcpSocket::readyRead, this, &VncProxyConnection::readFromClient );
	connect( m_vncServerSocket, &QTcpSocket::readyRead, this, &VncProxyConnection::readFromServer );

	connect( m_vncServerSocket, &QTcpSocket::disconnected, this, &VncProxyConnection::clientConnectionClosed );
	connect( m_proxyClientSocket, &QTcpSocket::disconnected, this, &VncProxyConnection::serverConnectionClosed );
}



VncProxyConnection::~VncProxyConnection()
{
	// do not get notified about disconnects any longer
	disconnect( m_vncServerSocket );
	disconnect( m_proxyClientSocket );

	delete m_vncServerSocket;
	delete m_proxyClientSocket;
}



void VncProxyConnection::start()
{
	serverProtocol().start();
}



void VncProxyConnection::readFromClient()
{
	if( serverProtocol().state() != VncServerProtocol::Running )
	{
		while( serverProtocol().read() ) // Flawfinder: ignore
		{
		}

		// try again later in case we could not proceed because of
		// external protocol dependencies or in case we're finished
		// and already have RFB messages in receive queue
		readFromClientLater();
	}
	else if( clientProtocol().state() == VncClientProtocol::Running )
	{
		while( receiveClientMessage() )
		{
		}
	}
	else
	{
		// try again as client connection is not yet ready and we can't forward data
		readFromClientLater();
	}

	if( serverProtocol().state() == VncServerProtocol::FramebufferInit &&
		clientProtocol().state() == VncClientProtocol::Disconnected )
	{
		m_vncServerSocket->connectToHost( QHostAddress::LocalHost, quint16(m_vncServerPort) );

		clientProtocol().start();
	}
}



void VncProxyConnection::readFromServer()
{
	if( clientProtocol().state() != VncClientProtocol::Running )
	{
		while( clientProtocol().read() ) // Flawfinder: ignore
		{
		}

		// did we finish client protocol initialization? then we must not miss this
		// read signaĺ from server but process it as the server is still waiting
		// for our response
		if( clientProtocol().state() == VncClientProtocol::Running )
		{
			// if client protocol is running we have the server init message which
			// we can forward to the real client
			serverProtocol().setServerInitMessage( clientProtocol().serverInitMessage() );

			readFromServerLater();
		}
	}
	else if( serverProtocol().state() == VncServerProtocol::Running )
	{
		while( receiveServerMessage() )
		{
		}
	}
	else
	{
		// try again as server connection is not yet ready and we can't forward data
		readFromServerLater();
	}
}



bool VncProxyConnection::forwardDataToClient( qint64 size )
{
	if( m_vncServerSocket->bytesAvailable() >= size )
	{
		const auto data = m_vncServerSocket->read( size ); // Flawfinder: ignore
		if( data.size() == size )
		{
			return m_proxyClientSocket->write( data ) == size;
		}
	}

	return false;
}



bool VncProxyConnection::forwardDataToServer( qint64 size )
{
	if( m_proxyClientSocket->bytesAvailable() >= size )
	{
		const auto data = m_proxyClientSocket->read( size ); // Flawfinder: ignore
		if( data.size() == size )
		{
			return m_vncServerSocket->write( data ) == size;
		}
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



bool VncProxyConnection::receiveClientMessage()
{
	auto socket = proxyClientSocket();

	uint8_t messageType = 0;
	if( socket->peek( reinterpret_cast<char *>( &messageType ), sizeof(messageType) ) != sizeof(messageType) )
	{
		return false;
	}

	switch( messageType )
	{
	case rfbSetEncodings:
		if( socket->bytesAvailable() >= sz_rfbSetEncodingsMsg )
		{
			rfbSetEncodingsMsg setEncodingsMessage;
			if( socket->peek( reinterpret_cast<char *>( &setEncodingsMessage ), sz_rfbSetEncodingsMsg ) == sz_rfbSetEncodingsMsg )
			{
				const auto nEncodings = qFromBigEndian(setEncodingsMessage.nEncodings);
				if( nEncodings > MAX_ENCODINGS )
				{
					vCritical() << "received too many encodings from client";
					socket->close();
					return false;
				}
				return forwardDataToServer( sz_rfbSetEncodingsMsg + nEncodings * sizeof(uint32_t) );
			}
		}
		break;

	default:
		if( m_rfbClientToServerMessageSizes.contains( messageType ) == false )
		{
			vCritical() << "received unknown message type:" << static_cast<int>( messageType );
			socket->close();
			return false;
		}

		return forwardDataToServer( m_rfbClientToServerMessageSizes[messageType] );
	}

	return false;
}



bool VncProxyConnection::receiveServerMessage()
{
	if( clientProtocol().receiveMessage() )
	{
		m_proxyClientSocket->write( clientProtocol().lastMessage() );

		return true;
	}

	return false;
}
