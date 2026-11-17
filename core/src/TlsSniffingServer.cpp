// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSslSocket>

#include "SniffedConnection.h"
#include "TlsSniffingServer.h"


TlsSniffingServer::TlsSniffingServer(const TlsServerPolicy& policy, const ConnectionLimiter::Config& connectionLimits,
									 int preAuthTimeout, QObject* parent) :
	QTcpServer(parent),
	m_policy(policy),
	m_preAuthTimeout(preAuthTimeout),
	m_connectionLimiter(std::make_shared<ConnectionLimiter>(connectionLimits))
{
	if (m_policy.mode == VeyonCore::TlsServerMode::Disabled)
	{
		vDebug() << "tls: disabled";
		return;
	}

	QString reason;
	if (VeyonCore::isTlsAvailable() == false)
	{
		reason = QStringLiteral("TLS backend not available");
	}
	else if (m_policy.hasCredentials() == false)
	{
		reason = QStringLiteral("no usable host certificate and key");
	}
	else
	{
		m_sslConfiguration = QSslConfiguration::defaultConfiguration();
		m_sslConfiguration.setProtocol(QSsl::TlsV1_3OrLater);
		m_sslConfiguration.setLocalCertificateChain(m_policy.certificateChain);
		m_sslConfiguration.setPrivateKey(m_policy.privateKey);
		// no client certificates in 4.12
		m_sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
		m_sslConfiguration.setCaCertificates({});
		m_tlsEnabled = true;
		return;
	}

	if (m_policy.mode == VeyonCore::TlsServerMode::Required)
	{
		// fail closed (05-tls.md §2)
		m_refusingConnections = true;
		vCritical() << "tls: TLS/ServerMode is Required but" << qUtf8Printable(reason) << "- refusing all connections";
	}
	else
	{
		vWarning() << "tls:" << qUtf8Printable(reason) << "- accepting plain connections only";
	}
}



TlsSniffingServer::~TlsSniffingServer()
{
	close();
}



void TlsSniffingServer::reportAuthenticationResult(QTcpSocket* socket, bool authenticated)
{
	if (socket == nullptr)
	{
		return;
	}

	const auto connection = socket->findChild<SniffedConnection*>(QString(), Qt::FindDirectChildrenOnly);
	if (connection)
	{
		connection->setAuthenticationResult(authenticated);
	}
}



void TlsSniffingServer::incomingConnection(qintptr socketDescriptor)
{
	auto socket = new QSslSocket(this);
	if (socket->setSocketDescriptor(socketDescriptor) == false)
	{
		vWarning() << "failed to set up socket:" << socket->errorString();
		delete socket;
		return;
	}

	const auto peerAddress = socket->peerAddress();

	if (m_refusingConnections || m_connectionLimiter->admit(peerAddress) == false)
	{
		vDebug() << "refusing connection from" << peerAddress.toString();
		socket->abort();
		socket->deleteLater();
		return;
	}

	auto connection = new SniffedConnection(socket, peerAddress, this, m_connectionLimiter);
	connection->start();
}



void TlsSniffingServer::addReadyConnection(QSslSocket* socket)
{
	addPendingConnection(socket);

	Q_EMIT newConnection();
}
