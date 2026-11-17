// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QLocalSocket>

#include "IpcConnection.h"
#include "IpcServer.h"
#include "IpcServerPlatform.h"


IpcServer::IpcServer(IpcProtocol::Role role, quint32 sessionId, QObject* parent) :
	QObject(parent),
	m_role(role),
	m_sessionId(sessionId),
	// one pre-auth connection at a time; loopback addresses are never rate-limited otherwise, the
	// failure limit is enforced via MaximumFailedHandshakes
	m_connectionLimiter(ConnectionLimiter::Config{1, IpcProtocol::MaximumFailedHandshakes, 60, 0}),
	m_listener(Listener::create(this))
{
	m_pendingPeerCheckTimer.setInterval(PendingPeerCheckInterval);
	connect(&m_pendingPeerCheckTimer, &QTimer::timeout, this, &IpcServer::checkPendingPeer);
}



IpcServer::~IpcServer()
{
	close();
}



bool IpcServer::listen(const QString& directory, const PeerPolicy& peerPolicy)
{
	if (m_credentials.has_value() || m_failed || m_connection)
	{
		return false;
	}

	const auto prefix = m_role == IpcProtocol::Role::Worker ? IpcProtocol::WorkerEndpointPrefix :
															  IpcProtocol::ServerEndpointPrefix;
	m_credentials = IpcEndpointCredentials::generate(prefix, int(m_sessionId));
	if (m_credentials.has_value() == false)
	{
		vCritical() << "failed to generate endpoint credentials";
		m_failed = true;
		return false;
	}

	m_endpointName = m_credentials->endpointName();
	m_peerPolicy = peerPolicy;

	if (m_listener->listen(directory) == false)
	{
		m_credentials.reset();
		m_failed = true;
		close();
		return false;
	}

	vDebug() << "listening at" << m_endpointName;

	return true;
}



VeyonCrypto::SecureBuffer IpcServer::credentialsLine() const
{
	if (m_credentials.has_value() == false)
	{
		return {};
	}

	return m_credentials->toLine();
}



void IpcServer::close()
{
	m_pendingPeerCheckTimer.stop();

	if (m_pendingSocket)
	{
		m_pendingSocket->abort();
		m_pendingSocket->deleteLater();
		m_pendingSocket = nullptr;
	}

	if (m_handshakeConnection)
	{
		m_handshakeConnection->disconnect(this);
		m_handshakeConnection->close();
		m_handshakeConnection->deleteLater();
		m_handshakeConnection = nullptr;
	}

	m_listener->close();
}



void IpcServer::acceptConnection(QLocalSocket* socket)
{
	socket->setParent(this);

	if (m_failed || m_connection || m_credentials.has_value() == false)
	{
		socket->abort();
		socket->deleteLater();
		return;
	}

	// the peer check comes first: connections of other processes are dropped before anything is
	// parsed and never count as failed attempts, so they can't exhaust the attempts of the child
	const auto peerCheckResult = checkPeer(socket);
	if (peerCheckResult == PeerCheckResult::Rejected)
	{
		dropConnection(socket, "peer check failed", false);
		return;
	}

	// the child never connects twice at the same time, keep the running attempt
	if (m_pendingSocket || m_handshakeConnection)
	{
		dropConnection(socket, "concurrent connection", false);
		return;
	}

	if (m_connectionLimiter.admit(QHostAddress::LocalHost) == false)
	{
		dropConnection(socket, "connection limit reached", false);
		return;
	}

	if (peerCheckResult == PeerCheckResult::Pending)
	{
		// Windows only: the child connected before its process ID was registered (stdin is
		// written while the process is created) - do not read anything until the check passed
		m_pendingSocket = socket;
		m_pendingSince = IpcChannel::steadyNow();
		m_pendingPeerCheckTimer.start();
		return;
	}

	startHandshake(socket);
}



void IpcServer::checkPendingPeer()
{
	if (m_pendingSocket.isNull())
	{
		m_pendingPeerCheckTimer.stop();
		return;
	}

	auto result = checkPeer(m_pendingSocket);
	if (result == PeerCheckResult::Pending &&
		IpcChannel::steadyNow() - m_pendingSince > IpcProtocol::HandshakeTimeout)
	{
		result = PeerCheckResult::Rejected;
	}

	if (result == PeerCheckResult::Pending)
	{
		return;
	}

	m_pendingPeerCheckTimer.stop();
	QLocalSocket* socket = m_pendingSocket;
	m_pendingSocket = nullptr;

	if (result == PeerCheckResult::Accepted)
	{
		startHandshake(socket);
	}
	else
	{
		dropConnection(socket, "peer check failed", true);
	}
}



IpcServer::PeerCheckResult IpcServer::checkPeer(QLocalSocket* socket) const
{
	const auto peer = peerIdentity(socket);
	if (peer.has_value() == false)
	{
		vWarning() << "could not determine peer identity at" << m_endpointName;
		return PeerCheckResult::Rejected;
	}

	if (checkPeerUser(*peer) == false)
	{
		return PeerCheckResult::Rejected;
	}

	if (m_peerPolicy.checkProcessId)
	{
		const ProcessId expectedProcessId = m_expectedProcessId;
		if (expectedProcessId == InvalidProcessId)
		{
			return checkUnregisteredPeer();
		}

		if (peer->processId != expectedProcessId)
		{
			vWarning() << "peer at" << m_endpointName << "has pid" << peer->processId << "instead of" << expectedProcessId;
			return PeerCheckResult::Rejected;
		}
	}

	return PeerCheckResult::Accepted;
}



void IpcServer::startHandshake(QLocalSocket* socket)
{
	auto connection = new IpcConnection(socket, IpcChannel::Side::Server, m_role, m_sessionId,
										m_credentials->clone(), &m_nonceHistory, this);
	m_handshakeConnection = connection;

	connect(connection, &IpcConnection::established, this, &IpcServer::handleEstablished);
	connect(connection, &IpcConnection::closed, this, [this, connection]() {
		if (connection == m_handshakeConnection)
		{
			m_handshakeConnection = nullptr;
			connection->deleteLater();
			m_connectionLimiter.closed(QHostAddress::LocalHost, true);
			vWarning() << "handshake failed at" << m_endpointName;
			handleFailure();
		}
	});

	connection->start();
}



void IpcServer::dropConnection(QLocalSocket* socket, const char* reason, bool admitted)
{
	// limit the log volume of a process flooding the endpoint
	++m_droppedConnections;
	if (m_droppedConnections <= MaximumLoggedDroppedConnections || m_droppedConnections % 100 == 0)
	{
		vWarning() << "dropping connection to" << m_endpointName << "-" << reason
				   << "(" << m_droppedConnections << "dropped so far)";
	}

	socket->abort();
	socket->deleteLater();

	if (admitted)
	{
		m_connectionLimiter.closed(QHostAddress::LocalHost, false);
	}

	m_listener->ensureListener();
}



void IpcServer::handleFailure()
{
	if (m_failed)
	{
		return;
	}

	++m_failedAttempts;

	if (m_failedAttempts >= m_peerPolicy.maximumFailedHandshakes)
	{
		vCritical() << "too many failed connection attempts at" << m_endpointName << "- closing endpoint";
		m_failed = true;
		m_credentials.reset();
		close();
		Q_EMIT failed();
		return;
	}

	// without a listening instance the child could not connect again
	if (m_listener->ensureListener() == false)
	{
		vCritical() << "no listening pipe instance at" << m_endpointName << "- closing endpoint";
		m_failed = true;
		m_credentials.reset();
		close();
		Q_EMIT failed();
	}
}



void IpcServer::handleEstablished()
{
	m_connection = m_handshakeConnection;
	m_handshakeConnection = nullptr;

	m_connectionLimiter.authenticated(QHostAddress::LocalHost);

	// one connection per child: wipe the token and close the listener
	m_credentials.reset();
	close();

	vDebug() << "connection established at" << m_endpointName;

	Q_EMIT connectionEstablished(m_connection);
}
