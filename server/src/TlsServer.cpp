/*
 * TlsServer.cpp - header file for TlsServer
 *
 * Copyright (c) 2021-2022 Tobias Junghans <tobydox@veyon.io>
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

#include <QSslKey>
#include <QSslSocket>

#include "TlsServer.h"


TlsServer::TlsServer( const VeyonCore::TlsConfiguration& tlsConfig, QObject* parent ) :
	QTcpServer( parent ),
	m_tlsConfig( tlsConfig )
{
}



void TlsServer::incomingConnection( qintptr socketDescriptor )
{
	if( m_tlsConfig.localCertificate().isNull() || m_tlsConfig.privateKey().isNull() )
	{
		auto socket = new QTcpSocket;
		if( socket->setSocketDescriptor(socketDescriptor) )
		{
			vDebug() << "accepting unencrypted connection for socket" << socketDescriptor;
			addPendingConnection( socket );
		}
		else
		{
			vCritical() << "failed to set socket descriptor for incoming non-TLS connection";
			delete socket;
		}
	}
	else
	{
		auto socket = new QSslSocket;
		if( socket->setSocketDescriptor(socketDescriptor) )
		{
			connect(socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
					 [this, socket]( const QList<QSslError> &errors) {
						 for( const auto& err : errors )
						 {
							 vCritical() << "SSL error" << err;
						 }

						 Q_EMIT tlsErrors( socket, errors );
					 } );

			connect(socket, &QSslSocket::encrypted, this,
					 []() { vDebug() << "connection encryption established"; } );

			socket->setSslConfiguration( m_tlsConfig );
			socket->startServerEncryption();

			vDebug() << "establishing TLS connection for socket" << socketDescriptor;
			addPendingConnection( socket );
		}
		else
		{
			vCritical() << "failed to set socket descriptor for incoming TLS connection";
			delete socket;
		}
	}
}
