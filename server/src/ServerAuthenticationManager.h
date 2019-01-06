/*
 * ServerAuthenticationManager.h - header file for ServerAuthenticationManager
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

#ifndef SERVER_AUTHENTICATION_MANAGER_H
#define SERVER_AUTHENTICATION_MANAGER_H

#include <QMutex>
#include <QStringList>

#include "RfbVeyonAuth.h"
#include "VncServerClient.h"

class VariantArrayMessage;

class ServerAuthenticationManager : public QObject
{
	Q_OBJECT
public:
	typedef enum AuthResults {
		AuthResultSuccessful,
		AuthResultFailed,
		AuthResultCount
	} AuthResult;

	ServerAuthenticationManager( QObject* parent );

	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const;

	void processAuthenticationMessage( VncServerClient* client,
									   VariantArrayMessage& message );

	void setAllowedIPs( const QStringList &allowedIPs );


signals:
	void authenticationDone( AuthResult result, const QString& host, const QString& user );

private:
	VncServerClient::AuthState performKeyAuthentication( VncServerClient* client, VariantArrayMessage& message );
	VncServerClient::AuthState performLogonAuthentication( VncServerClient* client, VariantArrayMessage& message );
	VncServerClient::AuthState performHostWhitelistAuth( VncServerClient* client, VariantArrayMessage& message );
	VncServerClient::AuthState performTokenAuthentication( VncServerClient* client, VariantArrayMessage& message );

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	QStringList m_failedAuthHosts;

} ;

#endif
