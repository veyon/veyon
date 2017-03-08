/*
 * VncServerClient.h - header file for the VncServerClient class
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

#ifndef VNC_SERVER_CLIENT_H
#define VNC_SERVER_CLIENT_H

#include <QByteArray>

#include "RfbItalcAuth.h"

class VncServerClient
{
public:
	typedef enum ProtocolStates {
		Disconnected,
		Protocol,
		SecurityInit,
		AuthenticationTypes,
		Authenticating,
		AccessControl,
		Initialized,
		Close,
		StateCount
	} ProtocolState;

	typedef enum AuthStates {
		AuthInit,
		AuthChallenge,
		AuthPassword,
		AuthFinishedSuccess,
		AuthFinishedFail,
	} AuthState;

	VncServerClient() :
		m_protocolState( Disconnected ),
		m_authState( AuthInit ),
		m_authType( RfbItalcAuth::Invalid ),
		m_username(),
		m_hostAddress(),
		m_challenge()
	{
	}

	ProtocolState protocolState() const
	{
		return m_protocolState;
	}

	void setProtocolState( ProtocolState protocolState )
	{
		m_protocolState = protocolState;
	}

	AuthState authState() const
	{
		return m_authState;
	}

	void setAuthState( AuthState authState )
	{
		m_authState = authState;
	}

	RfbItalcAuth::Type authType() const
	{
		return m_authType;
	}

	void setAuthType( RfbItalcAuth::Type authType )
	{
		m_authType = authType;
	}

	const QString& username() const
	{
		return m_username;
	}

	void setUsername( const QString& username )
	{
		m_username = username;
	}

	const QString& hostAddress() const
	{
		return m_hostAddress;
	}

	void setHostAddress( const QString& hostAddress )
	{
		m_hostAddress = hostAddress;
	}

	const QByteArray& challenge() const
	{
		return m_challenge;
	}

	void setChallenge( const QByteArray& challenge )
	{
		m_challenge = challenge;
	}

	const QString& privateKey() const
	{
		return m_privateKey;
	}

	void setPrivateKey( const QString& privateKey )
	{
		m_privateKey = privateKey;
	}

private:
	ProtocolState m_protocolState;
	AuthState m_authState;
	RfbItalcAuth::Type m_authType;
	QString m_username;
	QString m_hostAddress;
	QByteArray m_challenge;
	QString m_privateKey;

} ;

typedef QList<VncServerClient *> VncServerClientList;

#endif
