/*
 * VncServerClient.h - header file for the VncServerClient class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef VNC_SERVER_CLIENT_H
#define VNC_SERVER_CLIENT_H

#include <QElapsedTimer>

#include "VncServerProtocol.h"

class VEYON_CORE_EXPORT VncServerClient : public QObject
{
	Q_OBJECT
public:
	typedef enum AuthStates {
		AuthInit,
		AuthChallenge,
		AuthPassword,
		AuthToken,
		AuthFinishedSuccess,
		AuthFinishedFail,
	} AuthState;

	typedef enum AccessControlStates {
		AccessControlInit,
		AccessControlSuccessful,
		AccessControlPending,
		AccessControlWaiting,
		AccessControlFailed,
		AccessControlStateCount
	} AccessControlState;

	VncServerClient( QObject* parent = nullptr ) :
		QObject( parent ),
		m_protocolState( VncServerProtocol::Disconnected ),
		m_authState( AuthInit ),
		m_authType( RfbVeyonAuth::Invalid ),
		m_accessControlState( AccessControlInit ),
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

	const QString& privateKey() const
	{
		return m_privateKey;
	}

	void setPrivateKey( const QString& privateKey )
	{
		m_privateKey = privateKey;
	}

public slots:
	void finishAccessControl()
	{
		emit accessControlFinished( this );
	}

signals:
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
	QString m_privateKey;

} ;

typedef QList<VncServerClient *> VncServerClientList;

#endif
