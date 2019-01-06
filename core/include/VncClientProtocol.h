/*
 * VncClientProtocol.h - header file for the VncClientProtocol class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QRect>

#include "rfb/rfbproto.h"

#include "VeyonCore.h"

class QBuffer;
class QTcpSocket;

class VEYON_CORE_EXPORT VncClientProtocol
{
public:
	typedef enum States {
		Disconnected,
		Protocol,
		SecurityInit,
		SecurityChallenge,
		SecurityResult,
		FramebufferInit,
		Running,
		StateCount
	} State;

	VncClientProtocol( QTcpSocket* socket, const QString& vncPassword );

	State state() const
	{
		return m_state;
	}

	void start();
	bool read();  // Flawfinder: ignore

	const QByteArray& serverInitMessage() const
	{
		return m_serverInitMessage;
	}

	int framebufferWidth() const
	{
		return m_framebufferWidth;
	}

	int framebufferHeight() const
	{
		return m_framebufferHeight;
	}

	bool setPixelFormat( rfbPixelFormat pixelFormat );
	bool setEncodings( const QVector<uint32_t>& encodings );

	void requestFramebufferUpdate( bool incremental );

	bool receiveMessage();

	const QByteArray& lastMessage() const
	{
		return m_lastMessage;
	}

	uint8_t lastMessageType() const
	{
		return static_cast<uint8_t>( m_lastMessage.constData()[0] );
	}

	const QRect& lastUpdatedRect() const
	{
		return m_lastUpdatedRect;
	}

private:
	bool readProtocol();
	bool receiveSecurityTypes();
	bool receiveSecurityChallenge();
	bool receiveSecurityResult();
	bool receiveServerInitMessage();

	bool receiveFramebufferUpdateMessage();
	bool receiveColourMapEntriesMessage();
	bool receiveBellMessage();
	bool receiveCutTextMessage();
	bool receiveResizeFramebufferMessage();
	bool receiveXvpMessage();

	bool readMessage( int size );

	bool handleRect( QBuffer& buffer, rfbFramebufferUpdateRectHeader rectHeader );
	bool handleRectEncodingRRE( QBuffer& buffer, uint bytesPerPixel );
	bool handleRectEncodingCoRRE( QBuffer& buffer, uint bytesPerPixel );
	bool handleRectEncodingHextile( QBuffer& buffer,
									const rfbFramebufferUpdateRectHeader rectHeader,
									uint bytesPerPixel );
	bool handleRectEncodingZlib( QBuffer& buffer );
	bool handleRectEncodingZRLE( QBuffer& buffer );

	static bool isPseudoEncoding( rfbFramebufferUpdateRectHeader header );

	QTcpSocket* m_socket;
	State m_state;

	QByteArray m_vncPassword;

	QByteArray m_serverInitMessage;

	rfbPixelFormat m_pixelFormat;

	quint16 m_framebufferWidth;
	quint16 m_framebufferHeight;

	QByteArray m_lastMessage;
	QRect m_lastUpdatedRect;

} ;
