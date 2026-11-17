// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTcpSocket>

#include "TlsSniffingServer.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "VncProxyConnection.h"
#include "VncProxyConnectionFactory.h"
#include "VncProxyServer.h"


VncProxyServer::VncProxyServer(const QHostAddress& listenAddress,
							   int listenPort,
							   VncProxyConnectionFactory* connectionFactory,
							   QObject* parent) :
	QObject(parent),
	m_vncServerEndpoint(),
	m_vncServerPassword(),
	m_listenAddress(listenAddress),
	m_listenPort(listenPort),
	m_connectionLimit(qMax(1, VeyonCore::config().serverConnectionLimit())),
	m_connectionFactory(connectionFactory)
{
}



VncProxyServer::~VncProxyServer()
{
	stop();
}



bool VncProxyServer::start(const VncServerEndpoint& vncServerEndpoint, const Password& vncServerPassword,
						   const TlsServerPolicy& tlsServerPolicy)
{
	if (m_server)
	{
		vWarning() << "VNC proxy server is already listening";
		return false;
	}

	m_vncServerEndpoint = vncServerEndpoint;
	m_vncServerPassword = vncServerPassword;

	const auto& config = VeyonCore::config();
	m_server = new TlsSniffingServer(tlsServerPolicy, ConnectionLimiter::configFromConfiguration(config),
									 qMax(1, config.connectionLimitPreAuthTimeoutSeconds()) * 1000, this);

	connect(m_server, &QTcpServer::newConnection, this, &VncProxyServer::acceptConnection);
	connect(m_server, &QTcpServer::acceptError, this, &VncProxyServer::handleAcceptError);

	if (m_listenPort < 0 ||
		m_server->listen(m_listenAddress, static_cast<quint16>(m_listenPort)) == false)
	{
		vWarning() << "could not listen on port" << m_listenPort << m_server->errorString();
		delete m_server;
		m_server = nullptr;
		return false;
	}

	vDebug() << "started on port" << m_listenPort;
	return true;
}



void VncProxyServer::stop()
{
	for (auto connection : std::as_const(m_connections))
	{
		delete connection;
	}

	m_connections.clear();

	// also closes all connections that have not been handed over yet
	delete m_server;
	m_server = nullptr;
}



void VncProxyServer::reportAuthenticationResult(VncProxyConnection* connection, bool authenticated)
{
	if (m_server && connection)
	{
		m_server->reportAuthenticationResult(connection->proxyClientSocket(), authenticated);
	}
}



void VncProxyServer::acceptConnection()
{
	while (m_server->hasPendingConnections())
	{
		auto clientSocket = m_server->nextPendingConnection();
		if (clientSocket == nullptr)
		{
			vCritical() << "ignoring invalid client socket";
			continue;
		}

		if (m_connections.size() >= m_connectionLimit)
		{
			vWarning() << "rejecting connection from" << clientSocket->peerAddress().toString()
					   << "because the connection limit of" << m_connectionLimit << "has been reached";
			clientSocket->abort();
			clientSocket->deleteLater();
			continue;
		}

		auto connection = m_connectionFactory->createVncProxyConnection(clientSocket,
																		m_vncServerEndpoint,
																		m_vncServerPassword,
																		this);
		if (connection == nullptr)
		{
			vCritical() << "connection factory returned no VNC proxy connection";
			clientSocket->abort();
			clientSocket->deleteLater();
			continue;
		}

		connect(connection, &VncProxyConnection::clientStreamSynchronized, this,
				[=, this]() { Q_EMIT connectionSynchronized(connection); }, Qt::DirectConnection);

		connect(connection, &VncProxyConnection::clientConnectionClosed, this, [=, this]() { closeConnection(connection); });
		connect(connection, &VncProxyConnection::serverConnectionClosed, this, [=, this]() { closeConnection(connection); });

		m_connections += connection;

		connection->start();
	}
}



void VncProxyServer::closeConnection(VncProxyConnection* connection)
{
	if (m_connections.removeAll(connection) == 0)
	{
		return;
	}

	Q_EMIT connectionClosed(connection);

	connection->deleteLater();
}



void VncProxyServer::handleAcceptError(QAbstractSocket::SocketError socketError)
{
	vCritical() << "error while accepting connection" << socketError;
}
