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
#include "DesktopAccessPermission.h"

class QTcpSocket;

class ServerAuthenticationManager : public QObject
{
	Q_OBJECT
public:
	ServerAuthenticationManager( FeatureWorkerManager& featureWorkerManager,
								 DesktopAccessDialog& desktopAccessDialog );

	bool authenticateClient( QTcpSocket* socket, RfbItalcAuth::Type authType );

	void setAllowedIPs( const QStringList &allowedIPs );

	bool performAccessControl( const QString& username, const QString& host,
							   DesktopAccessPermission::AuthenticationMethod authenticationMethod );


signals:
	void authenticationError( QString host, QString user );

private:
	bool performKeyAuthentication( QTcpSocket* socket );
	bool performLogonAuthentication( QTcpSocket* socket );
	bool performHostWhitelistAuth( QTcpSocket* socket );
	bool performTokenAuthentication( QTcpSocket* socket );

	FeatureWorkerManager& m_featureWorkerManager;
	DesktopAccessDialog& m_desktopAccessDialog;

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	QStringList m_failedAuthHosts;

} ;

#endif
