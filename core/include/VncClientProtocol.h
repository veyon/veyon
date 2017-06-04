/*
 * VncClientProtocol.h - header file for the VncClientProtocol class
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

#ifndef VNC_CLIENT_PROTOCOL_H
#define VNC_CLIENT_PROTOCOL_H

#include <QRect>

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
	bool read();

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

	void requestFramebufferUpdate( bool incremental );

	bool receiveMessage();

	const QByteArray& lastMessage() const
	{
		return m_lastMessage;
	}

	uint8_t lastMessageType() const
	{
		return m_lastMessage.constData()[0];
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

	bool readMessage( qint64 size );

	bool handleRect( QBuffer& buffer, rfbFramebufferUpdateRectHeader& rectHeader );
	bool handleRectEncodingRRE( QBuffer& buffer, int bytesPerPixel );
	bool handleRectEncodingCoRRE( QBuffer& buffer, int bytesPerPixel );
	bool handleRectEncodingHextile( QBuffer& buffer,
									const rfbFramebufferUpdateRectHeader& rectHeader,
									int bytesPerPixel );
	bool handleRectEncodingZlib( QBuffer& buffer );
	bool handleRectEncodingZRLE( QBuffer& buffer );

	QTcpSocket* m_socket;
	State m_state;

	QByteArray m_vncPassword;

	QByteArray m_serverInitMessage;

	rfbPixelFormat m_pixelFormat;

	uint16_t m_framebufferWidth;
	uint16_t m_framebufferHeight;

	QByteArray m_lastMessage;
	QRect m_lastUpdatedRect;

} ;

#endif
