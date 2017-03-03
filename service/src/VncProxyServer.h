/*
 * VncProxyServer.h - a VNC proxy implementation for intercepting VNC connections
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

#ifndef VNC_PROXY_H
#define VNC_PROXY_H

#include <QObject>
#include <QVector>

class QTcpServer;
class VncProxyClient;
class VncProxyClientFactory;

class VncProxyServer : public QObject
{
	Q_OBJECT
public:
	typedef QVector<VncProxyClient *> VncProxyClientList;

	VncProxyServer( int vncServerPort, int listenPort, VncProxyClientFactory* clientFactory, QObject* parent = nullptr );
	virtual ~VncProxyServer();

	void start();

	const VncProxyClientList& clients() const
	{
		return m_clients;
	}

private slots:
	void acceptConnection();
	void closeConnection( VncProxyClient* );

private:
	int m_vncServerPort;
	int m_listenPort;
	QTcpServer* m_server;
	VncProxyClientFactory* m_clientFactory;
	VncProxyClientList m_clients;

} ;

#endif
