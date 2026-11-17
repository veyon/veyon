// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSslConfiguration>
#include <QTcpServer>

#include "ConnectionLimiter.h"
#include "TlsServerPolicy.h"

#include <memory>

class QSslSocket;

// TCP listener that serves TLS and plain RFB clients on the same port (05-tls.md §1). Every
// accepted connection is admitted by a per-listener ConnectionLimiter (§8) and then handled by a
// SniffedConnection: depending on the policy it waits for the client's first bytes (TLS clients
// speak first, plain RFB clients wait for the server's banner) and performs the TLS handshake.
// Connections appear as pending connections (newConnection(), nextPendingConnection()) once
// they are ready for the RFB protocol: a QSslSocket, encrypted for TLS clients and unencrypted for
// plain clients (see VeyonCore::isSecureTransport()).
//
// Effective behaviour per TLS/ServerMode:
// - Disabled: no sniffing, every connection is plain.
// - Optional: TLS and plain; without credentials or TLS backend plain only (warning).
// - Required: TLS only, plain clients are closed; without credentials or TLS backend every
//   connection is refused (fail closed).
//
// Users of the pending connections must report the authentication result of each connection
// with reportAuthenticationResult(): connections that are neither authenticated nor closed
// within the pre-auth timeout are closed.
class VEYON_CORE_EXPORT TlsSniffingServer : public QTcpServer
{
	Q_OBJECT
public:
	TlsSniffingServer(const TlsServerPolicy& policy, const ConnectionLimiter::Config& connectionLimits,
					  int preAuthTimeout, QObject* parent = nullptr);
	~TlsSniffingServer() override;

	const TlsServerPolicy& policy() const
	{
		return m_policy;
	}

	// TLS handshakes are possible (mode not Disabled, credentials and TLS backend available)
	bool isTlsEnabled() const
	{
		return m_tlsEnabled;
	}

	// mode Required without TLS: no connection is accepted
	bool isRefusingConnections() const
	{
		return m_refusingConnections;
	}

	int preAuthTimeout() const
	{
		return m_preAuthTimeout;
	}

	const ConnectionLimiter& connectionLimiter() const
	{
		return *m_connectionLimiter;
	}

	// authenticated: frees the pre-auth slot and stops the pre-auth timeout; failed: the
	// connection counts as failed authentication of its source address when it is closed
	void reportAuthenticationResult(QTcpSocket* socket, bool authenticated);

protected:
	void incomingConnection(qintptr socketDescriptor) override;

private:
	friend class SniffedConnection;

	void addReadyConnection(QSslSocket* socket);

	const TlsServerPolicy m_policy;
	const int m_preAuthTimeout;
	// shared with the connections, which may outlive the server (sockets of the users)
	const std::shared_ptr<ConnectionLimiter> m_connectionLimiter;
	QSslConfiguration m_sslConfiguration;
	bool m_tlsEnabled{false};
	bool m_refusingConnections{false};
};
