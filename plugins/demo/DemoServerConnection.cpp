/*
 * DemoServer.cpp - multi-threaded slim VNC-server for demo-purposes (optimized
 *                   for lot of clients accessing server in read-only-mode)
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

#include <QTcpSocket>

#include "DemoConfiguration.h"
#include "DemoServer.h"
#include "DemoServerConnection.h"


DemoServerConnection::DemoServerConnection( const QString& demoAccessToken,
											QTcpSocket* socket,
											DemoServer* demoServer ) :
	QObject( demoServer ),
	m_demoServer( demoServer ),
	m_socket( socket ),
	m_vncServerClient(),
	m_serverProtocol( demoAccessToken, m_socket, &m_vncServerClient ),
	m_rfbClientToServerMessageSizes( {
									 std::pair<int, int>( rfbSetPixelFormat, sz_rfbSetPixelFormatMsg ),
									 std::pair<int, int>( rfbFramebufferUpdateRequest, sz_rfbFramebufferUpdateRequestMsg ),
									 std::pair<int, int>( rfbKeyEvent, sz_rfbKeyEventMsg ),
									 std::pair<int, int>( rfbPointerEvent, sz_rfbPointerEventMsg ),
									 } ),
	m_keyFrame( -1 ),
	m_framebufferUpdateMessageIndex( 0 ),
	m_framebufferUpdateInterval( m_demoServer->configuration().framebufferUpdateInterval() )
{
	connect( m_socket, &QTcpSocket::readyRead, this, &DemoServerConnection::processClient );
	connect( m_socket, &QTcpSocket::disconnected, this, &DemoServerConnection::deleteLater );

	m_serverProtocol.setServerInitMessage( m_demoServer->serverInitMessage() );
	m_serverProtocol.start();
}



DemoServerConnection::~DemoServerConnection()
{
	delete m_socket;
}



void DemoServerConnection::processClient()
{
	if( m_serverProtocol.state() != VncServerProtocol::Running )
	{
		while( m_serverProtocol.read() )
		{
		}

		// try again later in case we could not proceed because of
		// external protocol dependencies or in case we're finished
		// and already have RFB messages in receive queue
		QTimer::singleShot( ProtocolRetryTime, this, &DemoServerConnection::processClient );
	}
	else
	{
		while( receiveClientMessage() )
		{
		}
	}
}



bool DemoServerConnection::receiveClientMessage()
{
	char messageType = 0;
	if( m_socket->peek( &messageType, sizeof(messageType) ) != sizeof(messageType) )
	{
		return false;
	}

	switch( messageType )
	{
	case rfbSetEncodings:
		if( m_socket->bytesAvailable() >= sz_rfbSetEncodingsMsg )
		{
			rfbSetEncodingsMsg setEncodingsMessage;
			if( m_socket->peek( (char *) &setEncodingsMessage, sz_rfbSetEncodingsMsg ) == sz_rfbSetEncodingsMsg )
			{
				const qint64 totalSize = sz_rfbSetEncodingsMsg + qFromBigEndian(setEncodingsMessage.nEncodings) * sizeof(uint32_t);
				if( m_socket->bytesAvailable() >= totalSize )
				{
					return m_socket->read( totalSize ).size() == totalSize;
				}
			}
		}
		break;

	default:
		if( m_rfbClientToServerMessageSizes.contains( messageType ) == false )
		{
			qCritical( "DemoServerConnection::receiveMessageFromClient(): received unknown message type: %d", (int) messageType );
			m_socket->close();
			return false;
		}

		// do not yet read any data if not enough is available for reading
		if( m_socket->bytesAvailable() < m_rfbClientToServerMessageSizes[messageType] )
		{
			return false;
		}

		m_socket->read( m_rfbClientToServerMessageSizes[messageType] );

		if( messageType == rfbFramebufferUpdateRequest )
		{
			sendFramebufferUpdate();
		}

		return true;
	}

	return false;
}



void DemoServerConnection::sendFramebufferUpdate()
{
	m_demoServer->lockDataForRead();

	const auto& framebufferUpdateMessages = m_demoServer->framebufferUpdateMessages();

	const int framebufferUpdateMessageCount = framebufferUpdateMessages.count();

	if( m_demoServer->keyFrame() != m_keyFrame ||
			m_framebufferUpdateMessageIndex > framebufferUpdateMessageCount )
	{
		m_framebufferUpdateMessageIndex = 0;
		m_keyFrame = m_demoServer->keyFrame();
	}

	bool sentUpdates = false;
	for( ; m_framebufferUpdateMessageIndex < framebufferUpdateMessageCount; ++m_framebufferUpdateMessageIndex )
	{
		m_socket->write( framebufferUpdateMessages[m_framebufferUpdateMessageIndex] );
		sentUpdates = true;
	}

	m_demoServer->unlockData();

	if( sentUpdates == false )
	{
		// did not send updates but client still waiting for update? then try again soon
		QTimer::singleShot( m_framebufferUpdateInterval, this, &DemoServerConnection::sendFramebufferUpdate );
	}
}
