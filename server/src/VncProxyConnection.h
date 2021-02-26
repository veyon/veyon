/*
 * VncProxyConnection.h - class representing a connection within VncProxyServer
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

#include "VeyonCore.h"

class QBuffer;
class QTcpSocket;

class VncClientProtocol;
class VncServerProtocol;

class VncProxyConnection : public QObject
{
	Q_OBJECT
public:
	enum {
		ProtocolRetryTime = 250
	};

	VncProxyConnection( QTcpSocket* clientSocket, int vncServerPort, QObject* parent );
	~VncProxyConnection() override;

	void start();

	QTcpSocket* proxyClientSocket() const
	{
		return m_proxyClientSocket;
	}

	QTcpSocket* vncServerSocket() const
	{
		return m_vncServerSocket;
	}

protected Q_SLOTS:
	void readFromClient();
	void readFromServer();

protected:
	bool forwardDataToClient( qint64 size );
	bool forwardDataToServer( qint64 size );

	void readFromServerLater();
	void readFromClientLater();

	virtual bool receiveClientMessage();
	virtual bool receiveServerMessage();

	virtual VncClientProtocol& clientProtocol() = 0;
	virtual VncServerProtocol& serverProtocol() = 0;

private:
	const int m_vncServerPort;

	QTcpSocket* m_proxyClientSocket;
	QTcpSocket* m_vncServerSocket;

	const QMap<int, int> m_rfbClientToServerMessageSizes;

Q_SIGNALS:
	void clientConnectionClosed();
	void serverConnectionClosed();

} ;
