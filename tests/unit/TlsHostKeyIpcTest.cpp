// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSslSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "IpcClient.h"
#include "IpcConnection.h"
#include "IpcServer.h"
#include "PlatformSecretFunctions.h"
#include "SecretStore.h"
#include "ServiceIpcCommandHandler.h"
#include "TlsSniffingServer.h"
#include "TlsTestHostCredentials.h"
#include "VeyonConfiguration.h"
#include "pki/TlsConfiguration.h"

#include <map>
#include <memory>
#include <unistd.h>

using Veyon::Crypto::SecureBuffer;

class HostKeySecretFunctions : public PlatformSecretFunctions
{
public:
	std::optional<SecureBuffer> readSecret(const QString& key) override
	{
		const auto it = m_secrets.find(key);
		if (it == m_secrets.end())
		{
			return {};
		}
		return it->second.clone();
	}

	bool writeSecret(const QString& key, const SecureBuffer& value) override
	{
		m_secrets[key] = value.clone();
		return true;
	}

	bool removeSecret(const QString& key) override
	{
		m_secrets.erase(key);
		return true;
	}

	QStringList secretKeys() override
	{
		return {};
	}

	bool isAvailable() override
	{
		return true;
	}

	bool prepareStorage() override
	{
		return true;
	}

private:
	std::map<QString, SecureBuffer> m_secrets;
};


// The host key from the service's SecretStore and certificate file to a session server's
// TlsSniffingServer, through a real IPC endpoint (02-ipc.md §6 GetTlsHostKey, 05-tls.md §1.3, §2)
class TlsHostKeyIpcTest : public QObject
{
	Q_OBJECT
private:
	static constexpr quint32 SessionId = 3;

	QDateTime m_now;
	Veyon::Crypto::PrivateKey m_hostKey;
	Veyon::Crypto::Certificate m_certificate;
	HostKeySecretFunctions m_secrets;
	QTemporaryDir m_directory;
	std::unique_ptr<Configuration::JsonStore> m_store;
	std::unique_ptr<VeyonConfiguration> m_configuration;

	IpcServer::PeerPolicy ownPolicy() const
	{
		IpcServer::PeerPolicy policy;
		policy.userId = qint64(getuid());
		policy.checkProcessId = true;
		return policy;
	}

	// GetTlsHostKey as answered by ServiceDataManager: TLS not disabled, checked credentials
	std::optional<ServiceIpcCommandHandler::TlsHostKey> loadHostKey() const
	{
		if (TlsConfiguration::serverMode(*m_configuration) == VeyonCore::TlsServerMode::Disabled)
		{
			return {};
		}
		auto credentials = TlsHostCredentials::load(*m_configuration, m_now);
		if (credentials.has_value() == false)
		{
			return {};
		}
		return ServiceIpcCommandHandler::TlsHostKey{std::move(credentials->privateKeyDer), credentials->certificateChainPem};
	}

	// one server start: endpoint, connection of the "session server" and its GetTlsHostKey call
	std::optional<TlsHostCredentials> fetch(bool tlsHostKeyAllowed, int calls = 1)
	{
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		if (server.listen(m_directory.path(), ownPolicy()) == false)
		{
			return {};
		}
		server.setExpectedProcessId(QCoreApplication::applicationPid());

		ServiceIpcCommandHandler handler;
		handler.setTlsHostKeyProvider([this]() { return loadHostKey(); });
		ServiceIpcCommandHandler::Client client;
		client.sessionId = int(SessionId);
		client.hasLoggedOnUser = []() { return true; };
		client.tlsHostKeyAllowed = tlsHostKeyAllowed;

		connect(&server, &IpcServer::connectionEstablished, &server, [&handler, &client](IpcConnection* connection) {
			connect(connection, &IpcConnection::messageReceived, connection, [&handler, &client, connection](const QByteArray& message) {
				const auto request = IpcMessage::decodeRequest(message);
				if (request.has_value())
				{
					(void)connection->sendMessage(IpcMessage::encodeResponse(handler.handleRequest(client, *request)));
				}
			});
		});

		auto line = server.credentialsLine();
		(void)line.resize(line.size() - 1);
		auto credentials = std::make_shared<IpcEndpointCredentials>(*IpcEndpointCredentials::parse(line));

		std::optional<TlsHostCredentials> result;
		const auto directory = m_directory.path();
		std::unique_ptr<QThread> thread(QThread::create([&result, credentials, directory, calls]() {
			IpcClient ipcClient(IpcProtocol::Role::SessionServer, SessionId);
			if (ipcClient.connectAndHandshake(directory, std::move(*credentials)))
			{
				for (int i = 0; i < calls; ++i)
				{
					result = TlsHostCredentials::fetchFromService(&ipcClient);
				}
			}
		}));
		thread->start();
		while (thread->isFinished() == false)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		}
		thread->wait();

		return result;
	}

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_directory.isValid());
		m_now = QDateTime::currentDateTimeUtc();
		m_hostKey = TlsTestHostCredentials::generateKey();
		m_certificate = TlsTestHostCredentials::createCertificate(m_hostKey, m_now);

		QFile certificateFile(m_directory.filePath(QStringLiteral("host-cert.pem")));
		QVERIFY(certificateFile.open(QFile::WriteOnly));
		certificateFile.write(m_certificate.toPem());
		certificateFile.close();

		m_store = std::make_unique<Configuration::JsonStore>(Configuration::Store::Scope::System,
															 m_directory.filePath(QStringLiteral("config.json")));
		m_configuration = std::make_unique<VeyonConfiguration>(m_store.get());
		m_configuration->setTlsHostCertificateFile(certificateFile.fileName());
	}

	void init()
	{
		SecretStore::setTestBackend(&m_secrets, VeyonCore::InitializationFlag::SecretStore);
		QVERIFY(m_secrets.writeSecret(TlsHostCredentials::PrivateKeySecret, *m_hostKey.toPkcs8Der()));
		m_configuration->setTlsServerMode(VeyonCore::TlsServerMode::Optional);
	}

	void cleanup()
	{
		SecretStore::setTestBackend(nullptr, VeyonCore::InitializationFlags());
	}

	void hostKeyReachesTheServer()
	{
		const auto credentials = fetch(true);
		QVERIFY(credentials.has_value());
		QCOMPARE(credentials->certificateChain().first().toDer(), m_certificate.toDer());
		QCOMPARE(credentials->privateKey().algorithm(), QSsl::Ec);

		// usable for the listener: TLS handshake with the fetched credentials
		TlsServerPolicy policy;
		policy.mode = VeyonCore::TlsServerMode::Required;
		policy.certificateChain = credentials->certificateChain();
		policy.privateKey = credentials->privateKey();
		TlsSniffingServer server(policy, {32, 10, 6, 300}, 10000);
		QVERIFY(server.isTlsEnabled());
		QVERIFY(server.listen(QHostAddress::LocalHost));

		QSslSocket client;
		auto configuration = client.sslConfiguration();
		configuration.setCaCertificates({m_certificate.toQSslCertificate()});
		client.setSslConfiguration(configuration);
		client.connectToHostEncrypted(QStringLiteral("127.0.0.1"), server.serverPort());
		QTRY_VERIFY(client.isEncrypted());
		QTRY_VERIFY(server.hasPendingConnections());
		QCOMPARE(client.peerCertificate().toDer(), m_certificate.toDer());
	}

	// a server whose memory other processes may read gets no key (T-PROC-02)
	void notAllowed()
	{
		QVERIFY(fetch(false).has_value() == false);
	}

	void disabled()
	{
		m_configuration->setTlsServerMode(VeyonCore::TlsServerMode::Disabled);
		QVERIFY(fetch(true).has_value() == false);
	}

	void noHostKey()
	{
		QVERIFY(m_secrets.removeSecret(TlsHostCredentials::PrivateKeySecret));
		QVERIFY(fetch(true).has_value() == false);
	}

	// at most two calls per server start (02-ipc.md §6)
	void requestLimit()
	{
		QVERIFY(fetch(true, 2).has_value());
		QVERIFY(fetch(true, 3).has_value() == false);
	}
};


QTEST_GUILESS_MAIN(TlsHostKeyIpcTest)
#include "TlsHostKeyIpcTest.moc"
