// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>
#include <QNetworkInterface>
#include <QProcess>
#include <QSignalSpy>
#include <QSslSocket>
#include <QStandardPaths>
#include <QtTest>

#include "MessageContext.h"
#include "TlsSniffingServer.h"
#include "TlsTestHostCredentials.h"

#include <memory>

// Relays one TCP connection to the server and holds back the client's first bytes: the first
// firstChunkSize bytes are forwarded at once, everything else only after delay (partial or
// delayed ClientHello, slowloris)
class DelayingRelay : public QObject
{
	Q_OBJECT
public:
	DelayingRelay(quint16 targetPort, int firstChunkSize, int delay) :
		m_targetPort(targetPort),
		m_firstChunkSize(firstChunkSize),
		m_delay(delay)
	{
		connect(&m_server, &QTcpServer::newConnection, this, &DelayingRelay::acceptConnection);
		(void)m_server.listen(QHostAddress::LocalHost);
	}

	quint16 port() const
	{
		return m_server.serverPort();
	}

private:
	void acceptConnection()
	{
		m_client = m_server.nextPendingConnection();
		m_upstream.connectToHost(QHostAddress::LocalHost, m_targetPort);

		connect(m_client, &QTcpSocket::readyRead, this, [this]() {
			const auto data = m_client->readAll();
			if (m_released)
			{
				m_upstream.write(data);
				return;
			}
			if (m_firstChunkSent == false)
			{
				m_firstChunkSent = true;
				m_upstream.write(data.left(m_firstChunkSize));
				m_heldBack = data.mid(m_firstChunkSize);
				QTimer::singleShot(m_delay, this, [this]() {
					m_released = true;
					m_upstream.write(m_heldBack);
				});
				return;
			}
			m_heldBack += data;
		});
		connect(&m_upstream, &QTcpSocket::readyRead, this, [this]() { m_client->write(m_upstream.readAll()); });
		connect(&m_upstream, &QTcpSocket::disconnected, m_client, &QTcpSocket::disconnectFromHost);
		connect(m_client, &QTcpSocket::disconnected, &m_upstream, &QTcpSocket::disconnectFromHost);
	}

	const quint16 m_targetPort;
	const int m_firstChunkSize;
	const int m_delay;
	QTcpServer m_server;
	QTcpSocket* m_client{nullptr};
	QTcpSocket m_upstream;
	QByteArray m_heldBack;
	bool m_firstChunkSent{false};
	bool m_released{false};
};


// TLS/plain sniffing listener (05-tls.md §1–3, §8, §10): real loopback connections with
// QSslSocket, plain sockets and openssl s_client as clients
class TlsSniffingServerTest : public QObject
{
	Q_OBJECT
private:
	static constexpr auto Banner = "RFB 003.008\n";

	struct AcceptedConnection
	{
		QPointer<QTcpSocket> socket;
		bool encrypted = false;
		bool secureTransport = false;
		qint64 elapsed = 0;
		QByteArray received;
	};

	QDateTime m_now;
	Veyon::Crypto::PrivateKey m_caKey;
	Veyon::Crypto::PrivateKey m_hostKey;
	Veyon::Crypto::Certificate m_caCertificate;
	std::optional<TlsHostCredentials> m_credentials;

	std::unique_ptr<TlsSniffingServer> m_server;
	std::vector<std::unique_ptr<AcceptedConnection>> m_accepted;
	QElapsedTimer m_connectTimer;
	bool m_writeBanner{true};

	QStringList m_logMessages;
	static QStringList* s_logMessages;

	static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_logMessages->append(message);
	}

	static ConnectionLimiter::Config defaultLimits()
	{
		return {32, 10, 6, 300};
	}

	TlsServerPolicy policy(VeyonCore::TlsServerMode mode, int sniffTimeout = 200, bool withCredentials = true) const
	{
		auto policy = TlsTestHostCredentials::policy(mode, withCredentials && m_credentials ? &*m_credentials : nullptr);
		policy.sniffTimeoutMs = sniffTimeout;
		return policy;
	}

	TlsSniffingServer& startServer(const TlsServerPolicy& policy,
								   const ConnectionLimiter::Config& limits = defaultLimits(),
								   int preAuthTimeout = 10000,
								   const QHostAddress& listenAddress = QHostAddress::LocalHost)
	{
		m_server = std::make_unique<TlsSniffingServer>(policy, limits, preAuthTimeout);
		connect(m_server.get(), &QTcpServer::newConnection, this, [this]() {
			while (m_server->hasPendingConnections())
			{
				auto connection = std::make_unique<AcceptedConnection>();
				auto socket = m_server->nextPendingConnection();
				connection->socket = socket;
				connection->encrypted = qobject_cast<QSslSocket*>(socket) && qobject_cast<QSslSocket*>(socket)->isEncrypted();
				connection->secureTransport = MessageContext(socket).isSecureTransport();
				connection->elapsed = m_connectTimer.elapsed();
				auto received = &connection->received;
				connect(socket, &QTcpSocket::readyRead, socket, [socket, received]() { *received += socket->readAll(); });
				// data that arrived before the hand-over
				*received += socket->readAll();
				if (m_writeBanner)
				{
					socket->write(Banner);
				}
				m_accepted.push_back(std::move(connection));
			}
		});
		if (m_server->listen(listenAddress) == false)
		{
			qFatal("listen failed");
		}
		m_connectTimer.start();
		return *m_server;
	}

	quint16 port() const
	{
		return m_server->serverPort();
	}

	std::unique_ptr<QSslSocket> createTlsClient(QSsl::SslProtocol protocol = QSsl::TlsV1_3OrLater)
	{
		auto client = std::make_unique<QSslSocket>();
		auto configuration = client->sslConfiguration();
		configuration.setProtocol(protocol);
		configuration.setCaCertificates({m_caCertificate.toQSslCertificate()});
		configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
		client->setSslConfiguration(configuration);
		return client;
	}

	std::unique_ptr<QSslSocket> connectTlsClient(QSsl::SslProtocol protocol = QSsl::TlsV1_3OrLater)
	{
		auto client = createTlsClient(protocol);
		m_connectTimer.restart();
		client->connectToHostEncrypted(QStringLiteral("127.0.0.1"), port());
		return client;
	}

	std::unique_ptr<QTcpSocket> connectPlainClient(const QByteArray& firstBytes = {},
												   const QHostAddress& sourceAddress = {},
												   const QHostAddress& target = QHostAddress::LocalHost)
	{
		auto client = std::make_unique<QTcpSocket>();
		if (sourceAddress.isNull() == false)
		{
			(void)client->bind(sourceAddress);
		}
		m_connectTimer.restart();
		client->connectToHost(target, port());
		if (firstBytes.isEmpty() == false)
		{
			client->write(firstBytes);
		}
		return client;
	}

	static bool waitForDisconnect(QAbstractSocket* socket, int timeout = 5000)
	{
		QElapsedTimer timer;
		timer.start();
		while (socket->state() != QAbstractSocket::UnconnectedState && timer.elapsed() < timeout)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		}
		return socket->state() == QAbstractSocket::UnconnectedState;
	}

	// all of the banner and nothing else
	static bool receivedBanner(QIODevice* device)
	{
		QByteArray received;
		QElapsedTimer timer;
		timer.start();
		while (received.size() < qsizetype(qstrlen(Banner)) && timer.elapsed() < 5000)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
			received += device->readAll();
		}
		return received == Banner;
	}

	static QHostAddress nonLoopbackAddress()
	{
		const auto addresses = QNetworkInterface::allAddresses();
		for (const auto& address : addresses)
		{
			if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false)
			{
				return address;
			}
		}
		return {};
	}

	int handshakeFailureLogCount() const
	{
		return int(std::count_if(m_logMessages.cbegin(), m_logMessages.cend(), [](const QString& message) {
			return message.contains(QLatin1String("tls: handshake failed from 127.0.0.1: "));
		}));
	}

private Q_SLOTS:
	void initTestCase()
	{
		m_now = QDateTime::currentDateTimeUtc();
		m_caKey = TlsTestHostCredentials::generateKey();
		m_hostKey = TlsTestHostCredentials::generateKey();
		m_caCertificate = PkiTestCertificates::createCa(m_caKey, QStringLiteral("Veyon Test CA"), m_now);
		const auto hostCertificate = TlsTestHostCredentials::createCertificate(m_hostKey, m_now, -60, 86400, &m_caKey,
																			   m_caCertificate.subjectCommonName());
		m_credentials = TlsHostCredentials::fromEncoded(TlsTestHostCredentials::encode(m_hostKey, {hostCertificate}), m_now);
		QVERIFY(m_credentials.has_value() || VeyonCore::isTlsAvailable() == false);
	}

	void init()
	{
		// e.g. MSYS2's Qt 6 has the Schannel backend only (spike S8)
		static const QStringList testsWithoutTls{
			QStringLiteral("requiredWithoutCredentials"),
			QStringLiteral("incompleteCredentials"),
			QStringLiteral("serverDestroyedWhileSniffing"),
			QStringLiteral("withoutTlsBackend"),
		};
		if (VeyonCore::isTlsAvailable() == false &&
			testsWithoutTls.contains(QLatin1String(QTest::currentTestFunction())) == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		m_logMessages.clear();
		s_logMessages = &m_logMessages;
		qInstallMessageHandler(captureMessage);
		m_writeBanner = true;
	}

	void cleanup()
	{
		m_accepted.clear();
		m_server.reset();
		qInstallMessageHandler(nullptr);

		// never any key material in the logs
		const auto keyDer = m_hostKey.toPkcs8Der()->toUnprotectedByteArray();
		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY(message.contains(QLatin1String("PRIVATE KEY")) == false);
			QVERIFY(message.contains(QString::fromLatin1(keyDer.toBase64().left(40))) == false);
		}
	}

	void tlsClient()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::MaximumSniffTimeout));
		QVERIFY(server.isTlsEnabled());
		QVERIFY(server.isRefusingConnections() == false);

		auto client = connectTlsClient();
		QByteArray clientReceived;
		connect(client.get(), &QSslSocket::readyRead, this, [&]() { clientReceived += client->readAll(); });

		// detected by the ClientHello, not by the sniff timeout
		QTRY_COMPARE_WITH_TIMEOUT(m_accepted.size(), size_t(1), TlsServerPolicy::MaximumSniffTimeout - 500);
		const auto& accepted = *m_accepted.front();
		QVERIFY(accepted.encrypted);
		QVERIFY(accepted.secureTransport);
		QVERIFY(VeyonCore::isSecureTransport(accepted.socket));
		QCOMPARE(qobject_cast<QSslSocket*>(accepted.socket.data())->sessionProtocol(), QSsl::TlsV1_3);

		QTRY_COMPARE(clientReceived, QByteArray(Banner));
		QVERIFY(client->isEncrypted());
		QVERIFY(client->sslHandshakeErrors().isEmpty());
		QCOMPARE(client->peerCertificate(), m_credentials->certificateChain().first());

		client->write(Banner);
		QTRY_COMPARE(accepted.received, QByteArray(Banner));
		QCOMPARE(server.connectionLimiter().preAuthCount(), 1);
	}

	// a 4.11 master waits for the banner: it gets it unencrypted after the sniff timeout
	void plainClientWaitingForBanner()
	{
		constexpr auto SniffTimeout = 300;
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, SniffTimeout));

		auto client = connectPlainClient();
		QTRY_COMPARE(m_accepted.size(), size_t(1));
		const auto& accepted = *m_accepted.front();
		QVERIFY(accepted.elapsed >= SniffTimeout - 50);
		QVERIFY(accepted.encrypted == false);
		QVERIFY(accepted.secureTransport == false);
		QVERIFY(VeyonCore::isSecureTransport(accepted.socket) == false);
		QVERIFY(qobject_cast<QSslSocket*>(accepted.socket.data()) != nullptr);

		QTRY_COMPARE(client->bytesAvailable(), qint64(qstrlen(Banner)));
		QCOMPARE(client->readAll(), QByteArray(Banner));
		client->write(Banner);
		QTRY_COMPARE(accepted.received, QByteArray(Banner));
		QCOMPARE(server.connectionLimiter().preAuthCount(), 1);
	}

	void plainClientSpeakingFirst_data()
	{
		QTest::addColumn<QByteArray>("firstBytes");

		QTest::newRow("RFB version") << QByteArray(Banner);
		QTest::newRow("RFB") << QByteArrayLiteral("RFB");
		QTest::newRow("HTTP") << QByteArrayLiteral("GET / HTTP/1.1\r\nHost: x\r\n\r\n");
		QTest::newRow("NUL bytes") << QByteArray(64, '\0');
		QTest::newRow("TLS alert record") << QByteArray::fromHex("15030300020228");
		QTest::newRow("TLS application data") << QByteArray::fromHex("1703030010") + QByteArray(16, 'x');
		QTest::newRow("SSLv2 ClientHello") << QByteArray::fromHex("802e010003000015");
	}

	void plainClientSpeakingFirst()
	{
		QFETCH(QByteArray, firstBytes);

		startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::MaximumSniffTimeout));
		auto client = connectPlainClient(firstBytes);

		// decided by the bytes, long before the sniff timeout; nothing consumed by the sniffing
		QTRY_COMPARE_WITH_TIMEOUT(m_accepted.size(), size_t(1), TlsServerPolicy::MaximumSniffTimeout - 500);
		QVERIFY(m_accepted.front()->encrypted == false);
		QTRY_COMPARE(m_accepted.front()->received, firstBytes);
		QVERIFY(receivedBanner(client.get()));
	}

	void singleByteThenDelay()
	{
		constexpr auto SniffTimeout = 300;
		startServer(policy(VeyonCore::TlsServerMode::Optional, SniffTimeout));

		auto client = connectPlainClient(QByteArray::fromHex("16"));
		QTRY_COMPARE(m_accepted.size(), size_t(1));
		QVERIFY(m_accepted.front()->elapsed >= SniffTimeout - 50);
		QVERIFY(m_accepted.front()->encrypted == false);
		QTRY_COMPARE(m_accepted.front()->received, QByteArray::fromHex("16"));
	}

	// ClientHello arriving in pieces within the sniff window
	void partialClientHello_data()
	{
		QTest::addColumn<int>("firstChunkSize");

		QTest::newRow("1 byte") << 1;
		QTest::newRow("0 bytes") << 0;
		QTest::newRow("2 bytes") << 2;
		QTest::newRow("5 bytes (record header)") << 5;
	}

	void partialClientHello()
	{
		QFETCH(int, firstChunkSize);

		startServer(policy(VeyonCore::TlsServerMode::Required, 1000));
		DelayingRelay relay(port(), firstChunkSize, 300);

		auto client = createTlsClient();
		QByteArray clientReceived;
		connect(client.get(), &QSslSocket::readyRead, this, [&]() { clientReceived += client->readAll(); });
		client->connectToHostEncrypted(QStringLiteral("127.0.0.1"), relay.port());

		QTRY_COMPARE(m_accepted.size(), size_t(1));
		QVERIFY(m_accepted.front()->encrypted);
		QTRY_COMPARE(clientReceived, QByteArray(Banner));
		QVERIFY(client->isEncrypted());
	}

	// a ClientHello after the sniff window (slowloris, or a very slow client) is treated as plain
	// and the handshake fails - never a hang
	void delayedClientHello_data()
	{
		QTest::addColumn<VeyonCore::TlsServerMode>("mode");

		QTest::newRow("Optional") << VeyonCore::TlsServerMode::Optional;
		QTest::newRow("Required") << VeyonCore::TlsServerMode::Required;
	}

	void delayedClientHello()
	{
		QFETCH(VeyonCore::TlsServerMode, mode);

		constexpr auto SniffTimeout = 100;
		auto& server = startServer(policy(mode, SniffTimeout));
		DelayingRelay relay(port(), 0, SniffTimeout * 4);

		auto client = createTlsClient();
		QSignalSpy errorSpy(client.get(), &QAbstractSocket::errorOccurred);
		client->connectToHostEncrypted(QStringLiteral("127.0.0.1"), relay.port());

		QVERIFY(waitForDisconnect(client.get()));
		QVERIFY(client->isEncrypted() == false);
		QVERIFY(errorSpy.isEmpty() == false);

		if (mode == VeyonCore::TlsServerMode::Optional)
		{
			// handed over as plain connection
			QCOMPARE(m_accepted.size(), size_t(1));
			QVERIFY(m_accepted.front()->encrypted == false);
		}
		else
		{
			QVERIFY(m_accepted.empty());
			QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);
		}
	}

	void requiredMode()
	{
		constexpr auto SniffTimeout = 200;
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Required, SniffTimeout));
		QVERIFY(server.isTlsEnabled());

		// plain client waiting for the banner: closed at the sniff timeout without any byte
		auto waitingClient = connectPlainClient();
		QVERIFY(waitForDisconnect(waitingClient.get()));
		QVERIFY(m_connectTimer.elapsed() >= SniffTimeout - 50);
		QCOMPARE(waitingClient->readAll(), QByteArray());

		// plain client speaking first: closed at once
		auto speakingClient = connectPlainClient(QByteArray(Banner));
		QVERIFY(waitForDisconnect(speakingClient.get(), 1000));
		QCOMPARE(speakingClient->readAll(), QByteArray());
		QVERIFY(m_accepted.empty());
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);

		// TLS client
		auto client = connectTlsClient();
		QTRY_COMPARE(m_accepted.size(), size_t(1));
		QVERIFY(m_accepted.front()->encrypted);
		QTRY_VERIFY(client->isEncrypted());

		// not logged as handshake failures
		QCOMPARE(handshakeFailureLogCount(), 0);
	}

	void disabledMode()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Disabled, TlsServerPolicy::MaximumSniffTimeout));
		QVERIFY(server.isTlsEnabled() == false);
		QVERIFY(server.isRefusingConnections() == false);

		// no sniffing delay
		auto plainClient = connectPlainClient();
		QTRY_COMPARE_WITH_TIMEOUT(m_accepted.size(), size_t(1), TlsServerPolicy::MaximumSniffTimeout - 500);
		QVERIFY(m_accepted.front()->encrypted == false);
		QVERIFY(receivedBanner(plainClient.get()));

		// a TLS client gets the RFB banner instead of a ServerHello (master detects a plain peer)
		auto client = connectTlsClient();
		QSignalSpy errorSpy(client.get(), &QAbstractSocket::errorOccurred);
		QTRY_COMPARE(m_accepted.size(), size_t(2));
		QVERIFY(m_accepted.back()->encrypted == false);
		QTRY_VERIFY(errorSpy.isEmpty() == false);
		QCOMPARE(client->error(), QAbstractSocket::SslHandshakeFailedError);
		QVERIFY(client->isEncrypted() == false);
	}

	// Optional without host certificate (or TLS backend): plain only, like Disabled, so that
	// TLS clients detect a plain peer
	void optionalWithoutCredentials()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::MaximumSniffTimeout, false));
		QVERIFY(server.isTlsEnabled() == false);
		QVERIFY(server.isRefusingConnections() == false);
		QVERIFY2(m_logMessages.join(QLatin1Char('\n')).contains(QLatin1String("tls: no usable host certificate and key - accepting plain connections only")),
				 qPrintable(m_logMessages.join(QLatin1Char('\n'))));

		auto plainClient = connectPlainClient();
		QTRY_COMPARE_WITH_TIMEOUT(m_accepted.size(), size_t(1), TlsServerPolicy::MaximumSniffTimeout - 500);
		QVERIFY(receivedBanner(plainClient.get()));

		auto client = connectTlsClient();
		QSignalSpy errorSpy(client.get(), &QAbstractSocket::errorOccurred);
		QTRY_VERIFY(errorSpy.isEmpty() == false);
		QCOMPARE(client->error(), QAbstractSocket::SslHandshakeFailedError);
	}

	// Required without host certificate: fail closed
	void requiredWithoutCredentials()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Required, 200, false));
		QVERIFY(server.isTlsEnabled() == false);
		QVERIFY(server.isRefusingConnections());
		QVERIFY2(m_logMessages.join(QLatin1Char('\n')).contains(QLatin1String("tls: TLS/ServerMode is Required but no usable host certificate and key - refusing all connections")),
				 qPrintable(m_logMessages.join(QLatin1Char('\n'))));

		auto plainClient = connectPlainClient();
		QVERIFY(waitForDisconnect(plainClient.get(), 1000));
		auto client = connectTlsClient();
		QVERIFY(waitForDisconnect(client.get(), 1000));
		QVERIFY(client->isEncrypted() == false);

		QTest::qWait(300);
		QVERIFY(m_accepted.empty());
		QCOMPARE(server.connectionLimiter().preAuthCount(), 0);
	}

	// without TLS backend Optional serves plain connections only and Required refuses all
	void withoutTlsBackend()
	{
		if (VeyonCore::isTlsAvailable())
		{
			QSKIP("Qt OpenSSL TLS backend available");
		}

		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::MaximumSniffTimeout));
		QVERIFY(server.isTlsEnabled() == false);
		QVERIFY(server.isRefusingConnections() == false);
		auto plainClient = connectPlainClient();
		QTRY_COMPARE_WITH_TIMEOUT(m_accepted.size(), size_t(1), TlsServerPolicy::MaximumSniffTimeout - 500);
		QVERIFY(receivedBanner(plainClient.get()));

		auto& requiredServer = startServer(policy(VeyonCore::TlsServerMode::Required));
		QVERIFY(requiredServer.isRefusingConnections());
		auto refusedClient = connectPlainClient();
		QVERIFY(waitForDisconnect(refusedClient.get(), 1000));
	}

	// a policy with credentials whose key is not usable is never taken for TLS
	void incompleteCredentials()
	{
		auto withoutKey = policy(VeyonCore::TlsServerMode::Required);
		withoutKey.privateKey = {};
		QVERIFY(withoutKey.hasCredentials() == false);
		TlsSniffingServer server(withoutKey, defaultLimits(), 1000);
		QVERIFY(server.isRefusingConnections());

		auto withoutCertificate = policy(VeyonCore::TlsServerMode::Optional);
		withoutCertificate.certificateChain = {QSslCertificate()};
		QVERIFY(withoutCertificate.hasCredentials() == false);
		TlsSniffingServer optionalServer(withoutCertificate, defaultLimits(), 1000);
		QVERIFY(optionalServer.isTlsEnabled() == false);
		QVERIFY(optionalServer.isRefusingConnections() == false);
	}

	void handshakeFailure_data()
	{
		QTest::addColumn<QByteArray>("bytes");
		QTest::addColumn<QByteArray>("payloadMarker");

		QTest::newRow("garbage handshake record") << QByteArray::fromHex("1603010010") + QByteArray(16, 'Z') << QByteArrayLiteral("ZZZZ");
		QTest::newRow("oversized record") << QByteArray::fromHex("1603017fff") + QByteArrayLiteral("SECRETMARKER") << QByteArrayLiteral("SECRETMARKER");
		QTest::newRow("garbage after header") << QByteArray::fromHex("1603") + QByteArrayLiteral("payload-bytes-XYZ") << QByteArrayLiteral("XYZ");
	}

	void handshakeFailure()
	{
		QFETCH(QByteArray, bytes);
		QFETCH(QByteArray, payloadMarker);

		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, 200));
		auto client = connectPlainClient(bytes);
		QVERIFY(waitForDisconnect(client.get()));

		QTRY_COMPARE(handshakeFailureLogCount(), 1);
		QVERIFY(m_accepted.empty());
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);
		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY(message.contains(QString::fromLatin1(payloadMarker)) == false);
		}
	}

	// "tls: handshake failed from <ip>: <reason>" also with IPv4 peers of a dual-stack listener
	void handshakeFailureOnDualStackListener()
	{
		startServer(policy(VeyonCore::TlsServerMode::Optional, 200), defaultLimits(), 10000, QHostAddress::Any);
		auto client = connectPlainClient(QByteArray::fromHex("1603010010") + QByteArray(16, 'Z'));
		QVERIFY(waitForDisconnect(client.get()));
		QTRY_COMPARE(handshakeFailureLogCount(), 1);
	}

	// TLS 1.3 only (05-tls.md §1.3)
	void tls12ClientRejected()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Required, 200));
		auto client = connectTlsClient(QSsl::TlsV1_2);
		QVERIFY(waitForDisconnect(client.get()));
		QVERIFY(client->isEncrypted() == false);
		QTRY_COMPARE(handshakeFailureLogCount(), 1);
		QVERIFY(m_accepted.empty());
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);
	}

	void clientAbortsHandshake()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Required, 200));
		auto client = connectPlainClient(QByteArray::fromHex("160301"));
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 1);
		QTest::qWait(50);
		client->abort();

		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);
		QVERIFY(m_accepted.empty());
		QTRY_COMPARE(handshakeFailureLogCount(), 1);
	}

	void handshakeTimeout()
	{
		auto stalling = policy(VeyonCore::TlsServerMode::Optional, 200);
		stalling.handshakeTimeoutMs = 300;
		auto& server = startServer(stalling);

		// record header of a ClientHello, then nothing
		auto client = connectPlainClient(QByteArray::fromHex("160301"));
		QVERIFY(waitForDisconnect(client.get(), 3000));
		QVERIFY(m_connectTimer.elapsed() >= 250);
		QVERIFY(m_logMessages.last().contains(QLatin1String("tls: handshake failed from 127.0.0.1: timeout")));
		QVERIFY(m_accepted.empty());
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);
	}

	void preAuthTimeout()
	{
		constexpr auto PreAuthTimeout = 500;
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, 100), defaultLimits(), PreAuthTimeout);
		QCOMPARE(server.preAuthTimeout(), PreAuthTimeout);

		// handed over but never authenticated: closed after the pre-auth timeout
		auto client = connectPlainClient();
		QTRY_COMPARE(m_accepted.size(), size_t(1));
		QSignalSpy disconnectedSpy(m_accepted.front()->socket.data(), &QAbstractSocket::disconnected);
		QVERIFY(waitForDisconnect(client.get(), 3000));
		QVERIFY(m_connectTimer.elapsed() >= PreAuthTimeout - 50);
		QCOMPARE(disconnectedSpy.count(), 1);
		QVERIFY(m_logMessages.last().contains(QLatin1String("after pre-auth timeout")));

		// the user of the socket deletes it, which frees the slot
		QCOMPARE(server.connectionLimiter().preAuthCount(), 1);
		delete m_accepted.front()->socket;
		QCOMPARE(server.connectionLimiter().preAuthCount(), 0);

		// still sniffing/handshaking when the pre-auth timeout expires
		auto stallingServerPolicy = policy(VeyonCore::TlsServerMode::Required, 100);
		stallingServerPolicy.handshakeTimeoutMs = 10000;
		auto& stallingServer = startServer(stallingServerPolicy, defaultLimits(), PreAuthTimeout);
		auto stallingClient = connectPlainClient(QByteArray::fromHex("1603"));
		QVERIFY(waitForDisconnect(stallingClient.get(), 3000));
		QVERIFY(m_logMessages.last().contains(QLatin1String("after pre-auth timeout")));
		QTRY_COMPARE(stallingServer.connectionLimiter().preAuthCount(), 0);
	}

	void authenticatedConnectionStaysOpen()
	{
		constexpr auto PreAuthTimeout = 300;
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, 100), defaultLimits(), PreAuthTimeout);

		auto client = connectTlsClient();
		QTRY_COMPARE(m_accepted.size(), size_t(1));
		QCOMPARE(server.connectionLimiter().preAuthCount(), 1);

		auto socket = m_accepted.front()->socket;
		server.reportAuthenticationResult(socket, true);
		QCOMPARE(server.connectionLimiter().preAuthCount(), 0);
		// reported twice and other sockets: no effect
		server.reportAuthenticationResult(socket, true);
		server.reportAuthenticationResult(socket, false);
		server.reportAuthenticationResult(nullptr, true);
		QTcpSocket unrelatedSocket;
		server.reportAuthenticationResult(&unrelatedSocket, true);
		QCOMPARE(server.connectionLimiter().preAuthCount(), 0);

		QTest::qWait(PreAuthTimeout * 2);
		QCOMPARE(client->state(), QAbstractSocket::ConnectedState);
		QVERIFY(socket.isNull() == false);
		QCOMPARE(socket->state(), QAbstractSocket::ConnectedState);

		delete socket;
		QCOMPARE(server.connectionLimiter().preAuthCount(), 0);
	}

	// ConnectionLimiter::admit() first: the global pre-auth cap applies to loopback as well
	void preAuthLimit()
	{
		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, 100), {2, 10, 6, 300});

		auto first = connectTlsClient();
		auto second = connectPlainClient();
		QTRY_COMPARE(m_accepted.size(), size_t(2));
		QCOMPARE(server.connectionLimiter().preAuthCount(), 2);

		auto refused = connectPlainClient();
		QVERIFY(waitForDisconnect(refused.get(), 2000));
		QCOMPARE(refused->readAll(), QByteArray());
		QCOMPARE(m_accepted.size(), size_t(2));

		// authenticated connections free their slot
		server.reportAuthenticationResult(m_accepted.front()->socket, true);
		auto admitted = connectPlainClient();
		QTRY_COMPARE(m_accepted.size(), size_t(3));
		QVERIFY(receivedBanner(admitted.get()));
	}

	// handshake failures and failed authentications count toward the per-IP backoff (05-tls.md §3)
	void failureBackoff()
	{
		const auto address = nonLoopbackAddress();
		if (address.isNull())
		{
			QSKIP("no non-loopback IPv4 address");
		}

		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, 100), {32, 10, 6, 300}, 10000, QHostAddress::AnyIPv4);

		// handshake failure ⇒ next connection from that address refused (1 s backoff)
		auto failing = connectPlainClient(QByteArray::fromHex("1603010010") + QByteArray(16, 'Z'), address, address);
		QVERIFY(waitForDisconnect(failing.get()));
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);

		auto refused = connectPlainClient({}, address, address);
		QVERIFY(waitForDisconnect(refused.get(), 2000));
		QVERIFY(m_accepted.empty());

		// loopback is not affected
		auto loopbackClient = connectTlsClient();
		QTRY_COMPARE(m_accepted.size(), size_t(1));

		// after the backoff: failed authentication reported by the user of the socket
		QTest::qWait(1100);
		auto client = connectPlainClient({}, address, address);
		QTRY_COMPARE(m_accepted.size(), size_t(2));
		server.reportAuthenticationResult(m_accepted.back()->socket, false);
		delete m_accepted.back()->socket;

		auto refusedAgain = connectPlainClient({}, address, address);
		QVERIFY(waitForDisconnect(refusedAgain.get(), 2000));
		QCOMPARE(m_accepted.size(), size_t(2));
	}

	// silent connections from one address don't delay connections from others (05-tls.md §10)
	void slowloris()
	{
		const auto address = nonLoopbackAddress();
		if (address.isNull())
		{
			QSKIP("no non-loopback IPv4 address");
		}

		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::DefaultSniffTimeout),
								   defaultLimits(), 10000, QHostAddress::AnyIPv4);
		m_writeBanner = false;

		std::vector<std::unique_ptr<QTcpSocket>> attackers;
		for (int i = 0; i < 100; ++i)
		{
			attackers.push_back(connectPlainClient({}, address, address));
		}
		QTRY_COMPARE(server.connectionLimiter().preAuthCount(), defaultLimits().perIpBurst);
		QTest::qWait(100);
		QCOMPARE(server.connectionLimiter().preAuthCount(), defaultLimits().perIpBurst);

		m_writeBanner = true;
		QElapsedTimer timer;
		timer.start();
		auto client = connectTlsClient();
		QTRY_VERIFY_WITH_TIMEOUT(client->isEncrypted(), 3000);
		QVERIFY(receivedBanner(client.get()));
		QVERIFY2(timer.elapsed() < 2000, qPrintable(QString::number(timer.elapsed())));
	}

	// connections that are still sniffing when the server goes away
	void serverDestroyedWhileSniffing()
	{
		startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::MaximumSniffTimeout));
		auto first = connectPlainClient();
		auto second = connectPlainClient(QByteArray::fromHex("1603"));
		QTRY_COMPARE(m_server->connectionLimiter().preAuthCount(), 2);

		m_server.reset();
		QVERIFY(waitForDisconnect(first.get()));
		QVERIFY(waitForDisconnect(second.get()));
	}

	void isSecureTransport()
	{
		QVERIFY(VeyonCore::isSecureTransport(nullptr) == false);
		QVERIFY(MessageContext().isSecureTransport() == false);

		QBuffer buffer;
		QVERIFY(VeyonCore::isSecureTransport(&buffer) == false);
		QVERIFY(MessageContext(&buffer).isSecureTransport() == false);

		QTcpSocket tcpSocket;
		QVERIFY(VeyonCore::isSecureTransport(&tcpSocket) == false);

		QSslSocket unencryptedSocket;
		QVERIFY(VeyonCore::isSecureTransport(&unencryptedSocket) == false);

		// a handed-over TLS connection
		startServer(policy(VeyonCore::TlsServerMode::Required, 200));
		auto client = connectTlsClient();
		QTRY_COMPARE(m_accepted.size(), size_t(1));
		auto socket = m_accepted.front()->socket;
		QVERIFY(VeyonCore::isSecureTransport(socket));
		QVERIFY(MessageContext(socket).isSecureTransport());
		QVERIFY(client->isEncrypted());
	}

	// interoperability with a TLS stack other than Qt's client code
	void opensslClient_data()
	{
		QTest::addColumn<QString>("protocolOption");
		QTest::addColumn<bool>("success");

		QTest::newRow("TLS 1.3") << QStringLiteral("-tls1_3") << true;
		QTest::newRow("TLS 1.2") << QStringLiteral("-tls1_2") << false;
	}

	void opensslClient()
	{
		QFETCH(QString, protocolOption);
		QFETCH(bool, success);

		const auto openssl = QStandardPaths::findExecutable(QStringLiteral("openssl"));
		if (openssl.isEmpty())
		{
			QSKIP("openssl not found");
		}

		auto& server = startServer(policy(VeyonCore::TlsServerMode::Optional, TlsServerPolicy::MaximumSniffTimeout));

		QProcess process;
		process.start(openssl, {QStringLiteral("s_client"), QStringLiteral("-connect"), QStringLiteral("127.0.0.1:%1").arg(port()), protocolOption, QStringLiteral("-quiet"), QStringLiteral("-ign_eof")});
		QVERIFY(process.waitForStarted());

		if (success)
		{
			QTRY_COMPARE_WITH_TIMEOUT(m_accepted.size(), size_t(1), 5000);
			QVERIFY(m_accepted.front()->encrypted);
			QVERIFY(receivedBanner(&process));
			process.write(Banner);
			QTRY_COMPARE(m_accepted.front()->received, QByteArray(Banner));
		}
		else
		{
			QTRY_COMPARE_WITH_TIMEOUT(handshakeFailureLogCount(), 1, 5000);
			QVERIFY(m_accepted.empty());
			QTRY_COMPARE(server.connectionLimiter().preAuthCount(), 0);
		}

		process.kill();
		process.waitForFinished();
	}
};

QStringList* TlsSniffingServerTest::s_logMessages = nullptr;


QTEST_GUILESS_MAIN(TlsSniffingServerTest)
#include "TlsSniffingServerTest.moc"
