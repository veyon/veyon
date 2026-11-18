// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMap>
#include <QSslSocket>
#include <QTcpServer>
#include <QThread>
#include <QtTest>

#include "MessageContext.h"
#include "TlsTestHostCredentials.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "VncClientProtocol.h"
#include "VncProxyConnection.h"
#include "VncProxyConnectionFactory.h"
#include "VncProxyServer.h"
#include "VncServerClient.h"
#include "VncServerProtocol.h"

#include <functional>
#include <memory>

namespace {

constexpr auto ValidToken = "valid-token";

// Veyon server protocol with token authentication only, standing in for VeyonServerProtocol and
// ServerAuthenticationManager (the auth types are WP5's business, the transport is WP6's)
class TestServerProtocol : public VncServerProtocol
{
public:
	using AuthenticationHandler = std::function<void(bool authenticated, bool secureTransport)>;

	TestServerProtocol(QTcpSocket* socket, VncServerClient* client, const AuthenticationHandler& handler) :
		VncServerProtocol(socket, client),
		m_handler(handler)
	{
	}

protected:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override
	{
		return {RfbVeyonAuth::Token};
	}

	void processAuthenticationMessage(VariantArrayMessage& message) override
	{
		if (client()->authState() == VncServerClient::AuthState::Init)
		{
			client()->setAuthState(VncServerClient::AuthState::Token);
			return;
		}

		const auto authenticated = message.read().toByteArray() == ValidToken;
		client()->setAuthState(authenticated ? VncServerClient::AuthState::Successful : VncServerClient::AuthState::Failed);
		m_handler(authenticated, MessageContext(socket()).isSecureTransport());
	}

	void performAccessControl() override
	{
		client()->setAccessControlState(VncServerClient::AccessControlState::Successful);
	}

private:
	const AuthenticationHandler m_handler;
};


class TestProxyConnection : public VncProxyConnection
{
	Q_OBJECT
public:
	using AuthenticationHandler = std::function<void(VncProxyConnection* connection, bool authenticated, bool secureTransport)>;

	TestProxyConnection(QTcpSocket* clientSocket, const VncServerEndpoint& vncServerEndpoint,
						const AuthenticationHandler& handler,
						QObject* parent) :
		VncProxyConnection(clientSocket, vncServerEndpoint, parent),
		m_serverProtocol(clientSocket, &m_serverClient,
						 [this, handler](bool authenticated, bool secureTransport) {
							 handler(this, authenticated, secureTransport);
						 }),
		m_clientProtocol(vncServerSocket(), {})
	{
	}

protected:
	VncClientProtocol& clientProtocol() override
	{
		return m_clientProtocol;
	}

	VncServerProtocol& serverProtocol() override
	{
		return m_serverProtocol;
	}

private:
	VncServerClient m_serverClient;
	TestServerProtocol m_serverProtocol;
	VncClientProtocol m_clientProtocol;
};

}


// VncProxyServer behind TlsSniffingServer with the real RFB/Veyon server protocol state machine
// (05-tls.md §1.5, §10): server mode × client transport, a 4.11-style plain client, TLS clients,
// and the authentication result reaching the connection limiter
class VncProxyServerTlsTest : public QObject, public VncProxyConnectionFactory
{
	Q_OBJECT
private:
	struct ClientResult
	{
		bool connected = false;
		bool encrypted = false;
		QByteArray banner;
		bool authenticated = false;
		bool connectedAfterHold = false;
	};

	struct ClientBehaviour
	{
		QByteArray token = ValidToken;
		// stop after receiving the banner
		bool stopAfterBanner = false;
		// send the protocol version before receiving the banner (scanners, non-conforming clients)
		bool sendVersionFirst = false;
		// keep the connection for this long at the end and check whether it is still open
		int holdTime = 0;
	};

	struct AuthenticationResult
	{
		bool authenticated = false;
		bool secureTransport = false;
	};

	QDateTime m_now;
	Veyon::Crypto::PrivateKey m_caKey;
	Veyon::Crypto::PrivateKey m_hostKey;
	Veyon::Crypto::Certificate m_caCertificate;
	std::optional<TlsHostCredentials> m_credentials;

	std::unique_ptr<VncProxyServer> m_proxyServer;
	QList<AuthenticationResult> m_authenticationResults;
	quint16 m_port = 0;

	VncProxyConnection* createVncProxyConnection(QTcpSocket* clientSocket, const VncServerEndpoint& vncServerEndpoint,
												 const Password& vncServerPassword, QObject* parent) override
	{
		Q_UNUSED(vncServerPassword)

		// report like ComputerControlServer does
		return new TestProxyConnection(clientSocket, vncServerEndpoint,
									   [this](VncProxyConnection* connection, bool authenticated, bool secureTransport) {
			m_authenticationResults.append({authenticated, secureTransport});
			m_proxyServer->reportAuthenticationResult(connection, authenticated);
		}, parent);
	}

	// first free port from a temporary listener
	static quint16 freePort()
	{
		QTcpServer server;
		(void)server.listen(QHostAddress::LocalHost);
		return server.serverPort();
	}

	// the VNC server is never reached: the clients stop after the authentication
	static VncServerEndpoint unusedVncServerEndpoint()
	{
		VncServerEndpoint endpoint;
		endpoint.port = freePort();
		return endpoint;
	}

	bool startProxyServer(const TlsServerPolicy& policy)
	{
		m_authenticationResults.clear();
		m_proxyServer.reset();
		m_port = freePort();
		m_proxyServer = std::make_unique<VncProxyServer>(QHostAddress::LocalHost, m_port, this);
		return m_proxyServer->start(unusedVncServerEndpoint(), {}, policy);
	}

	TlsServerPolicy policy(VeyonCore::TlsServerMode mode, bool withCredentials = true) const
	{
		auto policy = TlsTestHostCredentials::policy(mode, withCredentials && m_credentials ? &*m_credentials : nullptr);
		policy.sniffTimeoutMs = 100;
		return policy;
	}

	static bool waitForBytes(QTcpSocket& socket, qint64 count)
	{
		QElapsedTimer timer;
		timer.start();
		while (socket.bytesAvailable() < count && timer.elapsed() < 5000)
		{
			if (socket.state() != QAbstractSocket::ConnectedState && socket.bytesAvailable() < count)
			{
				return false;
			}
			socket.waitForReadyRead(100);
		}
		return socket.bytesAvailable() >= count;
	}

	static bool receiveMessage(QTcpSocket& socket, VariantArrayMessage& message)
	{
		QElapsedTimer timer;
		timer.start();
		while (message.isReadyForReceive() == false && timer.elapsed() < 5000 &&
			   socket.state() == QAbstractSocket::ConnectedState)
		{
			socket.waitForReadyRead(100);
		}
		return message.isReadyForReceive() && message.receive();
	}

	// a Veyon master's side of the RFB handshake and token authentication (as in
	// VeyonConnection.cpp), over plain TCP like a 4.11 master or over TLS like a 4.12 tunnel
	static void runClient(quint16 port, bool tls, const QSslCertificate& caCertificate, const ClientBehaviour& behaviour,
						  ClientResult& result)
	{
		std::unique_ptr<QTcpSocket> socket;
		runHandshake(socket, port, tls, caCertificate, behaviour, result);

		if (socket && behaviour.holdTime > 0)
		{
			QElapsedTimer timer;
			timer.start();
			while (timer.elapsed() < behaviour.holdTime && socket->state() == QAbstractSocket::ConnectedState)
			{
				socket->waitForReadyRead(50);
				socket->readAll();
			}
			result.connectedAfterHold = socket->state() == QAbstractSocket::ConnectedState;
		}
	}

	static void runHandshake(std::unique_ptr<QTcpSocket>& socket, quint16 port, bool tls, const QSslCertificate& caCertificate,
							 const ClientBehaviour& behaviour, ClientResult& result)
	{
		if (tls)
		{
			auto sslSocket = std::make_unique<QSslSocket>();
			auto configuration = sslSocket->sslConfiguration();
			configuration.setProtocol(QSsl::TlsV1_3OrLater);
			configuration.setCaCertificates({caCertificate});
			configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
			sslSocket->setSslConfiguration(configuration);
			sslSocket->connectToHostEncrypted(QStringLiteral("127.0.0.1"), port);
			result.connected = sslSocket->waitForEncrypted(5000);
			result.encrypted = sslSocket->isEncrypted();
			socket = std::move(sslSocket);
		}
		else
		{
			socket = std::make_unique<QTcpSocket>();
			socket->connectToHost(QHostAddress::LocalHost, port);
			result.connected = socket->waitForConnected(5000);
		}

		if (result.connected && behaviour.sendVersionFirst)
		{
			socket->write("RFB 003.008\n");
		}

		if (result.connected == false || waitForBytes(*socket, sz_rfbProtocolVersionMsg) == false)
		{
			return;
		}

		result.banner = socket->read(sz_rfbProtocolVersionMsg);
		if (behaviour.stopAfterBanner)
		{
			return;
		}
		if (behaviour.sendVersionFirst == false)
		{
			socket->write(result.banner);
		}

		// security types: count 1, rfbSecTypeVeyon
		if (waitForBytes(*socket, 2) == false || socket->read(2) != QByteArray::fromHex("0128"))
		{
			return;
		}
		socket->write(QByteArray(1, rfbSecTypeVeyon));

		VariantArrayMessage authTypesMessage(socket.get());
		if (receiveMessage(*socket, authTypesMessage) == false || authTypesMessage.read().toInt() != 1 ||
			authTypesMessage.read().toInt() != RfbVeyonAuth::Token)
		{
			return;
		}

		VariantArrayMessage(socket.get()).write(int(RfbVeyonAuth::Token)).write(QStringLiteral("teacher")).send();

		VariantArrayMessage ackMessage(socket.get());
		if (receiveMessage(*socket, ackMessage) == false)
		{
			return;
		}

		VariantArrayMessage(socket.get()).write(behaviour.token).send();

		if (waitForBytes(*socket, 4))
		{
			result.authenticated = socket->read(4) == QByteArray(4, '\0'); // rfbVncAuthOK
		}
	}

	ClientResult connectClient(bool tls)
	{
		return connectClient(tls, ClientBehaviour{});
	}

	ClientResult connectClient(bool tls, const ClientBehaviour& behaviour)
	{
		ClientResult result;
		const auto caCertificate = m_caCertificate.toQSslCertificate();
		const auto port = m_port;
		std::unique_ptr<QThread> thread(QThread::create([&result, port, tls, caCertificate, behaviour]() {
			runClient(port, tls, caCertificate, behaviour, result);
		}));
		thread->start();
		while (thread->isFinished() == false)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		}
		thread->wait();
		// let the server process the end of the connection
		QTest::qWait(50);
		return result;
	}

private Q_SLOTS:
	void initTestCase()
	{
		// VncProxyServer reads the connection limits from the (default) configuration
		new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::Server, QStringLiteral("VncProxyServerTlsTest"),
					  VeyonCore::minimalInitialization());

		m_now = QDateTime::currentDateTimeUtc();
		m_caKey = TlsTestHostCredentials::generateKey();
		m_hostKey = TlsTestHostCredentials::generateKey();
		m_caCertificate = PkiTestCertificates::createCa(m_caKey, QStringLiteral("Veyon Test CA"), m_now);
		const auto hostCertificate = TlsTestHostCredentials::createCertificate(m_hostKey, m_now, -60, 86400, &m_caKey,
																			   m_caCertificate.subjectCommonName());
		m_credentials = TlsHostCredentials::fromEncoded(TlsTestHostCredentials::encode(m_hostKey, {hostCertificate}), m_now);
		QVERIFY(m_credentials.has_value() || VeyonCore::isTlsAvailable() == false);
	}

	void cleanup()
	{
		m_proxyServer.reset();
	}

	// server mode × client transport (05-tls.md §10, server side)
	void policyMatrix_data()
	{
		QTest::addColumn<VeyonCore::TlsServerMode>("mode");
		QTest::addColumn<bool>("withCredentials");
		QTest::addColumn<bool>("tlsClient");
		QTest::addColumn<bool>("expectAuthenticated");

		using Mode = VeyonCore::TlsServerMode;
		QTest::newRow("Disabled, plain (4.11 master)") << Mode::Disabled << true << false << true;
		QTest::newRow("Disabled, TLS") << Mode::Disabled << true << true << false;
		QTest::newRow("Optional, plain (4.11 master)") << Mode::Optional << true << false << true;
		QTest::newRow("Optional, TLS") << Mode::Optional << true << true << true;
		QTest::newRow("Required, plain (4.11 master)") << Mode::Required << true << false << false;
		QTest::newRow("Required, TLS") << Mode::Required << true << true << true;
		QTest::newRow("Optional without certificate, plain") << Mode::Optional << false << false << true;
		QTest::newRow("Optional without certificate, TLS") << Mode::Optional << false << true << false;
		QTest::newRow("Required without certificate, plain") << Mode::Required << false << false << false;
		QTest::newRow("Required without certificate, TLS") << Mode::Required << false << true << false;
	}

	void policyMatrix()
	{
		QFETCH(VeyonCore::TlsServerMode, mode);
		QFETCH(bool, withCredentials);
		QFETCH(bool, tlsClient);
		QFETCH(bool, expectAuthenticated);

		// e.g. MSYS2's Qt 6 has the Schannel backend only (spike S8)
		if (VeyonCore::isTlsAvailable() == false && (tlsClient || (withCredentials && mode != VeyonCore::TlsServerMode::Disabled)))
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		QVERIFY(startProxyServer(policy(mode, withCredentials)));

		const auto result = connectClient(tlsClient);
		QCOMPARE(result.authenticated, expectAuthenticated);
		QCOMPARE(result.encrypted, expectAuthenticated && tlsClient);

		if (expectAuthenticated)
		{
			QCOMPARE(result.banner, QByteArrayLiteral("RFB 003.008\n"));
			QCOMPARE(m_authenticationResults.size(), 1);
			QVERIFY(m_authenticationResults.first().authenticated);
			// the server side sees the transport the client used
			QCOMPARE(m_authenticationResults.first().secureTransport, tlsClient);
		}
		else
		{
			QVERIFY(m_authenticationResults.isEmpty());
			// TLS client of a plain-only server: banner instead of ServerHello; plain client of a
			// TLS-only server: nothing at all
			QVERIFY(result.banner.isEmpty());
		}
	}

	// authentication results reach the limiter through VncProxyServer::reportAuthenticationResult()
	void authenticationResults()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		QVERIFY(startProxyServer(policy(VeyonCore::TlsServerMode::Optional)));

		const auto authenticated = connectClient(true);
		QVERIFY(authenticated.authenticated);
		ClientBehaviour wrongToken;
		wrongToken.token = "wrong-token";
		const auto failed = connectClient(false, wrongToken);
		QVERIFY(failed.authenticated == false);
		QCOMPARE(failed.banner, QByteArrayLiteral("RFB 003.008\n"));

		QCOMPARE(m_authenticationResults.size(), 2);
		QVERIFY(m_authenticationResults.at(0).authenticated);
		QVERIFY(m_authenticationResults.at(1).authenticated == false);
		QVERIFY(m_authenticationResults.at(1).secureTransport == false);

		// several connections, also concurrently authenticated ones
		for (int i = 0; i < 5; ++i)
		{
			QVERIFY(connectClient(i % 2 == 0).authenticated);
		}
		QCOMPARE(m_authenticationResults.size(), 7);

		// reporting for unknown connections is harmless
		m_proxyServer->reportAuthenticationResult(nullptr, true);
	}

	// Network/ConnectionLimit/PreAuthTimeoutSeconds: authenticated connections stay, others are closed
	void preAuthTimeout()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		auto& config = VeyonCore::config();
		const auto previousTimeout = config.connectionLimitPreAuthTimeoutSeconds();
		config.setConnectionLimitPreAuthTimeoutSeconds(1);
		const auto restoreTimeout = qScopeGuard([&]() { config.setConnectionLimitPreAuthTimeoutSeconds(previousTimeout); });

		QVERIFY(startProxyServer(policy(VeyonCore::TlsServerMode::Optional)));

		ClientBehaviour holding;
		holding.holdTime = 1800;
		const auto authenticated = connectClient(true, holding);
		QVERIFY(authenticated.authenticated);
		QVERIFY(authenticated.connectedAfterHold);

		ClientBehaviour idle = holding;
		idle.stopAfterBanner = true;
		for (const auto tls : {false, true})
		{
			const auto unauthenticated = connectClient(tls, idle);
			QCOMPARE(unauthenticated.banner, QByteArrayLiteral("RFB 003.008\n"));
			QVERIFY(unauthenticated.authenticated == false);
			QVERIFY(unauthenticated.connectedAfterHold == false);
		}
	}

	// a plain client that speaks before the banner is not stalled by the sniffing (it consumed the
	// first readyRead())
	void plainClientSpeakingFirst()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		QVERIFY(startProxyServer(policy(VeyonCore::TlsServerMode::Optional)));

		ClientBehaviour speakingFirst;
		speakingFirst.sendVersionFirst = true;
		const auto result = connectClient(false, speakingFirst);
		QCOMPARE(result.banner, QByteArrayLiteral("RFB 003.008\n"));
		QVERIFY(result.authenticated);
	}

	// an expired host certificate is still offered: the verifying master decides
	void expiredCertificate()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		const auto expired = TlsTestHostCredentials::createCertificate(m_hostKey, m_now, -10 * 86400, -86400, &m_caKey,
																	   m_caCertificate.subjectCommonName());
		const auto credentials = TlsHostCredentials::fromEncoded(TlsTestHostCredentials::encode(m_hostKey, {expired}), m_now);
		QVERIFY(credentials.has_value());
		auto expiredPolicy = TlsTestHostCredentials::policy(VeyonCore::TlsServerMode::Required, &*credentials);
		expiredPolicy.sniffTimeoutMs = 100;
		QVERIFY(startProxyServer(expiredPolicy));

		// TLS is still offered: the handshake reaches certificate verification, which fails
		QSslSocket client;
		auto configuration = client.sslConfiguration();
		configuration.setCaCertificates({m_caCertificate.toQSslCertificate()});
		client.setSslConfiguration(configuration);
		QList<QSslError> errors;
		connect(&client, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this,
				[&errors](const QList<QSslError>& sslErrors) { errors = sslErrors; });
		client.connectToHostEncrypted(QStringLiteral("127.0.0.1"), m_port);
		QTRY_VERIFY(errors.isEmpty() == false);
		QVERIFY(std::any_of(errors.cbegin(), errors.cend(), [](const QSslError& error) {
			return error.error() == QSslError::CertificateExpired;
		}));
		QCOMPARE(client.peerCertificate().toDer(), expired.toDer());
	}

	void startTwice()
	{
		if (VeyonCore::isTlsAvailable() == false)
		{
			QSKIP("Qt OpenSSL TLS backend not available");
		}

		QVERIFY(startProxyServer(policy(VeyonCore::TlsServerMode::Optional)));
		QVERIFY(m_proxyServer->start(unusedVncServerEndpoint(), {}, policy(VeyonCore::TlsServerMode::Optional)) == false);

		// port in use
		VncProxyServer otherServer(QHostAddress::LocalHost, m_port, this);
		QVERIFY(otherServer.start(unusedVncServerEndpoint(), {}, policy(VeyonCore::TlsServerMode::Optional)) == false);
		otherServer.reportAuthenticationResult(nullptr, true);

		// restart after stop
		m_proxyServer->stop();
		QVERIFY(m_proxyServer->start(unusedVncServerEndpoint(), {}, policy(VeyonCore::TlsServerMode::Optional)));
		QVERIFY(connectClient(true).authenticated);
	}
};


QTEST_GUILESS_MAIN(VncProxyServerTlsTest)
#include "VncProxyServerTlsTest.moc"
