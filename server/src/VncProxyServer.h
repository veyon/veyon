/*
 * VncProxyServer.h - a VNC proxy implementation for intercepting VNC connections
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QHostAddress>
#include <QVector>

class QTcpServer;
class VncProxyConnection;
class VncProxyConnectionFactory;

class VncProxyServer : public QObject
{
	Q_OBJECT
public:
	typedef QVector<VncProxyConnection *> VncProxyConnectionList;

	VncProxyServer( const QHostAddress& listenAddress,
					int listenPort,
					VncProxyConnectionFactory* clientFactory,
					QObject* parent = nullptr );
	~VncProxyServer() override;

	bool start( int vncServerPort, const QString& vncServerPassword );
	void stop();

	const VncProxyConnectionList& clients() const
	{
		return m_connections;
	}

private:
	void acceptConnection();
	void closeConnection( VncProxyConnection* );

	int m_vncServerPort;
	QString m_vncServerPassword;
	QHostAddress m_listenAddress;
	int m_listenPort;
	QTcpServer* m_server;
	VncProxyConnectionFactory* m_connectionFactory;
	VncProxyConnectionList m_connections;

} ;
