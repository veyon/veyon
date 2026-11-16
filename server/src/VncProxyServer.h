// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHostAddress>
#include <QVector>

#include "CryptoCore.h"

class QTcpServer;
class VncProxyConnection;
class VncProxyConnectionFactory;

class VncProxyServer : public QObject
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;
	using VncProxyConnectionList = QVector<VncProxyConnection*>;

	VncProxyServer(const QHostAddress& listenAddress,
				   int listenPort,
				   VncProxyConnectionFactory* clientFactory,
				   QObject* parent = nullptr);
	~VncProxyServer() override;

	bool start(int vncServerPort, const Password& vncServerPassword);
	void stop();

	const VncProxyConnectionList& clients() const
	{
		return m_connections;
	}

Q_SIGNALS:
	void connectionSynchronized(VncProxyConnection* connection);
	void connectionClosed(VncProxyConnection* connection);

private:
	void acceptConnection();
	void closeConnection(VncProxyConnection*);
	void handleAcceptError(QAbstractSocket::SocketError socketError);

	int m_vncServerPort;
	Password m_vncServerPassword;
	QHostAddress m_listenAddress;
	int m_listenPort;
	int m_connectionLimit;
	QTcpServer* m_server;
	VncProxyConnectionFactory* m_connectionFactory;
	VncProxyConnectionList m_connections;
};
