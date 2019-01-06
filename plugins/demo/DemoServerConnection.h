/*
 * DemoServerConnection.h - header file for DemoServerConnection class
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef DEMO_SERVER_CONNECTION_H
#define DEMO_SERVER_CONNECTION_H

#include "DemoServerProtocol.h"

class DemoServer;

// clazy:excludeall=ctor-missing-parent-argument

// the demo server creates an instance of this class for each client connection,
// i.e. each client is connected to a different server thread for best
// performance
class DemoServerConnection : public QObject
{
	Q_OBJECT
public:
	enum {
		ProtocolRetryTime = 250,
	};

	DemoServerConnection( const QString& demoAccessToken, QTcpSocket* socket, DemoServer* demoServer );
	~DemoServerConnection() override;

public slots:
	void processClient();
	void sendFramebufferUpdate();

private:
	bool receiveClientMessage();

	DemoServer* m_demoServer;

	QTcpSocket* m_socket;

	VncServerClient m_vncServerClient;
	DemoServerProtocol m_serverProtocol;

	const QMap<int, int> m_rfbClientToServerMessageSizes;

	int m_keyFrame;
	int m_framebufferUpdateMessageIndex;

	const int m_framebufferUpdateInterval;

} ;

#endif
