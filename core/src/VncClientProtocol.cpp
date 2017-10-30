/*
 * VncClientProtocol.cpp - implementation of the VncClientProtocol class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "VeyonCore.h"

#include <QBuffer>
#include <QRegion>
#include <QTcpSocket>

extern "C"
{
#include "rfb/rfbproto.h"
#include "common/d3des.h"
}

#include "VncClientProtocol.h"


/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */

static void
vncEncryptBytes(unsigned char *bytes, const char *passwd)
{
	unsigned char key[8];
	unsigned int i;

	/* key is simply password padded with nulls */

	for (i = 0; i < 8; i++) {
		if (i < strlen(passwd)) {
			key[i] = passwd[i];
		} else {
			key[i] = 0;
		}
	}

	rfbDesKey(key, EN0);

	for (i = 0; i < CHALLENGESIZE; i += 8) {
		rfbDes(bytes+i, bytes+i);
	}
}




VncClientProtocol::VncClientProtocol( QTcpSocket* socket, const QString& vncPassword ) :
	m_socket( socket ),
	m_state( Disconnected ),
	m_vncPassword( vncPassword.toUtf8() ),
	m_serverInitMessage(),
	m_framebufferWidth( 0 ),
	m_framebufferHeight( 0 )
{
	memset( &m_pixelFormat, 0, sz_rfbPixelFormat );
}



void VncClientProtocol::start()
{
	m_state = Protocol;
}



bool VncClientProtocol::read()
{
	switch( m_state )
	{
	case Protocol:
		return readProtocol();

	case SecurityInit:
		return receiveSecurityTypes();

	case SecurityChallenge:
		return receiveSecurityChallenge();

	case SecurityResult:
		return receiveSecurityResult();

	case FramebufferInit:
		return receiveServerInitMessage();

	default:
		break;
	}

	return false;
}



bool VncClientProtocol::setPixelFormat( rfbPixelFormat pixelFormat )
{
	rfbSetPixelFormatMsg spf;

	spf.type = rfbSetPixelFormat;
	spf.pad1 = 0;
	spf.pad2 = 0;
	spf.format = pixelFormat;
	spf.format.redMax = qFromBigEndian(pixelFormat.redMax);
	spf.format.greenMax = qFromBigEndian(pixelFormat.greenMax);
	spf.format.blueMax = qFromBigEndian(pixelFormat.blueMax);

	return m_socket->write( (const char *) &spf, sz_rfbSetPixelFormatMsg ) == sz_rfbSetPixelFormatMsg;
}



bool VncClientProtocol::setEncodings( const QVector<uint32_t>& encodings )
{
	if( encodings.size() > MAX_ENCODINGS )
	{
		return false;
	}

	char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];

	rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
	uint32_t *encs = (uint32_t *)(&buf[sz_rfbSetEncodingsMsg]);
	int len = 0;

	se->type = rfbSetEncodings;
	se->pad = 0;
	se->nEncodings = 0;

	for( auto encoding : encodings )
	{
		encs[se->nEncodings++] = qFromBigEndian<uint32_t>( encoding );
	}

	len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;

	se->nEncodings = qFromBigEndian(se->nEncodings);

	return m_socket->write( buf, len ) == len;
}



void VncClientProtocol::requestFramebufferUpdate( bool incremental )
{
	rfbFramebufferUpdateRequestMsg updateRequest;

	updateRequest.type = rfbFramebufferUpdateRequest;
	updateRequest.incremental = incremental ? 1 : 0;
	updateRequest.x = 0;
	updateRequest.y = 0;
	updateRequest.w = qFromBigEndian<uint16_t>( m_framebufferWidth );
	updateRequest.h = qFromBigEndian<uint16_t>( m_framebufferHeight );

	if( m_socket->write( (const char *) &updateRequest, sz_rfbFramebufferUpdateRequestMsg ) != sz_rfbFramebufferUpdateRequestMsg )
	{
		qDebug( "VncClientProtocol::requestFramebufferUpdate(): could not write to socket - closing connection" );
		m_socket->close();
	}
}



bool VncClientProtocol::receiveMessage()
{
	uint8_t messageType = 0;
	if( m_socket->peek( (char *) &messageType, sizeof(messageType) ) != sizeof(messageType) )
	{
		return false;
	}

	switch( messageType )
	{
	case rfbFramebufferUpdate:
		return receiveFramebufferUpdateMessage();

	case rfbSetColourMapEntries:
		return receiveColourMapEntriesMessage();

	case rfbBell:
		return receiveBellMessage();

	case rfbServerCutText:
		return receiveCutTextMessage();

	case rfbResizeFrameBuffer:
		return receiveResizeFramebufferMessage();

	case rfbXvp:
		return receiveXvpMessage();

	default:
		qCritical( "VncClientProtocol::receiveMessage(): received unknown message type: %d", (int) messageType );
		m_socket->close();
		return false;
	}

	return false;
}



bool VncClientProtocol::readProtocol()
{
	if( m_socket->bytesAvailable() == sz_rfbProtocolVersionMsg )
	{
		char protocol[sz_rfbProtocolVersionMsg+1];
		m_socket->read( protocol, sz_rfbProtocolVersionMsg );
		protocol[sz_rfbProtocolVersionMsg] = 0;

		int protocolMajor = 0, protocolMinor = 0;

		if( sscanf( protocol, rfbProtocolVersionFormat, &protocolMajor, &protocolMinor ) != 2 ||
				protocolMajor != 3 || protocolMinor < 7 )
		{
			qCritical( "VncClientProtocol:::readProtocol(): protocol initialization failed" );
			m_socket->close();

			return false;
		}

		m_socket->write( protocol, sz_rfbProtocolVersionMsg );

		m_state = SecurityInit;

		return true;
	}

	return false;
}



bool VncClientProtocol::receiveSecurityTypes()
{
	if( m_socket->bytesAvailable() >= 2 )
	{
		uint8_t securityTypeCount = 0;

		m_socket->read( (char *) &securityTypeCount, sizeof(securityTypeCount) );

		if( securityTypeCount == 0 )
		{
			qCritical( "VncClientProtocol::receiveSecurityTypes(): invalid number of security types received!" );
			m_socket->close();

			return false;
		}

		QByteArray securityTypeList = m_socket->read( securityTypeCount );
		if( securityTypeList.count() != securityTypeCount )
		{
			qCritical( "VncClientProtocol::receiveSecurityTypes(): could not read security types!" );
			m_socket->close();

			return false;
		}

		char securityType = rfbSecTypeVncAuth;

		if( securityTypeList.contains( securityType ) == false )
		{
			qCritical( "VncClientProtocol::receiveSecurityTypes(): no supported security type!" );
			m_socket->close();

			return false;
		}

		m_socket->write( &securityType, sizeof(securityType) );

		m_state = SecurityChallenge;

		return true;
	}

	return false;
}



bool VncClientProtocol::receiveSecurityChallenge()
{
	if( m_socket->bytesAvailable() >= CHALLENGESIZE )
	{
		uint8_t challenge[CHALLENGESIZE];
		m_socket->read( (char *) challenge, CHALLENGESIZE );

		vncEncryptBytes( challenge, m_vncPassword.constData() );

		m_socket->write( (const char *) challenge, CHALLENGESIZE );

		m_state = SecurityResult;

		return true;
	}

	return false;
}



bool VncClientProtocol::receiveSecurityResult()
{
	if( m_socket->bytesAvailable() >= 4 )
	{
		uint32_t authResult = 0;

		m_socket->read( (char *) &authResult, sizeof(authResult) );

		if( qFromBigEndian( authResult ) != rfbVncAuthOK )
		{
			qCritical( "VncClientProtocol::receiveSecurityResult(): authentication failed!" );
			m_socket->close();
			return false;
		}

		qDebug( "VncClientProtocol::receiveSecurityResult(): authentication successful" );

		// finally send client init message
		rfbClientInitMsg clientInitMessage;
		clientInitMessage.shared = 1;
		m_socket->write( (const char *) &clientInitMessage, sz_rfbClientInitMsg );

		// wait for server init message
		m_state = FramebufferInit;

		return true;
	}

	return false;
}



bool VncClientProtocol::receiveServerInitMessage()
{
	rfbServerInitMsg message;

	if( m_socket->bytesAvailable() >= sz_rfbServerInitMsg &&
			m_socket->peek( (char *) &message, sz_rfbServerInitMsg ) == sz_rfbServerInitMsg )
	{
		auto nameLength = qFromBigEndian( message.nameLength );

		if( nameLength > 255 )
		{
			qCritical() << Q_FUNC_INFO << "size of desktop name > 255!";
			m_socket->close();
			return false;
		}

		memcpy( &m_pixelFormat, &message.format, sz_rfbPixelFormat );

		if( static_cast<uint32_t>( m_socket->peek( nameLength ).size() ) == nameLength )
		{
			m_serverInitMessage = m_socket->read( sz_rfbServerInitMsg + nameLength );

			const auto serverInitMessage = (const rfbServerInitMsg *) m_serverInitMessage.constData();
			m_framebufferWidth = qFromBigEndian( serverInitMessage->framebufferWidth );
			m_framebufferHeight = qFromBigEndian( serverInitMessage->framebufferHeight );

			m_state = Running;

			return true;
		}
	}

	return false;
}



bool VncClientProtocol::receiveFramebufferUpdateMessage()
{
	// peek all available data and work on a local buffer so we can continously read from it
	QByteArray data = m_socket->peek( m_socket->bytesAvailable() );

	QBuffer buffer( &data );
	buffer.open( QBuffer::ReadOnly );

	rfbFramebufferUpdateMsg message;
	if( buffer.read( (char *) &message, sz_rfbFramebufferUpdateMsg ) != sz_rfbFramebufferUpdateMsg )
	{
		return false;
	}

	QRegion updatedRegion;

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

		if( isPseudoEncoding( rectHeader ) == false &&
			rectHeader.r.x+rectHeader.r.w <= m_framebufferWidth &&
			rectHeader.r.y+rectHeader.r.h <= m_framebufferHeight )
		{
			updatedRegion += QRect( rectHeader.r.x, rectHeader.r.y, rectHeader.r.w, rectHeader.r.h );
		}
	}

	m_lastUpdatedRect = updatedRegion.boundingRect();

	// save as much data as we read by processing rects
	return readMessage( buffer.pos() );
}



bool VncClientProtocol::receiveColourMapEntriesMessage()
{
	rfbSetColourMapEntriesMsg message;
	if( m_socket->peek( (char *) &message, sz_rfbSetColourMapEntriesMsg ) != sz_rfbSetColourMapEntriesMsg )
	{
		return false;
	}

	return readMessage( sz_rfbSetColourMapEntriesMsg + qFromBigEndian( message.nColours ) * 6 );
}




bool VncClientProtocol::receiveBellMessage()
{
	return readMessage( sz_rfbBellMsg );
}



bool VncClientProtocol::receiveCutTextMessage()
{
	rfbServerCutTextMsg message;
	if( m_socket->peek( (char *) &message, sz_rfbServerCutTextMsg ) != sz_rfbServerCutTextMsg )
	{
		return false;
	}

	return readMessage( sz_rfbServerCutTextMsg + qFromBigEndian( message.length ) );
}



bool VncClientProtocol::receiveResizeFramebufferMessage()
{
	if( readMessage( sz_rfbResizeFrameBufferMsg ) )
	{
		const auto msg = (rfbResizeFrameBufferMsg *) m_lastMessage.constData();
		m_framebufferWidth = qFromBigEndian( msg->framebufferWidth );
		m_framebufferHeight = qFromBigEndian( msg->framebufferHeigth );

		return true;
	}

	return false;
}



bool VncClientProtocol::receiveXvpMessage()
{
	return readMessage( sz_rfbXvpMsg );
}



bool VncClientProtocol::readMessage( qint64 size )
{
	if( m_socket->bytesAvailable() < size )
	{
		return false;
	}

	auto message = m_socket->read( size );
	if( message.size() == size )
	{
		m_lastMessage = message;
		return true;
	}

	qWarning( "VncClientProtocol::readMessage(): only received %d of %d bytes", (int) message.size(), (int) size );

	return false;
}



bool VncClientProtocol::handleRect( QBuffer& buffer, const rfbFramebufferUpdateRectHeader& rectHeader )
{
	const int width = rectHeader.r.w;
	const int height = rectHeader.r.h;

	const int bytesPerPixel = m_pixelFormat.bitsPerPixel / 8;
	const int bytesPerRow = ( width + 7 ) / 8;

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
				( buffer.read( width * height * bytesPerPixel ).size() == width * height * bytesPerPixel &&
				  buffer.read( bytesPerRow * height ).size() == bytesPerRow * height );

	case rfbEncodingSupportedMessages:
		return buffer.read( sz_rfbSupportedMessages ).size() == sz_rfbSupportedMessages;

	case rfbEncodingSupportedEncodings:
	case rfbEncodingServerIdentity:
		// width = byte count
		return buffer.read( width ).size() == width;

	case rfbEncodingRaw:
		return buffer.read( width * height * bytesPerPixel ).size() == width * height * bytesPerPixel;

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
		m_socket->close();
		break;
	}

	return false;
}



bool VncClientProtocol::handleRectEncodingRRE( QBuffer& buffer, int bytesPerPixel )
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



bool VncClientProtocol::handleRectEncodingCoRRE( QBuffer& buffer, int bytesPerPixel )
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



bool VncClientProtocol::handleRectEncodingHextile( QBuffer& buffer,
													const rfbFramebufferUpdateRectHeader rectHeader,
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

			int64_t subRectDataSize = 0;

			if( subEncoding & rfbHextileSubrectsColoured )
			{
				subRectDataSize = nSubrects * ( 2 + bytesPerPixel );
			}
			else
			{
				subRectDataSize = nSubrects * 2;
			}

			if( buffer.read( subRectDataSize ).size() != subRectDataSize )
			{
				return false;
			}
		}
	}

	return true;
}



bool VncClientProtocol::handleRectEncodingZlib( QBuffer& buffer )
{
	rfbZlibHeader hdr;

	if( buffer.read( (char *) &hdr, sz_rfbZlibHeader ) != sz_rfbZlibHeader )
	{
		return false;
	}

	const auto n = qFromBigEndian( hdr.nBytes );

	return buffer.read( n ).size() == static_cast<int64_t>( n );
}



bool VncClientProtocol::handleRectEncodingZRLE(QBuffer &buffer)
{
	rfbZRLEHeader hdr;

	if( buffer.read( (char *) &hdr, sz_rfbZRLEHeader ) != sz_rfbZRLEHeader )
	{
		return false;
	}

	const auto n = qFromBigEndian( hdr.length );

	return buffer.read( n ).size() == static_cast<int64_t>( n );
}



bool VncClientProtocol::isPseudoEncoding( const rfbFramebufferUpdateRectHeader& header )
{
	switch( header.encoding )
	{
	case rfbEncodingSupportedEncodings:
	case rfbEncodingSupportedMessages:
	case rfbEncodingServerIdentity:
	case rfbEncodingPointerPos:
	case rfbEncodingKeyboardLedState:
	case rfbEncodingNewFBSize:
		return true;
	default:
		break;
	}

	return false;
}
