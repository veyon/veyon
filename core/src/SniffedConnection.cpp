// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSslCipher>
#include <QSslSocket>

#include "ConnectionLimiter.h"
#include "SniffedConnection.h"
#include "TlsSniffingServer.h"


SniffedConnection::SniffedConnection(QSslSocket* socket, const QHostAddress& peerAddress, TlsSniffingServer* server,
									 const std::shared_ptr<ConnectionLimiter>& connectionLimiter) :
	QObject(socket),
	m_socket(socket),
	m_peerAddress(peerAddress),
	m_server(server),
	m_connectionLimiter(connectionLimiter)
{
	m_sniffTimer.setSingleShot(true);
	m_handshakeTimer.setSingleShot(true);
	m_preAuthTimer.setSingleShot(true);

	connect(&m_sniffTimer, &QTimer::timeout, this, [this]() { sniff(true); });
	connect(&m_handshakeTimer, &QTimer::timeout, this, [this]() { failHandshake(QStringLiteral("timeout")); });
	connect(&m_preAuthTimer, &QTimer::timeout, this, &SniffedConnection::handlePreAuthTimeout);
}



SniffedConnection::~SniffedConnection()
{
	if (m_authenticated == false)
	{
		m_connectionLimiter->closed(m_peerAddress, m_authFailed);
	}
}



void SniffedConnection::start()
{
	m_elapsedTimer.start();
	m_preAuthTimer.start(m_server->preAuthTimeout());

	// closed by the peer before it was handed over
	connect(m_socket, &QAbstractSocket::disconnected, this, [this]() { closeConnection(false); });

	// TLS disabled (or Optional without credentials): RFB banner right away
	if (m_server->isTlsEnabled() == false)
	{
		handOver();
		return;
	}

	connect(m_socket, &QSslSocket::readyRead, this, [this]() { sniff(false); });
	m_sniffTimer.start(m_server->policy().sniffTimeoutMs);
}



void SniffedConnection::setAuthenticationResult(bool authenticated)
{
	if (m_authenticated)
	{
		return;
	}

	if (authenticated == false)
	{
		m_authFailed = true;
		return;
	}

	m_authenticated = true;
	m_preAuthTimer.stop();
	m_connectionLimiter->authenticated(m_peerAddress);

	// nothing left to do for this connection
	deleteLater();
}



void SniffedConnection::sniff(bool sniffTimeoutExpired)
{
	if (m_state != State::Sniffing)
	{
		return;
	}

	const auto sniffTimeout = m_server->policy().sniffTimeoutMs;
	const auto elapsed = sniffTimeoutExpired ? qMax<qint64>(sniffTimeout, m_elapsedTimer.elapsed()) : m_elapsedTimer.elapsed();

	switch (SniffClassifier::classify(m_socket->peek(SniffClassifier::PeekSize), elapsed, sniffTimeout))
	{
	case SniffClassifier::Result::NeedMoreData:
		break;

	case SniffClassifier::Result::Tls:
		startHandshake();
		break;

	case SniffClassifier::Result::Plain:
		if (m_server->policy().mode == VeyonCore::TlsServerMode::Required)
		{
			vDebug() << "tls: closing plain connection from" << peerAddressString() << "(TLS required)";
			closeConnection(false);
		}
		else
		{
			handOver();
		}
		break;
	}
}



void SniffedConnection::startHandshake()
{
	m_state = State::Handshaking;
	m_sniffTimer.stop();
	disconnect(m_socket, &QSslSocket::readyRead, this, nullptr);

	connect(m_socket, &QSslSocket::encrypted, this, &SniffedConnection::finishHandshake);
	// the server verifies no peer certificates, so any reported error is fatal (never ignored)
	connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this,
			[this](const QList<QSslError>& errors) {
		failHandshake(errors.isEmpty() ? QString{} : errors.constFirst().errorString());
	});
	connect(m_socket, &QAbstractSocket::errorOccurred, this, [this]() { failHandshake(m_socket->errorString()); });

	m_handshakeTimer.start(m_server->policy().handshakeTimeoutMs);

	m_socket->setSslConfiguration(m_server->m_sslConfiguration);
	m_socket->startServerEncryption();
}



void SniffedConnection::finishHandshake()
{
	if (m_state != State::Handshaking)
	{
		return;
	}

	m_handshakeTimer.stop();

	vDebug() << "tls: connection from" << peerAddressString() << "encrypted with"
			 << m_socket->sessionCipher().name() << m_socket->sessionProtocol();

	handOver();
}



void SniffedConnection::failHandshake(const QString& reason)
{
	if (m_state != State::Handshaking)
	{
		return;
	}

	// one line, never any payload bytes (05-tls.md §3)
	vWarning() << qUtf8Printable(QStringLiteral("tls: handshake failed from %1: %2").arg(peerAddressString(), reason));

	closeConnection(true);
}



void SniffedConnection::handOver()
{
	m_state = State::HandedOver;
	m_sniffTimer.stop();
	m_handshakeTimer.stop();

	// from now on the socket belongs to the user of the pending connections
	disconnect(m_socket, nullptr, this, nullptr);

	m_server->addReadyConnection(m_socket);

	// the sniffing consumed the readyRead() of bytes a plain client sent before the banner
	if (m_socket->bytesAvailable() > 0)
	{
		QMetaObject::invokeMethod(m_socket, [socket = m_socket]() { Q_EMIT socket->readyRead(); }, Qt::QueuedConnection);
	}
}



void SniffedConnection::handlePreAuthTimeout()
{
	vWarning() << "closing connection from" << peerAddressString() << "after pre-auth timeout";

	if (m_state == State::HandedOver)
	{
		// counts as failure when the user of the socket deletes it
		m_authFailed = true;
		m_socket->abort();
	}
	else
	{
		closeConnection(true);
	}
}



QString SniffedConnection::peerAddressString() const
{
	// dual-stack listeners see IPv4 peers as IPv4-mapped IPv6 addresses
	bool isIPv4 = false;
	const auto ipv4Address = m_peerAddress.toIPv4Address(&isIPv4);
	return isIPv4 ? QHostAddress(ipv4Address).toString() : m_peerAddress.toString();
}



void SniffedConnection::closeConnection(bool authFailed)
{
	if (m_state == State::HandedOver || m_state == State::Closing)
	{
		return;
	}

	m_state = State::Closing;
	m_authFailed = m_authFailed || authFailed;

	m_sniffTimer.stop();
	m_handshakeTimer.stop();
	m_preAuthTimer.stop();

	disconnect(m_socket, nullptr, this, nullptr);
	m_socket->abort();
	// also deletes this object, which releases the limiter slot
	m_socket->deleteLater();
}
