/*
 * ComputerControlClient.cpp - implementation of the ComputerControlClient class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QTcpSocket>

#include "VeyonCore.h"
#include "ComputerControlClient.h"
#include "ComputerControlServer.h"


ComputerControlClient::ComputerControlClient( ComputerControlServer* server,
											  QTcpSocket* clientSocket,
											  int vncServerPort,
											  const Password& vncServerPassword,
											  QObject* parent ) :
	VncProxyConnection( clientSocket, vncServerPort, parent ),
	m_server( server ),
	m_serverProtocol( clientSocket,
					  &m_serverClient,
					  server->authenticationManager(),
					  server->accessControlManager() ),
	m_clientProtocol( vncServerSocket(), vncServerPassword )
{
	m_framebufferUpdateTimer.start();
}



ComputerControlClient::~ComputerControlClient()
{
	m_server->accessControlManager().removeClient( &m_serverClient );
}



bool ComputerControlClient::receiveClientMessage()
{
	auto socket = proxyClientSocket();

	char messageType = 0;
	if( socket->peek( &messageType, sizeof(messageType) ) != sizeof(messageType) )
	{
		return false;
	}

	if( messageType == FeatureMessage::RfbMessageType )
	{
		return m_server->handleFeatureMessage(this);
	}

	// filter framebuffer update requests when minimum framebuffer update interval is set
	if (messageType == rfbFramebufferUpdateRequest && m_minimumFramebufferUpdateInterval > 0)
	{
		if (socket->bytesAvailable() < sz_rfbFramebufferUpdateRequestMsg)
		{
			return false;
		}

		const auto messageData = socket->read(sz_rfbFramebufferUpdateRequestMsg);
		const auto updateRequestMessage = reinterpret_cast<const rfbFramebufferUpdateRequestMsg *>(messageData.constData());

		if (updateRequestMessage->incremental &&
			m_framebufferUpdateTimer.hasExpired(m_minimumFramebufferUpdateInterval) == false)
		{
			// discard update request
			return true;
		}

		// forward request to server
		m_framebufferUpdateTimer.restart();
		return vncServerSocket()->write(messageData) == messageData.size();
	}

	return VncProxyConnection::receiveClientMessage();
}



void ComputerControlClient::setMinimumFramebufferUpdateInterval(int interval)
{
	m_minimumFramebufferUpdateInterval = interval;
}
