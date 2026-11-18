// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHostAddress>
#include <QVector>

#include "CryptoCore.h"
#include "TlsServerPolicy.h"
#include "VncServerSocket.h"

class TlsSniffingServer;
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

	bool start(const VncServerEndpoint& vncServerEndpoint, const Password& vncServerPassword,
			   const TlsServerPolicy& tlsServerPolicy);
	void stop();

	// frees the connection's pre-auth slot or marks it as failed authentication (05-tls.md §8)
	void reportAuthenticationResult(VncProxyConnection* connection, bool authenticated);

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

	static constexpr int MaximumPreAuthTimeoutSeconds = 300;

	VncServerEndpoint m_vncServerEndpoint;
	Password m_vncServerPassword;
	QHostAddress m_listenAddress;
	int m_listenPort;
	int m_connectionLimit;
	TlsSniffingServer* m_server{nullptr};
	VncProxyConnectionFactory* m_connectionFactory;
	VncProxyConnectionList m_connections;
};
