// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QHostAddress>
#include <QPointer>
#include <QSslConfiguration>
#include <QTimer>

#include "SniffClassifier.h"

#include <memory>

class QSslSocket;
class ConnectionLimiter;
class TlsSniffingServer;

// State of one connection accepted by TlsSniffingServer (05-tls.md §1.1–1.4), a child of its
// socket: decides between TLS and plain, performs the TLS handshake, hands the socket over to the
// server's pending connections and enforces the pre-auth timeout until the authentication result
// is reported. It is deleted together with the socket or once the connection authenticated, and
// releases the ConnectionLimiter slot the connection was admitted with exactly once.
class SniffedConnection : public QObject
{
	Q_OBJECT
public:
	enum class State {
		Sniffing,
		Handshaking,
		HandedOver,
		Closing,
	};
	Q_ENUM(State)

	SniffedConnection(QSslSocket* socket, const QHostAddress& peerAddress, TlsSniffingServer* server,
					  const std::shared_ptr<ConnectionLimiter>& connectionLimiter);
	~SniffedConnection() override;

	void start();

	State state() const
	{
		return m_state;
	}

	void setAuthenticationResult(bool authenticated);

private:
	void sniff(bool sniffTimeoutExpired);
	void startHandshake();
	void finishHandshake();
	void failHandshake(const QString& reason);
	void handOver();
	void handlePreAuthTimeout();
	void closeConnection(bool authFailed);
	QString peerAddressString() const;

	QSslSocket* const m_socket;
	const QHostAddress m_peerAddress;
	const QPointer<TlsSniffingServer> m_server;
	const std::shared_ptr<ConnectionLimiter> m_connectionLimiter;

	State m_state{State::Sniffing};
	bool m_authFailed{false};
	bool m_authenticated{false};
	QElapsedTimer m_elapsedTimer;
	QTimer m_sniffTimer{this};
	QTimer m_handshakeTimer{this};
	QTimer m_preAuthTimer{this};
};
