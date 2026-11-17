// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QTimer>

#include "VncServerSocket.h"

class QBuffer;
class QIODevice;
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

	VncProxyConnection(QTcpSocket* clientSocket, const VncServerEndpoint& vncServerEndpoint, QObject* parent);
	~VncProxyConnection() override;

	void start();

	QTcpSocket* proxyClientSocket() const
	{
		return m_proxyClientSocket;
	}

	QIODevice* vncServerSocket() const
	{
		return m_vncServerSocket->device();
	}

protected Q_SLOTS:
	void readFromClient();
	void readFromServer();

protected:
	bool forwardDataToServer(qint64 size);

	void readFromServerLater();
	void readFromClientLater();

	virtual bool receiveClientMessage();
	virtual bool receiveServerMessage();

	virtual VncClientProtocol& clientProtocol() = 0;
	virtual VncServerProtocol& serverProtocol() = 0;

private:
	bool flushPendingToSocket(QIODevice* target, QByteArray& pending);
	void closeSockets();
	bool synchronizeClientStream();

	void updateHandshakeState();

	static constexpr auto HandshakeTimeout = 30000;
	static constexpr auto ClientSyncInterval = 500;
	static constexpr auto MaximumReadBufferSize = 64 * 1024 * 1024;
	static constexpr auto MaximumPendingWriteSize = 64 * 1024 * 1024;

	QTcpSocket* m_proxyClientSocket;
	VncServerSocket* m_vncServerSocket;

	const QMap<int, int> m_rfbClientToServerMessageSizes;
	QTimer m_clientSyncTimer{this};
	QTimer m_clientRetryTimer{this};
	QTimer m_serverRetryTimer{this};
	QTimer m_handshakeTimer{this};
	QByteArray m_pendingClientData{};
	QByteArray m_pendingServerData{};

Q_SIGNALS:
	void clientConnectionClosed();
	void serverConnectionClosed();
	void clientStreamSynchronized();
};
