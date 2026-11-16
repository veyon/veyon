// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTcpSocket>

#include "ComputerControlClient.h"
#include "ComputerControlServer.h"
#include "VeyonCore.h"

#include <cstring>


ComputerControlClient::ComputerControlClient(ComputerControlServer* server,
											 QTcpSocket* clientSocket,
											 int vncServerPort,
											 const Password& vncServerPassword,
											 QObject* parent) :
	VncProxyConnection(clientSocket, vncServerPort, parent),
	m_server(server),
	m_serverClient(),
	m_serverProtocol(clientSocket,
					 &m_serverClient,
					 server->authenticationManager(),
					 server->accessControlManager()),
	m_clientProtocol(vncServerSocket(), vncServerPassword)
{
	m_framebufferUpdateTimer.start();
}



ComputerControlClient::~ComputerControlClient()
{
	m_server->accessControlManager().removeClient(&m_serverClient);
}



bool ComputerControlClient::receiveClientMessage()
{
	auto socket = proxyClientSocket();

	char messageType = 0;
	if (socket->peek(&messageType, sizeof(messageType)) != sizeof(messageType))
	{
		return false;
	}

	if (messageType == FeatureMessage::RfbMessageType)
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
		if (messageData.size() != sz_rfbFramebufferUpdateRequestMsg)
		{
			return false;
		}
		rfbFramebufferUpdateRequestMsg updateRequestMessage{};
		std::memcpy(&updateRequestMessage, messageData.constData(), sz_rfbFramebufferUpdateRequestMsg);

		if (updateRequestMessage.incremental &&
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
