// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#include "VncClientProtocol.h"
#include "VncProxyConnection.h"
#include "VncServerProtocol.h"

VncProxyConnection::VncProxyConnection(QTcpSocket* clientSocket,
									   const VncServerEndpoint& vncServerEndpoint,
									   QObject* parent) :
	QObject(parent),
	m_proxyClientSocket(clientSocket),
	m_vncServerSocket(new VncServerSocket(vncServerEndpoint, this)),
	m_rfbClientToServerMessageSizes({
		{rfbFramebufferUpdateRequest, sz_rfbFramebufferUpdateRequestMsg},
		{rfbKeyEvent, sz_rfbKeyEventMsg},
		{rfbPointerEvent, sz_rfbPointerEventMsg},
		{rfbXvp, sz_rfbXvpMsg},
	})
{
	m_proxyClientSocket->setReadBufferSize(MaximumReadBufferSize);
	m_vncServerSocket->setReadBufferSize(MaximumReadBufferSize);

	m_clientRetryTimer.setSingleShot(true);
	m_serverRetryTimer.setSingleShot(true);
	m_handshakeTimer.setSingleShot(true);

	connect(m_proxyClientSocket, &QTcpSocket::readyRead, this, &VncProxyConnection::readFromClient);
	connect(m_vncServerSocket->device(), &QIODevice::readyRead, this, &VncProxyConnection::readFromServer);
	connect(&m_clientSyncTimer, &QTimer::timeout, this, &VncProxyConnection::synchronizeClientStream);
	connect(&m_clientRetryTimer, &QTimer::timeout, this, &VncProxyConnection::readFromClient);
	connect(&m_serverRetryTimer, &QTimer::timeout, this, &VncProxyConnection::readFromServer);
	connect(&m_handshakeTimer, &QTimer::timeout, this, [this]() {
		vWarning() << "closing connection after RFB handshake timeout from"
				   << m_proxyClientSocket->peerAddress().toString();
		closeSockets();
	});
	connect(m_vncServerSocket->device(), &QIODevice::bytesWritten, this, [this] { readFromClientLater(); });
	connect(m_proxyClientSocket, &QTcpSocket::bytesWritten, this, [this] { readFromServerLater(); });

	connect(m_vncServerSocket, &VncServerSocket::disconnected, this, &VncProxyConnection::clientConnectionClosed);
	connect(m_proxyClientSocket, &QTcpSocket::disconnected, this, &VncProxyConnection::serverConnectionClosed);
}



VncProxyConnection::~VncProxyConnection()
{
	// do not get notified about disconnects any longer
	disconnect(m_vncServerSocket);
	disconnect(m_vncServerSocket->device());
	disconnect(m_proxyClientSocket);

	delete m_vncServerSocket;
	delete m_proxyClientSocket;
}



void VncProxyConnection::start()
{
	m_handshakeTimer.start(HandshakeTimeout);
	serverProtocol().start();
}



void VncProxyConnection::readFromClient()
{
	if (serverProtocol().state() != VncServerProtocol::State::Running)
	{
		while (serverProtocol().read()) // Flawfinder: ignore
		{
		}

		// try again later in case we could not proceed because of
		// external protocol dependencies or in case we're finished
		// and already have RFB messages in receive queue
		readFromClientLater();
	}
	else if (clientProtocol().state() == VncClientProtocol::Running)
	{
		while (receiveClientMessage())
		{
		}
	}
	else
	{
		// try again as client connection is not yet ready and we can't forward data
		readFromClientLater();
	}

	if (serverProtocol().state() == VncServerProtocol::State::FramebufferInit &&
		clientProtocol().state() == VncClientProtocol::Disconnected)
	{
		// no unauthenticated fallback towards veyon-capture-helper (02-ipc.md §1a.5)
		clientProtocol().setVncAuthenticationRequired(m_vncServerSocket->endpoint().isLocalSocket());
		m_vncServerSocket->connectToServer();

		clientProtocol().start();
	}

	updateHandshakeState();
}



void VncProxyConnection::readFromServer()
{
	if (clientProtocol().state() != VncClientProtocol::Running)
	{
		while (clientProtocol().read()) // Flawfinder: ignore
		{
		}

		// did we finish client protocol initialization? then we must not miss this
		// read signaĺ from server but process it as the server is still waiting
		// for our response
		if (clientProtocol().state() == VncClientProtocol::Running)
		{
			// if client protocol is running we have the server init message which
			// we can forward to the real client
			serverProtocol().setServerInitMessage(clientProtocol().serverInitMessage());

			readFromServerLater();
		}
	}
	else if (serverProtocol().state() == VncServerProtocol::State::Running)
	{
		while (synchronizeClientStream() && receiveServerMessage())
		{
		}
	}
	else
	{
		// try again as server connection is not yet ready and we can't forward data
		readFromServerLater();
	}

	updateHandshakeState();
}



bool VncProxyConnection::flushPendingToSocket(QIODevice* target, QByteArray& pending)
{
	if (pending.isEmpty())
	{
		return true;
	}

	const auto written = target->write(pending);
	if (written < 0)
	{
		closeSockets();
		return false;
	}

	if (written > 0)
	{
		pending.remove(0, static_cast<qsizetype>(written));
	}

	if (pending.size() > MaximumPendingWriteSize)
	{
		vCritical() << "closing slow peer"
					<< (target == m_proxyClientSocket ? m_proxyClientSocket->peerAddress().toString() :
														m_vncServerSocket->peerName())
					<< "with oversized pending write buffer";
		closeSockets();
		return false;
	}

	return pending.isEmpty();
}



bool VncProxyConnection::synchronizeClientStream()
{
	if (serverProtocol().state() == VncServerProtocol::State::Running &&
		clientProtocol().state() == VncClientProtocol::Running &&
		flushPendingToSocket(m_proxyClientSocket, m_pendingClientData))
	{
		Q_EMIT clientStreamSynchronized();

		return true;
	}

	return false;
}



bool VncProxyConnection::forwardDataToServer(qint64 size)
{
	if (!flushPendingToSocket(m_vncServerSocket->device(), m_pendingServerData))
	{
		return false;
	}

	if (m_proxyClientSocket->bytesAvailable() < size)
	{
		return false;
	}

	const auto data = m_proxyClientSocket->read(size);
	if (data.size() != size)
	{
		return false;
	}

	const auto written = m_vncServerSocket->device()->write(data);
	if (written < 0)
	{
		m_vncServerSocket->device()->close();
		return false;
	}

	if (written < size)
	{
		m_pendingServerData.append(data.mid(static_cast<qsizetype>(written)));
		if (m_pendingServerData.size() > MaximumPendingWriteSize)
		{
			vCritical() << "closing slow VNC server with oversized pending write buffer";
			closeSockets();
		}
		return false;
	}

	return true;
}



void VncProxyConnection::closeSockets()
{
	m_proxyClientSocket->close();
	m_vncServerSocket->device()->close();
}



void VncProxyConnection::readFromServerLater()
{
	if (m_serverRetryTimer.isActive() == false)
	{
		m_serverRetryTimer.start(ProtocolRetryTime);
	}
}



void VncProxyConnection::readFromClientLater()
{
	if (m_clientRetryTimer.isActive() == false)
	{
		m_clientRetryTimer.start(ProtocolRetryTime);
	}
}



bool VncProxyConnection::receiveClientMessage()
{
	auto socket = proxyClientSocket();

	uint8_t messageType = 0;
	if (socket->peek(reinterpret_cast<char*>(&messageType), sizeof(messageType)) != sizeof(messageType))
	{
		return false;
	}

	switch (messageType)
	{
	case rfbSetEncodings:
		if (socket->bytesAvailable() >= sz_rfbSetEncodingsMsg)
		{
			rfbSetEncodingsMsg setEncodingsMessage;
			if (socket->peek(reinterpret_cast<char*>(&setEncodingsMessage), sz_rfbSetEncodingsMsg) == sz_rfbSetEncodingsMsg)
			{
				const auto nEncodings = qFromBigEndian(setEncodingsMessage.nEncodings);
				if (nEncodings > MAX_ENCODINGS)
				{
					vCritical() << "received too many encodings from client";
					socket->close();
					return false;
				}
				return forwardDataToServer(sz_rfbSetEncodingsMsg + nEncodings * sizeof(uint32_t));
			}
		}
		break;

	case rfbSetPixelFormat:
		if (socket->bytesAvailable() >= sz_rfbSetPixelFormatMsg)
		{
			rfbSetPixelFormatMsg setPixelFormatMessage;
			if (socket->peek(reinterpret_cast<char*>(&setPixelFormatMessage), sz_rfbSetPixelFormatMsg) == sz_rfbSetPixelFormatMsg)
			{
				auto format = setPixelFormatMessage.format;
				if ((format.bitsPerPixel != 8 && format.bitsPerPixel != 16 && format.bitsPerPixel != 32) ||
					format.depth > format.bitsPerPixel)
				{
					vCritical() << "rejecting invalid pixel format" << format.bitsPerPixel << format.depth;
					return false;
				}
				format.redMax = qFromBigEndian(format.redMax);
				format.greenMax = qFromBigEndian(format.greenMax);
				format.blueMax = qFromBigEndian(format.blueMax);
				clientProtocol().setPixelFormat(format);

				return forwardDataToServer(sz_rfbSetPixelFormatMsg);
			}
		}
		break;

	default:
		if (m_rfbClientToServerMessageSizes.contains(messageType) == false)
		{
			vCritical() << "received unknown message type:" << static_cast<int>(messageType);
			socket->close();
			return false;
		}

		return forwardDataToServer(m_rfbClientToServerMessageSizes[messageType]);
	}

	return false;
}



bool VncProxyConnection::receiveServerMessage()
{
	if (clientProtocol().receiveMessage())
	{
		const auto& message = clientProtocol().lastMessage();
		const auto written = m_proxyClientSocket->write(message);
		if (written < 0)
		{
			m_proxyClientSocket->close();
			return false;
		}
		if (written < message.size())
		{
			m_pendingClientData = message.mid(static_cast<qsizetype>(written));
			if (m_pendingClientData.size() > MaximumPendingWriteSize)
			{
				vCritical() << "closing slow client with oversized pending write buffer";
				m_proxyClientSocket->close();
			}
		}

		return true;
	}

	return false;
}



void VncProxyConnection::updateHandshakeState()
{
	if (serverProtocol().state() == VncServerProtocol::State::Running &&
		clientProtocol().state() == VncClientProtocol::Running)
	{
		m_handshakeTimer.stop();

		m_clientSyncTimer.start(ClientSyncInterval);
	}
}
