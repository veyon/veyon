/*
 * ComputerControlClient.cpp - implementation of the ComputerControlClient class
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

#include <QtEndian>
#include <QTcpSocket>
#include <QTimer>

#include <rfb/rfbproto.h>

#include "ComputerControlClient.h"
#include "ComputerControlServer.h"


ComputerControlClient::ComputerControlClient( ComputerControlServer* server,
											  QTcpSocket* clientSocket,
											  int vncServerPort,
											  const QString& vncServerPassword,
											  QObject* parent ) :
	VncProxyConnection( clientSocket, vncServerPort, parent ),
	m_server( server ),
	m_serverClient(),
	m_serverProtocol( proxyClientSocket(),
					  &m_serverClient,
					  server->authenticationManager(),
					  server->accessControlManager() ),
	m_clientProtocol( vncServerSocket(), vncServerPassword )
{
	m_rfbMessageSizes[rfbSetPixelFormat] = sz_rfbSetPixelFormatMsg;
	m_rfbMessageSizes[rfbFramebufferUpdateRequest] = sz_rfbFramebufferUpdateRequestMsg;
	m_rfbMessageSizes[rfbKeyEvent] = sz_rfbKeyEventMsg;
	m_rfbMessageSizes[rfbPointerEvent] = sz_rfbPointerEventMsg;

	m_serverProtocol.start();
	m_clientProtocol.start();
}



ComputerControlClient::~ComputerControlClient()
{
	m_server->accessControlManager().removeClient( &m_serverClient );
}



void ComputerControlClient::readFromClient()
{
	if( m_serverClient.protocolState() != VncServerClient::Initialized )
	{
		while( m_serverProtocol.read() )
		{
		}

		// did we finish server protocol initialization? then we must not miss this
		// read signaĺ from client but process it as the client is still waiting
		// for our response
		if( m_serverClient.protocolState() == VncServerClient::Initialized )
		{
			readFromClient();
		}
	}
	else if( m_clientProtocol.state() == VncClientProtocol::Authenticated )
	{
		while( proxyClientSocket()->bytesAvailable() >= 1 && receiveMessage() )
		{
		}
	}
	else
	{
		// try again as client connection is not yet ready and we can't forward data
		QTimer::singleShot( ProtocolRetryTime, this, &ComputerControlClient::readFromClient );
	}
}



void ComputerControlClient::readFromServer()
{
	if( m_clientProtocol.state() != VncClientProtocol::Authenticated )
	{
		while( m_clientProtocol.read() )
		{
		}

		// did we finish client protocol initialization? then we must not miss this
		// read signaĺ from server but process it as the server is still waiting
		// for our response
		if( m_clientProtocol.state() == VncClientProtocol::Authenticated )
		{
			readFromServer();
		}
	}
	else if( m_serverClient.protocolState() == VncServerClient::Initialized )
	{
		forwardDataToClient();
	}
	else
	{
		// try again as server connection is not yet ready and we can't forward data
		QTimer::singleShot( ProtocolRetryTime, this, &ComputerControlClient::readFromServer );
	}
}



bool ComputerControlClient::receiveMessage()
{
	QTcpSocket* socket = proxyClientSocket();

	char messageType = 0;
	if( socket->peek( &messageType, sizeof(messageType) ) != sizeof(messageType) )
	{
		qCritical( "ComputerControlClient:::receiveMessage(): could not peek message type - closing connection" );

		deleteLater();
		return false;
	}

	int rfbMessageSize = m_rfbMessageSizes.value( messageType, 0 );

	switch( messageType )
	{
	case rfbItalcFeatureMessage:
		return m_server->handleFeatureMessage( socket );

	case rfbSetEncodings:
		if( socket->bytesAvailable() >= sz_rfbSetEncodingsMsg )
		{
			rfbSetEncodingsMsg setEncodingsMessage;
			if( socket->peek( (char *) &setEncodingsMessage, sz_rfbSetEncodingsMsg ) == sz_rfbSetEncodingsMsg )
			{
				return forwardDataToServer( sz_rfbSetEncodingsMsg + qFromBigEndian(setEncodingsMessage.nEncodings) * sizeof(uint32_t) );
			}
		}
		break;

	default:
		return forwardDataToServer( rfbMessageSize );

	}

	return false;
}
