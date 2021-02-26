/*
 * VncServerProtocol.h - header file for the VncServerProtocol class
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

#pragma once

#include "RfbVeyonAuth.h"

class QTcpSocket;

class VariantArrayMessage;
class VncServerClient;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT VncServerProtocol
{
public:
	enum State {
		Disconnected,
		Protocol,
		SecurityInit,
		AuthenticationTypes,
		Authenticating,
		AccessControl,
		FramebufferInit,
		Running,
		Close,
		StateCount
	} ;

	VncServerProtocol( QTcpSocket* socket,
					   VncServerClient* client );
	virtual ~VncServerProtocol() = default;

	State state() const;

	void start();
	bool read(); // Flawfinder: ignore

	void setServerInitMessage( const QByteArray& serverInitMessage )
	{
		m_serverInitMessage = serverInitMessage;
	}

protected:
	virtual QVector<RfbVeyonAuth::Type> supportedAuthTypes() const = 0;
	virtual void processAuthenticationMessage( VariantArrayMessage& message ) = 0;
	virtual void performAccessControl() = 0;

	QTcpSocket* socket()
	{
		return m_socket;
	}

	VncServerClient* client()
	{
		return m_client;
	}

private:
	void setState( State state );

	bool readProtocol();
	bool sendSecurityTypes();
	bool receiveSecurityTypeResponse();
	bool sendAuthenticationTypes();
	bool receiveAuthenticationTypeResponse();
	bool receiveAuthenticationMessage();

	bool processAuthentication( VariantArrayMessage& message );
	bool processAccessControl();

	bool processFramebufferInit();

private:
	QTcpSocket* m_socket;
	VncServerClient* m_client;

	QByteArray m_serverInitMessage;

} ;
