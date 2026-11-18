// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSemaphore>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QtEndian>
#include <QtTest>

#include "LocalCredentialsVerifier.h"
#include "RfbVeyonAuth.h"
#include "VariantArrayMessage.h"
#include "VncServerClient.h"
#include "VncServerProtocol.h"

#include <memory>

// the server protocol of veyon-server (VncServerProtocol) with Logon authentication as in
// ServerAuthenticationManager, credentials "teacher"/"correct"
class LogonServerProtocol : public VncServerProtocol
{
public:
	LogonServerProtocol(QTcpSocket* socket, VncServerClient* client, bool grantAccess) :
		VncServerProtocol(socket, client),
		m_grantAccess(grantAccess)
	{
	}

protected:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override
	{
		return {RfbVeyonAuth::Logon};
	}

	void processAuthenticationMessage(VariantArrayMessage& message) override
	{
		switch (client()->authState())
		{
		case VncServerClient::AuthState::Init:
			client()->setPrivateKey(CryptoCore::KeyGenerator().createRSA(2048));
			VariantArrayMessage(message.ioDevice()).write(client()->privateKey().toPublicKey().toPEM()).send();
			client()->setAuthState(VncServerClient::AuthState::Password);
			break;
		case VncServerClient::AuthState::Password:
		{
			auto privateKey = client()->privateKey();
			CryptoCore::SecureArray password;
			const auto decrypted = privateKey.decrypt(message.read().toByteArray(), &password,
													  CryptoCore::DefaultEncryptionAlgorithm);
			client()->setAuthState(decrypted && client()->username() == QLatin1String("teacher") &&
										   password.toByteArray() == "correct" ?
									   VncServerClient::AuthState::Successful :
									   VncServerClient::AuthState::Failed);
			break;
		}
		default:
			break;
		}
	}

	void performAccessControl() override
	{
		client()->setAccessControlState(m_grantAccess ? VncServerClient::AccessControlState::Successful : VncServerClient::AccessControlState::Failed);
	}

private:
	const bool m_grantAccess;
};



// Credential check of master and configurator through the local Veyon Server
// (docs/specs/02-ipc.md §6b) against an emulated server side of the Veyon protocol up to the
// security result
class LocalCredentialsVerifierTest : public QObject
{
	Q_OBJECT
private:
	enum class Peer {
		CheckPassword, // "teacher"/"correct" ⇒ rfbVncAuthOK, else close (as ServerAuthenticationManager)
		AccessDenied,  // rfbVncAuthOK, then empty ServerInit and close (access control failed)
		NoLogon,	   // offers key file authentication only
		NotVeyon,	   // another RFB server
		Silent,		   // never sends the security result
	};

	struct PeerResult
	{
		QString username;
		QByteArray password;
	};

	static constexpr int ShortTimeout = 1500;

	std::unique_ptr<QCA::Initializer> m_qcaInitializer;
	CryptoCore::PrivateKey m_privateKey;

	static bool waitForBytes(QTcpSocket& socket, qint64 count)
	{
		while (socket.bytesAvailable() < count)
		{
			if (socket.waitForReadyRead(5000) == false)
			{
				return false;
			}
		}
		return true;
	}

	static bool receive(QTcpSocket& socket, VariantArrayMessage& message)
	{
		while (message.isReadyForReceive() == false)
		{
			if (socket.waitForReadyRead(5000) == false)
			{
				return false;
			}
		}
		return message.receive();
	}

	// the server side of one connection, run in its own thread
	void servePeer(QTcpSocket& socket, Peer peer, PeerResult& result) const
	{
		if (peer == Peer::NotVeyon)
		{
			socket.write("RFB 003.003\n");
			socket.waitForBytesWritten();
			return;
		}

		socket.write("RFB 003.008\n");
		const char securityTypes[2] = {1, rfbSecTypeVeyon};
		socket.write(securityTypes, sizeof(securityTypes));
		if (waitForBytes(socket, 12 + 1) == false)
		{
			return;
		}
		socket.read(12 + 1);

		VariantArrayMessage authTypes(&socket);
		authTypes.write(1).write(int(peer == Peer::NoLogon ? RfbVeyonAuth::KeyFile : RfbVeyonAuth::Logon));
		authTypes.send();

		VariantArrayMessage chosen(&socket);
		if (receive(socket, chosen) == false || chosen.read().value<RfbVeyonAuth::Type>() != RfbVeyonAuth::Logon)
		{
			return;
		}
		result.username = chosen.read().toString();

		VariantArrayMessage(&socket).send();
		VariantArrayMessage publicKey(&socket);
		publicKey.write(m_privateKey.toPublicKey().toPEM());
		publicKey.send();

		VariantArrayMessage passwordMessage(&socket);
		if (receive(socket, passwordMessage) == false)
		{
			return;
		}
		CryptoCore::SecureArray decrypted;
		auto privateKey = m_privateKey;
		privateKey.decrypt(passwordMessage.read().toByteArray(), &decrypted, CryptoCore::DefaultEncryptionAlgorithm);
		result.password = decrypted.toByteArray();

		const auto ok = qToBigEndian<quint32>(0);
		switch (peer)
		{
		case Peer::CheckPassword:
			if (result.username == QLatin1String("teacher") && result.password == "correct")
			{
				socket.write(reinterpret_cast<const char*>(&ok), sizeof(ok));
			}
			break;
		case Peer::AccessDenied:
			socket.write(reinterpret_cast<const char*>(&ok), sizeof(ok));
			socket.write(QByteArray(24, 0));
			break;
		case Peer::Silent:
			QThread::msleep(ShortTimeout + 500);
			break;
		default:
			break;
		}
		socket.waitForBytesWritten();
		socket.disconnectFromHost();
		if (socket.state() != QAbstractSocket::UnconnectedState)
		{
			socket.waitForDisconnected(1000);
		}
	}

	LocalCredentialsVerifier::Result verifyWith(Peer peer, const QString& username, const QByteArray& password,
												PeerResult* peerResult = nullptr, int timeout = LocalCredentialsVerifier::Timeout)
	{
		quint16 port = 0;
		QSemaphore listening;
		PeerResult result;
		std::unique_ptr<QThread> thread(QThread::create([&]() {
			QTcpServer server;
			const auto ok = server.listen(QHostAddress::LocalHost);
			port = server.serverPort();
			listening.release();
			if (ok && server.waitForNewConnection(10000))
			{
				std::unique_ptr<QTcpSocket> socket(server.nextPendingConnection());
				servePeer(*socket, peer, result);
			}
		}));
		thread->start();
		listening.acquire();

		const auto verification = LocalCredentialsVerifier::verify(username, Veyon::Crypto::SecureBuffer::fromByteArray(password),
																   QStringLiteral("127.0.0.1"), port, timeout);
		thread->wait();
		if (peerResult)
		{
			*peerResult = result;
		}
		return verification;
	}

private Q_SLOTS:
	void initTestCase()
	{
		m_qcaInitializer = std::make_unique<QCA::Initializer>();
		m_privateKey = CryptoCore::KeyGenerator().createRSA(2048);
		QVERIFY(m_privateKey.isNull() == false);
	}

	void cleanupTestCase()
	{
		m_privateKey = {};
		m_qcaInitializer.reset();
	}

	// the real server protocol (events in this thread, the blocking check in another one); it sends
	// the security result before access control, whose failure path needs a full VeyonCore and is
	// covered by validButAccessDenied()
	void serverProtocol_data()
	{
		QTest::addColumn<QString>("username");
		QTest::addColumn<QByteArray>("password");
		QTest::addColumn<bool>("grantAccess");
		QTest::addColumn<int>("expected");

		QTest::newRow("valid") << QStringLiteral("teacher") << QByteArray("correct") << true
							   << int(LocalCredentialsVerifier::Result::Valid);
		QTest::newRow("wrong password") << QStringLiteral("teacher") << QByteArray("wrong") << true
										<< int(LocalCredentialsVerifier::Result::Invalid);
		QTest::newRow("other account") << QStringLiteral("veyon") << QByteArray("correct") << true
									   << int(LocalCredentialsVerifier::Result::Invalid);
	}

	void serverProtocol()
	{
		QFETCH(QString, username);
		QFETCH(QByteArray, password);
		QFETCH(bool, grantAccess);
		QFETCH(int, expected);

		QTcpServer server;
		QVERIFY(server.listen(QHostAddress::LocalHost));
		std::unique_ptr<QTcpSocket> socket;
		VncServerClient client;
		std::unique_ptr<LogonServerProtocol> protocol;
		connect(&server, &QTcpServer::newConnection, this, [&]() {
			socket.reset(server.nextPendingConnection());
			protocol = std::make_unique<LogonServerProtocol>(socket.get(), &client, grantAccess);
			connect(socket.get(), &QTcpSocket::readyRead, this, [&]() {
				while (protocol->read())
				{
				}
			});
			protocol->start();
		});

		auto result = LocalCredentialsVerifier::Result::Unavailable;
		const auto port = server.serverPort();
		std::unique_ptr<QThread> thread(QThread::create([&]() {
			result = LocalCredentialsVerifier::verify(username, Veyon::Crypto::SecureBuffer::fromByteArray(password),
													  QStringLiteral("127.0.0.1"), port);
		}));
		thread->start();
		QTRY_VERIFY_WITH_TIMEOUT(thread->isFinished(), LocalCredentialsVerifier::Timeout);
		thread->wait();

		QCOMPARE(int(result), expected);
		QCOMPARE(client.username(), username);
	}

	void validCredentials()
	{
		PeerResult peer;
		QCOMPARE(verifyWith(Peer::CheckPassword, QStringLiteral("teacher"), "correct", &peer),
				 LocalCredentialsVerifier::Result::Valid);
		// the server got the credentials, the password only encrypted with its key
		QCOMPARE(peer.username, QStringLiteral("teacher"));
		QCOMPARE(peer.password, QByteArray("correct"));
	}

	void invalidCredentials()
	{
		QCOMPARE(verifyWith(Peer::CheckPassword, QStringLiteral("teacher"), "wrong"),
				 LocalCredentialsVerifier::Result::Invalid);
		QCOMPARE(verifyWith(Peer::CheckPassword, QStringLiteral("veyon"), "correct"),
				 LocalCredentialsVerifier::Result::Invalid);
	}

	// access control is not part of the check
	void validButAccessDenied()
	{
		QCOMPARE(verifyWith(Peer::AccessDenied, QStringLiteral("teacher"), "correct"),
				 LocalCredentialsVerifier::Result::Valid);
	}

	void noServer()
	{
		QTcpServer server;
		QVERIFY(server.listen(QHostAddress::LocalHost));
		const auto port = server.serverPort();
		server.close();

		QCOMPARE(LocalCredentialsVerifier::verify(QStringLiteral("teacher"), Veyon::Crypto::SecureBuffer::fromByteArray("correct"),
												  QStringLiteral("127.0.0.1"), port),
				 LocalCredentialsVerifier::Result::Unavailable);
	}

	void logonNotOffered()
	{
		QCOMPARE(verifyWith(Peer::NoLogon, QStringLiteral("teacher"), "correct"),
				 LocalCredentialsVerifier::Result::Unavailable);
	}

	void notAVeyonServer()
	{
		QCOMPARE(verifyWith(Peer::NotVeyon, QStringLiteral("teacher"), "correct"),
				 LocalCredentialsVerifier::Result::Unavailable);
	}

	void noAnswerInTime()
	{
		QElapsedTimer timer;
		timer.start();
		QCOMPARE(verifyWith(Peer::Silent, QStringLiteral("teacher"), "correct", nullptr, ShortTimeout),
				 LocalCredentialsVerifier::Result::Unavailable);
		QVERIFY(timer.elapsed() < ShortTimeout + 2000);
	}
};


QTEST_GUILESS_MAIN(LocalCredentialsVerifierTest)
#include "LocalCredentialsVerifierTest.moc"
