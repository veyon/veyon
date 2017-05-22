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
	m_proxyClientSocket( clientSocket ),
	m_vncServerSocket( new QTcpSocket( this ) ),
	m_rfbClientToServerMessageSizes( {
									 std::pair<int, int>( rfbSetPixelFormat, sz_rfbSetPixelFormatMsg ),
									 std::pair<int, int>( rfbFramebufferUpdateRequest, sz_rfbFramebufferUpdateRequestMsg ),
									 std::pair<int, int>( rfbKeyEvent, sz_rfbKeyEventMsg ),
									 std::pair<int, int>( rfbPointerEvent, sz_rfbPointerEventMsg ),
									 } )
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



void VncProxyConnection::readFromClient()
{
	if( serverProtocol().state() != VncServerProtocol::Running )
	{
		while( serverProtocol().read() )
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
}



void VncProxyConnection::readFromServer()
{
	if( clientProtocol().state() != VncClientProtocol::Running )
	{
		while( clientProtocol().read() )
		{
		}

		// did we finish client protocol initialization? then we must not miss this
		// read signaĺ from server but process it as the server is still waiting
		// for our response
		if( clientProtocol().state() == VncClientProtocol::Running )
		{
			qDebug("client protocol running");
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
		return m_proxyClientSocket->write( m_vncServerSocket->read( size ) ) == size;
	}

	return false;
}



bool VncProxyConnection::forwardDataToServer( qint64 size )
{
	if( m_proxyClientSocket->bytesAvailable() >= size )
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



bool VncProxyConnection::receiveClientMessage()
{
	auto socket = proxyClientSocket();

	char messageType = 0;
	if( socket->peek( &messageType, sizeof(messageType) ) != sizeof(messageType) )
	{
		return false;
	}

	switch( messageType )
	{
	case rfbSetEncodings:
		if( socket->bytesAvailable() >= sz_rfbSetEncodingsMsg )
		{
			rfbSetEncodingsMsg setEncodingsMessage;
			if( socket->peek( (char *) &setEncodingsMessage, sz_rfbSetEncodingsMsg ) == sz_rfbSetEncodingsMsg )
			{
				qDebug() << "trying to forward set encodings:"
						 << sz_rfbSetEncodingsMsg + qFromBigEndian(setEncodingsMessage.nEncodings) * sizeof(uint32_t);
				return forwardDataToServer( sz_rfbSetEncodingsMsg + qFromBigEndian(setEncodingsMessage.nEncodings) * sizeof(uint32_t) );
			}
		}
		break;

	default:
		if( m_rfbClientToServerMessageSizes.contains( messageType ) == false )
		{
			qCritical( "VncProxyConnection::receiveMessageFromClient(): received unknown message type: %d", (int) messageType );
			socket->close();
			return false;
		}

		return forwardDataToServer( m_rfbClientToServerMessageSizes[messageType] );
	}

	return false;
}



bool VncProxyConnection::receiveServerMessage()
{
	auto socket = vncServerSocket();

	char messageType = 0;
	if( socket->peek( &messageType, sizeof(messageType) ) != sizeof(messageType) )
	{
		return false;
	}

	switch( messageType )
	{
	case rfbFramebufferUpdate:
		return forwardServerFramebufferUpdate();

	case rfbSetColourMapEntries:
		return forwardServerColourMapEntries();

	case rfbBell:
		return forwardDataToClient( sz_rfbBellMsg );

	case rfbServerCutText:
		return forwardServerCutText();

	case rfbResizeFrameBuffer:
		return forwardDataToClient( sz_rfbResizeFrameBufferMsg );

	default:
		qCritical( "ComputerControlClient::receiveMessage(): received unknown message type: %d", (int) messageType );
		socket->close();
		return false;
	}

	return false;
}



bool VncProxyConnection::forwardServerFramebufferUpdate()
{
	// peek all available data and work on a local buffer so we can continously read from it
	QByteArray data = vncServerSocket()->peek( vncServerSocket()->bytesAvailable() );

	QBuffer buffer( &data );
	buffer.open( QBuffer::ReadOnly );

	rfbFramebufferUpdateMsg message;
	if( buffer.read( (char *) &message, sz_rfbFramebufferUpdateMsg ) != sz_rfbFramebufferUpdateMsg )
	{
		return false;
	}

	int nRects = qFromBigEndian( message.nRects );

	for( int i = 0; i < nRects; ++i )
	{
		rfbFramebufferUpdateRectHeader rectHeader;
		if( buffer.read( (char *) &rectHeader, sz_rfbFramebufferUpdateRectHeader ) != sz_rfbFramebufferUpdateRectHeader )
		{
			return false;
		}

		rectHeader.encoding = qFromBigEndian( rectHeader.encoding );
		rectHeader.r.w = qFromBigEndian( rectHeader.r.w );
		rectHeader.r.h = qFromBigEndian( rectHeader.r.h );
		rectHeader.r.x = qFromBigEndian( rectHeader.r.x );
		rectHeader.r.y = qFromBigEndian( rectHeader.r.y );

		if( rectHeader.encoding == rfbEncodingLastRect )
		{
			break;
		}

		if( handleRect( buffer, rectHeader ) == false )
		{
			return false;
		}
	}

	// forward as much data as we read by processing rects
	forwardDataToClient( buffer.pos() );

	return true;
}



bool VncProxyConnection::forwardServerColourMapEntries()
{
	rfbSetColourMapEntriesMsg message;
	if( vncServerSocket()->peek( (char *) &message, sz_rfbSetColourMapEntriesMsg ) != sz_rfbSetColourMapEntriesMsg )
	{
		return false;
	}

	return forwardDataToClient( sz_rfbSetColourMapEntriesMsg + qFromBigEndian( message.nColours ) * 6 );
}



bool VncProxyConnection::forwardServerCutText()
{
	rfbServerCutTextMsg message;
	if( vncServerSocket()->peek( (char *) &message, sz_rfbServerCutTextMsg ) != sz_rfbServerCutTextMsg )
	{
		return false;
	}

	return forwardDataToClient( sz_rfbServerCutTextMsg + qFromBigEndian( message.length ) );
}


bool VncProxyConnection::handleRect( QBuffer& buffer, rfbFramebufferUpdateRectHeader& rectHeader )
{
	const int width = rectHeader.r.w;
	const int height = rectHeader.r.h;

	const int bytesPerPixel = clientProtocol().pixelFormat().bitsPerPixel / 8;
	const int bytesPerRow = ( width + 7 ) / 8;
	const int bytesPerLine = width * bytesPerPixel;
	const int rawSize = width * height * bytesPerPixel;

	switch( rectHeader.encoding )
	{
	case rfbEncodingLastRect:
		return true;

	case rfbEncodingXCursor:
		return width * height == 0 ||
				( buffer.read( sz_rfbXCursorColors ).size() == sz_rfbXCursorColors &&
				  buffer.read( 2 * bytesPerRow * height ).size() == 2 * bytesPerRow * height );

	case rfbEncodingRichCursor:
		return width * height == 0 ||
				( buffer.read( rawSize ).size() == rawSize &&
				  buffer.read( bytesPerRow * height ).size() == bytesPerRow * height );

	case rfbEncodingSupportedMessages:
		return buffer.read( sz_rfbSupportedMessages ).size() == sz_rfbSupportedMessages;

	case rfbEncodingSupportedEncodings:
	case rfbEncodingServerIdentity:
		// width = byte count
		return buffer.read( width ).size() == width;

	case rfbEncodingRaw:
		return buffer.read( bytesPerLine * height ).size() == bytesPerLine * height;

	case rfbEncodingCopyRect:
		return buffer.read( sz_rfbCopyRect ).size() == sz_rfbCopyRect;

	case rfbEncodingRRE:
		return handleRectEncodingRRE( buffer, bytesPerPixel );

	case rfbEncodingCoRRE:
		return handleRectEncodingCoRRE( buffer, bytesPerPixel );

	case rfbEncodingHextile:
		return handleRectEncodingHextile( buffer, rectHeader, bytesPerPixel );

	case rfbEncodingUltra:
	case rfbEncodingUltraZip:
	case rfbEncodingZlib:
		return handleRectEncodingZlib( buffer );

	case rfbEncodingZRLE:
	case rfbEncodingZYWRLE:
		return handleRectEncodingZRLE( buffer );

	case rfbEncodingPointerPos:
	case rfbEncodingKeyboardLedState:
	case rfbEncodingNewFBSize:
		// no further data to read for this rect
		return true;

	default:
		qCritical() << Q_FUNC_INFO << "Unsupported rect encoding" << rectHeader.encoding;
		m_vncServerSocket->close();
		m_proxyClientSocket->close();
		break;
	}

	return false;
}



bool VncProxyConnection::handleRectEncodingRRE( QBuffer& buffer, int bytesPerPixel )
{
	rfbRREHeader hdr;

	if( buffer.read( (char *) &hdr, sz_rfbRREHeader ) != sz_rfbRREHeader )
	{
		return false;
	}

	const int rectDataSize =
			qFromBigEndian( hdr.nSubrects ) * ( bytesPerPixel + sz_rfbRectangle );

	return buffer.read( bytesPerPixel + rectDataSize ).size() == bytesPerPixel + rectDataSize;
}



bool VncProxyConnection::handleRectEncodingCoRRE( QBuffer& buffer, int bytesPerPixel )
{
	rfbRREHeader hdr;

	if( buffer.read( (char *) &hdr, sz_rfbRREHeader ) != sz_rfbRREHeader )
	{
		return false;
	}

	const int rectDataSize =
			qFromBigEndian( hdr.nSubrects ) * ( bytesPerPixel + 4 );

	return buffer.read( bytesPerPixel + rectDataSize ).size() == bytesPerPixel + rectDataSize;

}



bool VncProxyConnection::handleRectEncodingHextile( QBuffer& buffer,
													const rfbFramebufferUpdateRectHeader& rectHeader,
													int bytesPerPixel )
{
	const int rx = rectHeader.r.x;
	const int ry = rectHeader.r.y;
	const int rw = rectHeader.r.w;
	const int rh = rectHeader.r.h;

	for( int y = ry; y < ry+rh; y += 16 )
	{
		for( int x = rx; x < rx+rw; x += 16 )
		{
			int w = 16, h = 16;
			if( rx+rw - x < 16 ) w = rx+rw - x;
			if( ry+rh - y < 16 ) h = ry+rh - y;

			uint8_t subEncoding = 0;
			if( buffer.read( (char *) &subEncoding, 1 ) != 1 )
			{
				return false;
			}

			if( subEncoding & rfbHextileRaw )
			{
				if( buffer.read( w * h * bytesPerPixel ).size() != w * h * bytesPerPixel )
				{
					return false;
				}
				continue;
			}

			if( subEncoding & rfbHextileBackgroundSpecified )
			{
				if( buffer.read( bytesPerPixel ).size() != bytesPerPixel )
				{
					return false;
				}
			}

			if( subEncoding & rfbHextileForegroundSpecified )
			{
				if( buffer.read( bytesPerPixel ).size() != bytesPerPixel )
				{
					return false;
				}
			}

			if( !( subEncoding & rfbHextileAnySubrects ) )
			{
				continue;
			}

			uint8_t nSubrects = 0;
			if( buffer.read( (char *) &nSubrects, 1 ) != 1 )
			{
				return false;
			}

			int subRectDataSize = 0;

			if( subEncoding & rfbHextileSubrectsColoured )
			{
				subRectDataSize = nSubrects * ( 2 + bytesPerPixel );
			}
			else
			{
				subRectDataSize = nSubrects * bytesPerPixel;
			}

			if( buffer.read( subRectDataSize ).size() != subRectDataSize )
			{
				return false;
			}
		}
	}

	return true;
}



bool VncProxyConnection::handleRectEncodingZlib( QBuffer& buffer )
{
	rfbZlibHeader hdr;

	if( buffer.read( (char *) &hdr, sz_rfbZlibHeader ) != sz_rfbZlibHeader )
	{
		return false;
	}

	hdr.nBytes = qFromBigEndian( hdr.nBytes );

	return buffer.read( hdr.nBytes ).size() == static_cast<int64_t>( hdr.nBytes );
}



bool VncProxyConnection::handleRectEncodingZRLE(QBuffer &buffer)
{
	rfbZRLEHeader hdr;

	if( buffer.read( (char *) &hdr, sz_rfbZRLEHeader ) != sz_rfbZRLEHeader )
	{
		return false;
	}

	hdr.length = qFromBigEndian( hdr.length );

	return buffer.read( hdr.length ).size() == static_cast<int64_t>( hdr.length );
}
