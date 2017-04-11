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

#include <QString>

class QTcpSocket;

class VncClientProtocol
{
public:
	typedef enum States {
		Disconnected,
		Protocol,
		SecurityInit,
		SecurityChallenge,
		SecurityResult,
		Authenticated,
		Connected,
		StateCount
	} State;

	VncClientProtocol( QTcpSocket* socket, const QString& vncPassword );

	State state() const
	{
		return m_state;
	}

	void start();
	bool read();

private:
	enum {
		MaxSecurityTypes = 254
	};

	bool readProtocol();
	bool receiveSecurityTypes();
	bool receiveSecurityChallenge();
	bool receiveSecurityResult();

	QTcpSocket* m_socket;
	State m_state;

	QByteArray m_vncPassword;

} ;

#endif
