/*
 * ServerAuthenticationManager.h - header file for ServerAuthenticationManager
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

#ifndef SERVER_AUTHENTICATION_MANAGER_H
#define SERVER_AUTHENTICATION_MANAGER_H

#include <QMutex>
#include <QStringList>

#include "RfbItalcAuth.h"

class VariantArrayMessage;

class ServerAuthenticationManager : public QObject
{
	Q_OBJECT
public:
	class Client
	{
	public:
		typedef enum States {
			Init,
			Challenge,
			Password,
			FinishedSuccess,
			FinishedFail,
		} State;

		Client( RfbItalcAuth::Type authType, const QString& username, const QString& hostAddress ) :
			m_state( Init ),
			m_authType( authType ),
			m_username( username ),
			m_hostAddress( hostAddress ),
			m_challenge()
		{
		}

		State state() const
		{
			return m_state;
		}

		void setState( State state )
		{
			m_state = state;
		}

		RfbItalcAuth::Type authType() const
		{
			return m_authType;
		}

		const QString& username() const
		{
			return m_username;
		}

		const QString& hostAddress() const
		{
			return m_hostAddress;
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
		State m_state;
		RfbItalcAuth::Type m_authType;
		QString m_username;
		QString m_hostAddress;
		QByteArray m_challenge;
		QString m_privateKey;

	} ;

	ServerAuthenticationManager( QObject* parent );

	const QVector<RfbItalcAuth::Type>& supportedAuthTypes() const
	{
		return m_supportedAuthTypes;
	}

	void processAuthenticationMessage( Client* client,
									   VariantArrayMessage& message );

	void setAllowedIPs( const QStringList &allowedIPs );


signals:
	void authenticationError( QString host, QString user );

private:
	Client::State performKeyAuthentication( Client* client, VariantArrayMessage& message );
	Client::State performLogonAuthentication( Client* client, VariantArrayMessage& message );
	Client::State performHostWhitelistAuth( Client* client, VariantArrayMessage& message );
	Client::State performTokenAuthentication( Client* client, VariantArrayMessage& message );

	QVector<RfbItalcAuth::Type> m_supportedAuthTypes;

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	QStringList m_failedAuthHosts;

} ;

#endif
