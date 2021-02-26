/*
 * VncClientProtocol.cpp - implementation of the VncClientProtocol class
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

extern "C"
{
#include "rfb/rfbproto.h"
#include "d3des.h"
}

#include <QBuffer>
#include <QRegion>
#include <QTcpSocket>

#include "VncClientProtocol.h"


/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */

static void
vncEncryptBytes(unsigned char *bytes, const char *passwd, size_t passwd_length)
{
	constexpr int KeyLength = 8;
	unsigned char key[KeyLength];  // Flawfinder: ignore
	unsigned int i;

	/* key is simply password padded with nulls */

	for (i = 0; i < KeyLength; i++) {
		if (i < passwd_length) {
			key[i] = static_cast<unsigned char>( passwd[i] );
		} else {
			key[i] = 0;
		}
	}

	rfbDesKey(key, EN0);

	for (i = 0; i < CHALLENGESIZE; i += KeyLength) {
		rfbDes(bytes+i, bytes+i);
	}
}




VncClientProtocol::VncClientProtocol( QTcpSocket* socket, const Password& vncPassword ) :
	m_socket( socket ),
	m_state( Disconnected ),
	m_vncPassword( vncPassword ),
	m_serverInitMessage(),
	m_pixelFormat( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ),
	m_framebufferWidth( 0 ),
	m_framebufferHeight( 0 )
{
}



void VncClientProtocol::start()
{
	m_state = Protocol;
}



bool VncClientProtocol::read() // Flawfinder: ignore
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

	return m_socket->write( reinterpret_cast<const char *>( &spf ), sz_rfbSetPixelFormatMsg ) == sz_rfbSetPixelFormatMsg;
}



bool VncClientProtocol::setEncodings( const QVector<uint32_t>& encodings )
{
	if( encodings.size() > MAX_ENCODINGS )
	{
		return false;
	}

	alignas(rfbSetEncodingsMsg) char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4]; // Flawfinder: ignore

	auto setEncodingsMsg = reinterpret_cast<rfbSetEncodingsMsg *>( buf );
	auto encs = reinterpret_cast<uint32_t *>( &buf[sz_rfbSetEncodingsMsg] );

	setEncodingsMsg->type = rfbSetEncodings;
	setEncodingsMsg->pad = 0;
	setEncodingsMsg->nEncodings = 0;

	for( auto encoding : encodings )
	{
		encs[setEncodingsMsg->nEncodings++] = qFromBigEndian<uint32_t>( encoding );
	}

	const auto len = sz_rfbSetEncodingsMsg + setEncodingsMsg->nEncodings * 4;

	setEncodingsMsg->nEncodings = qFromBigEndian(setEncodingsMsg->nEncodings);

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

	if( m_socket->write( reinterpret_cast<const char *>( &updateRequest ), sz_rfbFramebufferUpdateRequestMsg ) != sz_rfbFramebufferUpdateRequestMsg )
	{
		vDebug() << "could not write to socket - closing connection";
		m_socket->close();
	}
}



bool VncClientProtocol::receiveMessage()
{
	if( m_socket->bytesAvailable() > MaximumMessageSize )
	{
		vCritical() << "Message too big or invalid";
		m_socket->close();
		return false;
	}

	uint8_t messageType = 0;
	if( m_socket->peek( reinterpret_cast<char *>( &messageType ), sizeof(messageType) ) != sizeof(messageType) )
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
		vCritical() << "received unknown message type" << static_cast<int>( messageType );
		m_socket->close();
	}

	return false;
}



bool VncClientProtocol::readProtocol()
{
	if( m_socket->bytesAvailable() == sz_rfbProtocolVersionMsg )
	{
		const auto protocol = m_socket->read( sz_rfbProtocolVersionMsg );

		if( protocol.size() != sz_rfbProtocolVersionMsg )
		{
			vCritical() << "protocol initialization failed";
			m_socket->close();

			return false;
		}

		QRegExp protocolRX( QStringLiteral("RFB (\\d\\d\\d)\\.(\\d\\d\\d)\n") );

		if( protocolRX.indexIn( QString::fromUtf8( protocol ) ) != 0 ||
			protocolRX.cap( 1 ).toInt() != 3 ||
			protocolRX.cap( 2 ).toInt() < 7 )
		{
			vCritical() << "invalid protocol version";
			m_socket->close();

			return false;
		}

		m_socket->write( protocol );

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

		m_socket->read( reinterpret_cast<char *>( &securityTypeCount ), sizeof(securityTypeCount) );

		if( securityTypeCount == 0 )
		{
			vCritical() << "invalid number of security types received!";
			m_socket->close();

			return false;
		}

		const auto securityTypeList = m_socket->read( securityTypeCount );
		if( securityTypeList.count() != securityTypeCount )
		{
			vCritical() << "could not read security types!";
			m_socket->close();

			return false;
		}

		char securityType = rfbSecTypeInvalid;

		if( securityTypeList.contains( rfbSecTypeVncAuth ) )
		{
			securityType = rfbSecTypeVncAuth;
			m_state = State::SecurityChallenge;
		}
		else if( securityTypeList.contains( rfbSecTypeNone ) )
		{
			securityType = rfbSecTypeNone;
			m_state = State::SecurityResult;
		}
		else
		{
			vCritical() << "unsupported security types!" << securityTypeList;
			m_socket->close();
			return false;
		}

		m_socket->write( &securityType, sizeof(securityType) );

		return true;
	}

	return false;
}



bool VncClientProtocol::receiveSecurityChallenge()
{
	if( m_socket->bytesAvailable() >= CHALLENGESIZE )
	{
		uint8_t challenge[CHALLENGESIZE];
		m_socket->read( reinterpret_cast<char *>( challenge ), CHALLENGESIZE );

		vncEncryptBytes( challenge, m_vncPassword.constData(), static_cast<size_t>( m_vncPassword.size() ) );

		m_socket->write( reinterpret_cast<const char *>( challenge ), CHALLENGESIZE );

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

		m_socket->read( reinterpret_cast<char *>( &authResult ), sizeof(authResult) );

		if( qFromBigEndian( authResult ) != rfbVncAuthOK )
		{
			vCritical() << "authentication failed!";
			m_socket->close();
			return false;
		}

		vDebug() << "authentication successful";

		// finally send client init message
		rfbClientInitMsg clientInitMessage;
		clientInitMessage.shared = 1;
		m_socket->write( reinterpret_cast<const char *>( &clientInitMessage ), sz_rfbClientInitMsg );

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
			m_socket->peek( reinterpret_cast<char *>( &message ), sz_rfbServerInitMsg ) == sz_rfbServerInitMsg )
	{
		const auto nameLength = qFromBigEndian( message.nameLength );

		if( nameLength > 255 )
		{
			vCritical() << "size of desktop name > 255!";
			m_socket->close();
			return false;
		}

		static_assert( sizeof(m_pixelFormat) >= sz_rfbPixelFormat, "m_pixelFormat has wrong size" );
		static_assert( sizeof(m_pixelFormat) >= sizeof(message.format), "m_pixelFormat too small" );

		memcpy( &m_pixelFormat, &message.format, sz_rfbPixelFormat ); // Flawfinder: ignore

		if( static_cast<uint32_t>( m_socket->peek( nameLength ).size() ) == nameLength )
		{
			m_serverInitMessage = m_socket->read( sz_rfbServerInitMsg + nameLength );

			const auto serverInitMessage = reinterpret_cast<const rfbServerInitMsg *>( m_serverInitMessage.constData() );
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
	auto data = m_socket->peek( m_socket->bytesAvailable() );

	QBuffer buffer( &data );
	buffer.open( QBuffer::ReadOnly ); // Flawfinder: ignore

	rfbFramebufferUpdateMsg message;
	if( buffer.read( reinterpret_cast<char *>( &message ), sz_rfbFramebufferUpdateMsg ) != sz_rfbFramebufferUpdateMsg )
	{
		return false;
	}

	QRegion updatedRegion;

	const auto nRects = qFromBigEndian( message.nRects );

	for( int i = 0; i < nRects; ++i )
	{
		rfbFramebufferUpdateRectHeader rectHeader;
		if( buffer.read( reinterpret_cast<char *>( &rectHeader ), sz_rfbFramebufferUpdateRectHeader ) != sz_rfbFramebufferUpdateRectHeader )
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
	return readMessage( static_cast<int>( buffer.pos() ) );
}



bool VncClientProtocol::receiveColourMapEntriesMessage()
{
	rfbSetColourMapEntriesMsg message;
	if( m_socket->peek( reinterpret_cast<char *>( &message ), sz_rfbSetColourMapEntriesMsg ) != sz_rfbSetColourMapEntriesMsg )
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
	if( m_socket->peek( reinterpret_cast<char *>( &message ), sz_rfbServerCutTextMsg ) != sz_rfbServerCutTextMsg )
	{
		return false;
	}

	return readMessage( sz_rfbServerCutTextMsg + static_cast<int>( qFromBigEndian( message.length ) ) );
}



bool VncClientProtocol::receiveResizeFramebufferMessage()
{
	if( readMessage( sz_rfbResizeFrameBufferMsg ) )
	{
		const auto msg = reinterpret_cast<const rfbResizeFrameBufferMsg *>( m_lastMessage.constData() );
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



bool VncClientProtocol::readMessage( int size )
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

	vWarning() << "only received" << message.size() << "of" << size << "bytes";

	return false;
}



bool VncClientProtocol::handleRect( QBuffer& buffer, rfbFramebufferUpdateRectHeader rectHeader )
{
	const uint width = rectHeader.r.w;
	const uint height = rectHeader.r.h;

	const uint bytesPerPixel = m_pixelFormat.bitsPerPixel / 8;
	const uint bytesPerRow = ( width + 7 ) / 8;

	switch( rectHeader.encoding )
	{
	case rfbEncodingLastRect:
		return true;

	case rfbEncodingXCursor:
		return width * height == 0 ||
				( buffer.read( sz_rfbXCursorColors ).size() == sz_rfbXCursorColors &&
				  buffer.read( 2 * bytesPerRow * height ).size() == static_cast<int>( 2 * bytesPerRow * height ) );

	case rfbEncodingRichCursor:
		return width * height == 0 ||
				( buffer.read( width * height * bytesPerPixel ).size() == static_cast<int>( width * height * bytesPerPixel ) &&
				  buffer.read( bytesPerRow * height ).size() == static_cast<int>( bytesPerRow * height ) );

	case rfbEncodingSupportedMessages:
		return buffer.read( sz_rfbSupportedMessages ).size() == sz_rfbSupportedMessages;

	case rfbEncodingSupportedEncodings:
	case rfbEncodingServerIdentity:
		// width = byte count
		return buffer.read( width ).size() == static_cast<int>( width );

	case rfbEncodingRaw:
		return buffer.read( width * height * bytesPerPixel ).size() == static_cast<int>( width * height * bytesPerPixel );

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
		vCritical() << "Unsupported rect encoding" << rectHeader.encoding;
		m_socket->close();
		break;
	}

	return false;
}



bool VncClientProtocol::handleRectEncodingRRE( QBuffer& buffer, uint bytesPerPixel )
{
	rfbRREHeader hdr;

	if( buffer.read( reinterpret_cast<char *>( &hdr ), sz_rfbRREHeader ) != sz_rfbRREHeader )
	{
		return false;
	}

	const auto rectDataSize = qFromBigEndian( hdr.nSubrects ) * ( bytesPerPixel + sz_rfbRectangle );
	const auto totalDataSize = static_cast<int>( bytesPerPixel + rectDataSize );

	return buffer.read( totalDataSize ).size() == totalDataSize;
}



bool VncClientProtocol::handleRectEncodingCoRRE( QBuffer& buffer, uint bytesPerPixel )
{
	rfbRREHeader hdr;

	if( buffer.read( reinterpret_cast<char *>( &hdr ), sz_rfbRREHeader ) != sz_rfbRREHeader )
	{
		return false;
	}

	const auto rectDataSize = qFromBigEndian( hdr.nSubrects ) * ( bytesPerPixel + 4 );
	const auto totalDataSize = static_cast<int>( bytesPerPixel + rectDataSize );

	return buffer.read( totalDataSize ).size() == totalDataSize;

}



bool VncClientProtocol::handleRectEncodingHextile( QBuffer& buffer,
													const rfbFramebufferUpdateRectHeader rectHeader,
													uint bytesPerPixel )
{
	const uint rx = rectHeader.r.x;
	const uint ry = rectHeader.r.y;
	const uint rw = rectHeader.r.w;
	const uint rh = rectHeader.r.h;

	for( uint y = ry; y < ry+rh; y += 16 )
	{
		for( uint x = rx; x < rx+rw; x += 16 )
		{
			uint w = 16, h = 16;
			if( rx+rw - x < 16 )
			{
				w = rx+rw - x;
			}
			if( ry+rh - y < 16 )
			{
				h = ry+rh - y;
			}

			uint8_t subEncoding = 0;
			if( buffer.read( reinterpret_cast<char *>( &subEncoding ), 1 ) != 1 )
			{
				return false;
			}

			if( subEncoding & rfbHextileRaw )
			{
				const auto dataSize = static_cast<int>( w * h * bytesPerPixel );
				if( buffer.read( dataSize ).size() != dataSize )
				{
					return false;
				}
				continue;
			}

			if( subEncoding & rfbHextileBackgroundSpecified )
			{
				if( buffer.read( bytesPerPixel ).size() != static_cast<int>( bytesPerPixel ) )
				{
					return false;
				}
			}

			if( subEncoding & rfbHextileForegroundSpecified )
			{
				if( buffer.read( bytesPerPixel ).size() != static_cast<int>( bytesPerPixel ) )
				{
					return false;
				}
			}

			if( !( subEncoding & rfbHextileAnySubrects ) )
			{
				continue;
			}

			uint8_t nSubrects = 0;
			if( buffer.read( reinterpret_cast<char *>( &nSubrects ), 1 ) != 1 )
			{
				return false;
			}

			int subRectDataSize = 0;

			if( subEncoding & rfbHextileSubrectsColoured )
			{
				subRectDataSize = static_cast<int>( nSubrects * ( 2 + bytesPerPixel ) );
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

	if( buffer.read( reinterpret_cast<char *>( &hdr ), sz_rfbZlibHeader ) != sz_rfbZlibHeader )
	{
		return false;
	}

	const auto n = qFromBigEndian( hdr.nBytes );

	return buffer.read( n ).size() == static_cast<int>( n );
}



bool VncClientProtocol::handleRectEncodingZRLE(QBuffer &buffer)
{
	rfbZRLEHeader hdr;

	if( buffer.read( reinterpret_cast<char *>( &hdr ), sz_rfbZRLEHeader ) != sz_rfbZRLEHeader )
	{
		return false;
	}

	const auto n = qFromBigEndian( hdr.length );

	return buffer.read( n ).size() == static_cast<int>( n );
}



bool VncClientProtocol::isPseudoEncoding( rfbFramebufferUpdateRectHeader header )
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
