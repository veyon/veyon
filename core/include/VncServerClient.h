/*
 * VncServerClient.h - header file for the VncServerClient class
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

#include <QElapsedTimer>

#include "CryptoCore.h"
#include "VncServerProtocol.h"

class VEYON_CORE_EXPORT VncServerClient : public QObject
{
	Q_OBJECT
public:
	enum class AuthState {
		Init,
		Challenge,
		Password,
		Token,
		Successful,
		Failed,
	} ;
	Q_ENUM(AuthState)

	enum class AccessControlState {
		Init,
		Successful,
		Pending,
		Waiting,
		Failed
	} ;
	Q_ENUM(AccessControlState)

	explicit VncServerClient( QObject* parent = nullptr ) :
		QObject( parent ),
		m_protocolState( VncServerProtocol::Disconnected ),
		m_authState( AuthState::Init ),
		m_authType( RfbVeyonAuth::Invalid ),
		m_accessControlState( AccessControlState::Init ),
		m_username(),
		m_hostAddress(),
		m_challenge()
	{
	}

	VncServerProtocol::State protocolState() const
	{
		return m_protocolState;
	}

	void setProtocolState( VncServerProtocol::State protocolState )
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

	RfbVeyonAuth::Type authType() const
	{
		return m_authType;
	}

	void setAuthType( RfbVeyonAuth::Type authType )
	{
		m_authType = authType;
	}

	AccessControlState accessControlState() const
	{
		return m_accessControlState;
	}

	void setAccessControlState( AccessControlState accessControlState )
	{
		m_accessControlState = accessControlState;
	}

	QElapsedTimer& accessControlTimer()
	{
		return m_accessControlTimer;
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

	const CryptoCore::PrivateKey& privateKey() const
	{
		return m_privateKey;
	}

	void setPrivateKey( const CryptoCore::PrivateKey& privateKey )
	{
		m_privateKey = privateKey;
	}

public Q_SLOTS:
	void finishAccessControl()
	{
		Q_EMIT accessControlFinished( this );
	}

Q_SIGNALS:
	void accessControlFinished( VncServerClient* );

private:
	VncServerProtocol::State m_protocolState;
	AuthState m_authState;
	RfbVeyonAuth::Type m_authType;
	AccessControlState m_accessControlState;
	QElapsedTimer m_accessControlTimer;
	QString m_username;
	QString m_hostAddress;
	QByteArray m_challenge;
	CryptoCore::PrivateKey m_privateKey;

} ;

using VncServerClientList = QList<VncServerClient *>;
