/*
 * DemoServerConnection.h - header file for DemoServerConnection class
 *
 * Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "DemoServerProtocol.h"

class DemoServer;

// clazy:excludeall=ctor-missing-parent-argument

// the demo server creates an instance of this class for each client connection,
// i.e. each client is connected to a different server thread for best
// performance
class DemoServerConnection : public QThread
{
	Q_OBJECT
public:
	static constexpr int ProtocolRetryTime = 250;

	DemoServerConnection( DemoServer* demoServer, const DemoAuthentication& authentication, quintptr socketDescriptor );
	~DemoServerConnection() = default;

private:
	void run() override;

	void processClient(); // clazy:exclude=thread-with-slots
	void sendFramebufferUpdate();

	bool receiveClientMessage();

	const DemoAuthentication& m_authentication;
	DemoServer* m_demoServer;

	quintptr m_socketDescriptor;
	QTcpSocket* m_socket{nullptr};

	VncServerClient m_vncServerClient{};
	DemoServerProtocol* m_serverProtocol{nullptr};

	const QMap<int, int> m_rfbClientToServerMessageSizes;

	int m_keyFrame{-1};
	int m_framebufferUpdateMessageIndex{0};

	const int m_framebufferUpdateInterval;

} ;
