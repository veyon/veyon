// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>
#include <QDir>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include "AuthenticationCredentials.h"
#include "AuthenticationProxy.h"
#include "CryptoCore.h"
#include "Logger.h"
#include "ServerAuthenticationManager.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncServerClient.h"
#include "VncServerProtocol.h"
#include "rfb/rfbproto.h"

// Pre-auth hardening of the Veyon authentication (05-tls.md §6): every message of the
// authentication phase is capped at 64 KiB on both sides, and the master only uses the
// authentication type of its configured method. The wire format is unchanged, so the scripted
// peers below speak exactly what 4.11 servers and masters send.

using AuthType = RfbVeyonAuth::Type;

// the server side of a master connection: reads come from a script, writes are recorded
class ScriptedServerDevice : public QIODevice
{
	Q_OBJECT
public:
	explicit ScriptedServerDevice(const QByteArray& serverData) :
		m_serverData(serverData)
	{
		open(QIODevice::ReadWrite | QIODevice::Unbuffered);
	}

	bool isSequential() const override
	{
		return true;
	}

	const QByteArray& clientData() const
	{
		return m_clientData;
	}

	void setWriteLimit(qint64 writeLimit)
	{
		m_writeLimit = writeLimit;
	}

protected:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto size = std::min<qint64>(maxSize, m_serverData.size() - m_readPosition);
		std::memcpy(data, m_serverData.constData() + m_readPosition, size_t(size));
		m_readPosition += size;
		return size;
	}

	qint64 writeData(const char* data, qint64 size) override
	{
		if (m_clientData.size() + size > m_writeLimit)
		{
			return -1;
		}
		m_clientData.append(data, size);
		return size;
	}

private:
	const QByteArray m_serverData;
	qint64 m_readPosition = 0;
	QByteArray m_clientData;
	qint64 m_writeLimit = std::numeric_limits<qint64>::max();
};



class TestAuthenticationProxy : public AuthenticationProxy
{
public:
	TestAuthenticationProxy(AuthType authType, const AuthenticationCredentials& credentials) :
		m_authType(authType)
	{
		setCredentials(credentials);
	}

	AuthType initCredentials() override
	{
		return m_authType;
	}

private:
	const AuthType m_authType;
};



// VeyonServerProtocol without access control
class TestServerProtocol : public VncServerProtocol
{
public:
	TestServerProtocol(QTcpSocket* socket, VncServerClient* client, ServerAuthenticationManager& authenticationManager) :
		VncServerProtocol(socket, client),
		m_authenticationManager(authenticationManager)
	{
	}

	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override
	{
		return m_authenticationManager.supportedAuthTypes();
	}

	void processAuthenticationMessage(VariantArrayMessage& message) override
	{
		m_authenticationManager.processAuthenticationMessage(client(), message);
	}

	void performAccessControl() override
	{
		client()->setAccessControlState(VncServerClient::AccessControlState::Successful);
	}

private:
	ServerAuthenticationManager& m_authenticationManager;
};



class VeyonAuthenticationTest : public QObject
{
	Q_OBJECT
private:
	static constexpr auto MaxSize = qsizetype(VariantArrayMessage::MaxAuthenticationMessageSize);
	static constexpr auto IoTimeout = 10000;
	static constexpr auto Username = "teacher";
	static constexpr auto Password = "Password-9d1c";
	static constexpr auto KeyName = "teacher";

	// tokens are exactly CryptoCore::ChallengeSize bytes
	static QByteArray tokenValue()
	{
		return QByteArray(CryptoCore::ChallengeSize, 't');
	}

	QTemporaryDir m_temporaryDirectory;
	VeyonCore* m_core = nullptr;
	CryptoCore::PrivateKey m_privateKey;

	// --- helpers for the master side

	static QByteArray serverMessage(const QVariantList& values, qsizetype paddedSize = 0)
	{
		const auto serialize = [&values](qsizetype paddingSize) {
			QBuffer buffer;
			buffer.open(QBuffer::ReadWrite);
			VariantArrayMessage message(&buffer);
			for (const auto& value : values)
			{
				message.write(value);
			}
			if (paddingSize >= 0)
			{
				message.write(QByteArray(paddingSize, 'x'));
			}
			(void)message.send();
			return buffer.data();
		};

		if (paddedSize <= 0)
		{
			return serialize(-1);
		}

		// payload size = message size - size field; grows by one byte per padding byte
		const auto minimumSize = serialize(0).size() - qsizetype(sizeof(VariantArrayMessage::MessageSize));
		return serialize(paddedSize - minimumSize);
	}

	static QByteArray authTypesMessage(const QList<int>& authTypes, qsizetype paddedSize = 0)
	{
		QVariantList values{int(authTypes.size())};
		for (const auto authType : authTypes)
		{
			values.append(authType);
		}
		return serverMessage(values, paddedSize);
	}

	static QByteArray ackMessage()
	{
		return serverMessage({});
	}

	static QList<QVariantList> clientMessages(const QByteArray& data)
	{
		auto dataCopy = data;
		QBuffer buffer(&dataCopy);
		buffer.open(QBuffer::ReadOnly);

		QList<QVariantList> messages;
		while (buffer.atEnd() == false)
		{
			VariantArrayMessage message(&buffer);
			if (message.receive() == false)
			{
				break;
			}
			QVariantList values;
			for (auto value = message.read(); value.isValid(); value = message.read())
			{
				values.append(value);
			}
			messages.append(values);
		}
		return messages;
	}

	// a connection whose (private) destructor runs via the VncConnection
	struct Connection
	{
		explicit Connection(AuthType authType = AuthType::Invalid, AuthenticationProxy* proxy = nullptr) :
			connection(new VeyonConnection)
		{
			if (authType != AuthType::Invalid)
			{
				connection->setVeyonAuthType(authType);
			}
			connection->setAuthenticationProxy(proxy);
		}

		~Connection()
		{
			connection->stopAndDeleteLater();
			QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		}

		bool authenticate(ScriptedServerDevice& device)
		{
			return VeyonConnection::authenticate(connection, device);
		}

		VeyonConnection* connection;
	};

	QByteArray challengeFlow(const QByteArray& challenge) const
	{
		return authTypesMessage({AuthType::KeyFile}) + ackMessage() + serverMessage({challenge});
	}

	QByteArray logonFlow() const
	{
		return authTypesMessage({AuthType::Logon}) + ackMessage() + serverMessage({m_privateKey.toPublicKey().toPEM()});
	}

	void setKeyFileCredentials()
	{
		VeyonCore::authenticationCredentials().setPrivateKey(m_privateKey);
		VeyonCore::authenticationCredentials().setAuthenticationKeyName(QString::fromLatin1(KeyName));
	}

	// --- helpers for the server side

	struct ServerConnection
	{
		explicit ServerConnection(ServerAuthenticationManager& authenticationManager)
		{
			QVERIFY(listener.listen(QHostAddress::LocalHost));
			master.connectToHost(QHostAddress::LocalHost, listener.serverPort());
			QVERIFY(master.waitForConnected(IoTimeout));
			QVERIFY(listener.waitForNewConnection(IoTimeout));
			socket = listener.nextPendingConnection();
			QVERIFY(socket);
			protocol = std::make_unique<TestServerProtocol>(socket, &client, authenticationManager);
		}

		// lets the server process everything it has received so far
		void pump()
		{
			socket->waitForReadyRead(10);
			while (protocol->read())
			{
			}
			socket->flush();
		}

		QByteArray readFromServer(qsizetype size)
		{
			QByteArray data;
			for (int i = 0; i < 100 && data.size() < size; ++i)
			{
				if (master.bytesAvailable() == 0)
				{
					master.waitForReadyRead(100);
				}
				data += master.read(size - data.size());
			}
			return data;
		}

		std::optional<QVariantList> receiveFromServer()
		{
			for (int i = 0; i < 100; ++i)
			{
				VariantArrayMessage message(&master);
				if (message.isReadyForReceive())
				{
					if (message.receive() == false)
					{
						return {};
					}
					QVariantList values;
					for (auto value = message.read(); value.isValid(); value = message.read())
					{
						values.append(value);
					}
					return values;
				}
				master.waitForReadyRead(100);
			}
			return {};
		}

		// sends data and lets the server process it, also when it arrives in several parts
		void sendToServer(const QByteArray& data)
		{
			master.write(data);
			for (int i = 0; i < 500; ++i)
			{
				master.waitForBytesWritten(10);
				pump();
				if (master.bytesToWrite() == 0 && (socket->isOpen() == false || socket->bytesAvailable() == 0))
				{
					break;
				}
			}
		}

		// connection setup up to the list of authentication types (as libvncclient does it)
		std::optional<QVariantList> startHandshake()
		{
			protocol->start();
			socket->flush();
			if (readFromServer(sz_rfbProtocolVersionMsg) != QByteArrayLiteral("RFB 003.008\n"))
			{
				return {};
			}
			sendToServer(QByteArrayLiteral("RFB 003.008\n"));
			if (readFromServer(2) != QByteArray("\x01\x28", 2))
			{
				return {};
			}
			sendToServer(QByteArray(1, rfbSecTypeVeyon));
			return receiveFromServer();
		}

		bool isClosed()
		{
			for (int i = 0; i < 20 && master.state() == QAbstractSocket::ConnectedState; ++i)
			{
				master.waitForDisconnected(100);
			}
			return socket->isOpen() == false || master.state() != QAbstractSocket::ConnectedState;
		}

		QTcpServer listener;
		QTcpSocket master;
		QTcpSocket* socket = nullptr;
		VncServerClient client;
		std::unique_ptr<TestServerProtocol> protocol;
	};

	static QByteArray authResultOk()
	{
		const auto authResult = qToBigEndian<uint32_t>(rfbVncAuthOK);
		return QByteArray(reinterpret_cast<const char*>(&authResult), sizeof(authResult));
	}

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_temporaryDirectory.isValid());
		// never read or write a real configuration
		QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, m_temporaryDirectory.filePath(QStringLiteral("system")));
		QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_temporaryDirectory.filePath(QStringLiteral("user")));

		m_core = new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::CLI, QStringLiteral("AuthenticationTest"),
							   VeyonCore::InitializationFlags(VeyonCore::InitializationFlag::LegacyCrypto) |
								   VeyonCore::InitializationFlag::AuthenticationCredentials);

		QCOMPARE(VeyonCore::config().authenticationMethod(), VeyonCore::AuthenticationMethod::LogonAuthentication);
		VeyonCore::config().setPublicKeyBaseDir(m_temporaryDirectory.filePath(QStringLiteral("keys")));

		m_privateKey = CryptoCore::KeyGenerator().createRSA(CryptoCore::RsaKeySize);
		QVERIFY(m_privateKey.canSign());

		const auto publicKeyDirectory = m_temporaryDirectory.filePath(QStringLiteral("keys/%1").arg(QLatin1String(KeyName)));
		QVERIFY(QDir().mkpath(publicKeyDirectory));
		QVERIFY(m_privateKey.toPublicKey().toPEMFile(publicKeyDirectory + QStringLiteral("/key")));
	}

	void cleanupTestCase()
	{
		delete m_core;
		m_core = nullptr;
	}

	void init()
	{
		auto& credentials = VeyonCore::authenticationCredentials();
		credentials = AuthenticationCredentials();
		credentials.setLogonUsername(QString::fromLatin1(Username));
		credentials.setLogonPassword(AuthenticationCredentials::Password(QByteArray(Password)));
		VeyonCore::config().setAuthenticationMethod(VeyonCore::AuthenticationMethod::LogonAuthentication);
	}

	// ----- master

	void masterConfiguredType()
	{
		QCOMPARE(Connection().connection->veyonAuthType(), AuthType::Logon);

		VeyonCore::config().setAuthenticationMethod(VeyonCore::AuthenticationMethod::KeyFileAuthentication);
		QCOMPARE(Connection().connection->veyonAuthType(), AuthType::KeyFile);

		// the demo client: the token is the only credential it has
		VeyonCore::authenticationCredentials().setToken(AuthenticationCredentials::Token(tokenValue()));
		QCOMPARE(Connection().connection->veyonAuthType(), AuthType::Token);
	}

	// 4.11 server with logon authentication
	void masterLogon()
	{
		ScriptedServerDevice device(logonFlow());
		QVERIFY(Connection().authenticate(device));

		const auto messages = clientMessages(device.clientData());
		QCOMPARE(messages.size(), 2);
		QCOMPARE(messages[0], (QVariantList{int(AuthType::Logon), QString::fromLatin1(Username)}));

		CryptoCore::SecureArray decryptedPassword;
		QVERIFY(m_privateKey.decrypt(messages[1].value(0).toByteArray(), &decryptedPassword, CryptoCore::DefaultEncryptionAlgorithm));
		QCOMPARE(decryptedPassword.toByteArray(), QByteArray(Password));
	}

	// 4.11 server with key file authentication
	void masterKeyFile()
	{
		setKeyFileCredentials();
		const auto challenge = CryptoCore::generateChallenge();

		ScriptedServerDevice device(challengeFlow(challenge));
		QVERIFY(Connection(AuthType::KeyFile).authenticate(device));

		const auto messages = clientMessages(device.clientData());
		QCOMPARE(messages.size(), 2);
		QCOMPARE(messages[0].value(0).toInt(), int(AuthType::KeyFile));
		QCOMPARE(messages[1].value(0).toString(), QString::fromLatin1(KeyName));
		QVERIFY(m_privateKey.toPublicKey().verifyMessage(challenge, messages[1].value(1).toByteArray(),
														 CryptoCore::DefaultSignatureAlgorithm));
	}

	// demo server
	void masterToken()
	{
		VeyonCore::authenticationCredentials().setToken(AuthenticationCredentials::Token(tokenValue()));

		ScriptedServerDevice device(authTypesMessage({AuthType::Token}) + ackMessage());
		QVERIFY(Connection().authenticate(device));

		const auto messages = clientMessages(device.clientData());
		QCOMPARE(messages.size(), 2);
		QCOMPARE(messages[0].value(0).toInt(), int(AuthType::Token));
		QCOMPARE(messages[1], QVariantList{tokenValue()});
	}

	void masterChoosesConfiguredType_data()
	{
		QTest::addColumn<QList<int>>("offered");

		// the configured type is chosen wherever the server lists it
		QTest::newRow("only") << QList<int>{AuthType::Logon};
		QTest::newRow("last") << QList<int>{AuthType::KeyFile, AuthType::Token, AuthType::Logon};
		// unknown types (e.g. KeyFileBound = 6 of a newer server) are ignored
		QTest::newRow("unknown types") << QList<int>{6, 99, AuthType::Logon};
		QTest::newRow("maximum count") << QList<int>(VeyonConnection::MaxAuthTypeCount, AuthType::Logon);
	}

	void masterChoosesConfiguredType()
	{
		QFETCH(QList<int>, offered);

		ScriptedServerDevice device(authTypesMessage(offered) + ackMessage() +
									serverMessage({m_privateKey.toPublicKey().toPEM()}));
		QVERIFY(Connection().authenticate(device));
		QCOMPARE(clientMessages(device.clientData()).value(0).value(0).toInt(), int(AuthType::Logon));
	}

	// 05-tls.md §6/§10: the server cannot choose the type; nothing is sent then
	void masterAbortsWithoutConfiguredType_data()
	{
		QTest::addColumn<int>("configured");
		QTest::addColumn<QList<int>>("offered");

		QTest::newRow("logon, server offers key file") << int(AuthType::Logon) << QList<int>{AuthType::KeyFile};
		QTest::newRow("logon, server offers token") << int(AuthType::Logon) << QList<int>{AuthType::Token};
		QTest::newRow("key file, server offers logon") << int(AuthType::KeyFile) << QList<int>{AuthType::Logon, AuthType::Token};
		QTest::newRow("token, server offers logon") << int(AuthType::Token) << QList<int>{AuthType::Logon};
		QTest::newRow("unknown types only") << int(AuthType::Logon) << QList<int>{6, 7};
	}

	void masterAbortsWithoutConfiguredType()
	{
		QFETCH(int, configured);
		QFETCH(QList<int>, offered);

		setKeyFileCredentials();
		ScriptedServerDevice device(authTypesMessage(offered) + ackMessage() + serverMessage({CryptoCore::generateChallenge()}));
		QCOMPARE(Connection(AuthType(configured)).authenticate(device), false);
		QVERIFY(device.clientData().isEmpty());
	}

	void masterRejectsInvalidAuthTypeList_data()
	{
		QTest::addColumn<QByteArray>("serverData");

		QTest::newRow("no message") << QByteArray();
		QTest::newRow("truncated size") << QByteArray("\x00\x00", 2);
		QTest::newRow("truncated message") << authTypesMessage({AuthType::Logon}).chopped(1);
		QTest::newRow("no auth types") << authTypesMessage({});
		QTest::newRow("negative count") << serverMessage({-1, int(AuthType::Logon)});
		QTest::newRow("too many auth types") << authTypesMessage(QList<int>(VeyonConnection::MaxAuthTypeCount + 1, AuthType::Logon));
		QTest::newRow("count not a number") << serverMessage({QStringLiteral("one"), int(AuthType::Logon)});
		QTest::newRow("count without types") << serverMessage({2, int(AuthType::Logon)});
		QTest::newRow("invalid type") << authTypesMessage({AuthType::Invalid, AuthType::Logon});
		QTest::newRow("type not a number") << serverMessage({1, QStringLiteral("Logon")});
		QTest::newRow("empty message") << serverMessage({});
	}

	void masterRejectsInvalidAuthTypeList()
	{
		QFETCH(QByteArray, serverData);

		auto proxy = new TestAuthenticationProxy(AuthType::Logon, VeyonCore::authenticationCredentials());
		ScriptedServerDevice device(serverData + ackMessage());
		QCOMPARE(Connection(AuthType::Invalid, proxy).authenticate(device), false);
		QVERIFY(proxy->protocolErrorOccurred());
		QVERIFY(device.clientData().isEmpty());
	}

	// 64 KiB cap for the list of authentication types and every later server message
	void masterMessageSizeCap_data()
	{
		QTest::addColumn<int>("message");
		QTest::addColumn<qsizetype>("size");
		QTest::addColumn<bool>("accepted");

		for (int message = 0; message < 3; ++message)
		{
			const auto name = QByteArrayList{"auth types", "ack", "public key"}.at(message);
			QTest::newRow(QByteArray(name + " below").constData()) << message << MaxSize - 1 << true;
			QTest::newRow(QByteArray(name + " at").constData()) << message << MaxSize << true;
			QTest::newRow(QByteArray(name + " above").constData()) << message << MaxSize + 1 << false;
			QTest::newRow(QByteArray(name + " far above").constData()) << message << 16 * MaxSize << false;
		}
	}

	void masterMessageSizeCap()
	{
		QFETCH(int, message);
		QFETCH(qsizetype, size);
		QFETCH(bool, accepted);

		const auto publicKey = m_privateKey.toPublicKey().toPEM();
		const QByteArray serverData = authTypesMessage({AuthType::Logon}, message == 0 ? size : 0) +
			serverMessage({}, message == 1 ? size : 0) + serverMessage({publicKey}, message == 2 ? size : 0);
		const auto paddedMessage = message == 0 ? authTypesMessage({AuthType::Logon}, size) : serverMessage(message == 1 ? QVariantList{} : QVariantList{publicKey}, size);
		QCOMPARE(paddedMessage.size(), qsizetype(sizeof(VariantArrayMessage::MessageSize)) + size);

		ScriptedServerDevice device(serverData);
		QCOMPARE(Connection().authenticate(device), accepted);
		// the oversized authentication types never get an answer
		QCOMPARE(device.clientData().isEmpty(), message == 0 && accepted == false);
	}

	void masterRejectsInvalidChallenge_data()
	{
		QTest::addColumn<QByteArray>("challengeMessage");

		QTest::newRow("missing") << QByteArray();
		QTest::newRow("empty message") << serverMessage({});
		QTest::newRow("string") << serverMessage({QString::fromLatin1(CryptoCore::generateChallenge().toHex())});
		QTest::newRow("too short") << serverMessage({CryptoCore::generateChallenge().chopped(1)});
		QTest::newRow("too long") << serverMessage({QByteArray(CryptoCore::generateChallenge() + "x")});
		QTest::newRow("oversized") << serverMessage({CryptoCore::generateChallenge()}, MaxSize + 1);
	}

	void masterRejectsInvalidChallenge()
	{
		QFETCH(QByteArray, challengeMessage);

		setKeyFileCredentials();
		ScriptedServerDevice device(authTypesMessage({AuthType::KeyFile}) + ackMessage() + challengeMessage);
		QCOMPARE(Connection(AuthType::KeyFile).authenticate(device), false);
		// only the chosen type, no signature
		QCOMPARE(clientMessages(device.clientData()).size(), 1);
	}

	void masterRejectsInvalidPublicKey_data()
	{
		QTest::addColumn<QByteArray>("publicKeyMessage");

		QTest::newRow("missing") << QByteArray();
		QTest::newRow("empty message") << serverMessage({});
		QTest::newRow("garbage") << serverMessage({QStringLiteral("-----BEGIN PUBLIC KEY-----\nxyz\n-----END PUBLIC KEY-----\n")});
		QTest::newRow("oversized") << serverMessage({m_privateKey.toPublicKey().toPEM()}, MaxSize + 1);
	}

	void masterRejectsInvalidPublicKey()
	{
		QFETCH(QByteArray, publicKeyMessage);

		ScriptedServerDevice device(authTypesMessage({AuthType::Logon}) + ackMessage() + publicKeyMessage);
		QCOMPARE(Connection().authenticate(device), false);
		// only the chosen type, no password
		QCOMPARE(clientMessages(device.clientData()).size(), 1);
	}

	void masterRequiresAck()
	{
		ScriptedServerDevice device(authTypesMessage({AuthType::Logon}));
		QCOMPARE(Connection().authenticate(device), false);
		QCOMPARE(clientMessages(device.clientData()).size(), 1);
	}

	void masterKeyFileWithoutPrivateKey()
	{
		ScriptedServerDevice device(challengeFlow(CryptoCore::generateChallenge()));
		QCOMPARE(Connection(AuthType::KeyFile).authenticate(device), false);
		QCOMPARE(clientMessages(device.clientData()).size(), 1);
	}

	void masterUnsupportedConfiguredType()
	{
		ScriptedServerDevice device(authTypesMessage({6}) + ackMessage());
		QCOMPARE(Connection(AuthType(6)).authenticate(device), false);
	}

	void masterSendFailures_data()
	{
		QTest::addColumn<int>("configured");
		QTest::addColumn<int>("writeLimit");

		QTest::newRow("auth type") << int(AuthType::Logon) << 0;
		QTest::newRow("password") << int(AuthType::Logon) << 64;
		QTest::newRow("challenge response") << int(AuthType::KeyFile) << 64;
		QTest::newRow("token") << int(AuthType::Token) << 64;
	}

	void masterSendFailures()
	{
		QFETCH(int, configured);
		QFETCH(int, writeLimit);

		setKeyFileCredentials();
		VeyonCore::authenticationCredentials().setToken(AuthenticationCredentials::Token(tokenValue()));

		const QByteArray serverData = configured == AuthType::KeyFile ? challengeFlow(CryptoCore::generateChallenge()) : (configured == AuthType::Token ? QByteArray(authTypesMessage({AuthType::Token}) + ackMessage()) : logonFlow());
		ScriptedServerDevice device(serverData);
		device.setWriteLimit(writeLimit);
		QCOMPARE(Connection(AuthType(configured)).authenticate(device), false);
	}

	// web API: the type selected through the proxy, with the proxy's credentials
	void masterProxy()
	{
		AuthenticationCredentials credentials;
		credentials.setLogonUsername(QStringLiteral("webapi-user"));
		credentials.setLogonPassword(AuthenticationCredentials::Password(QByteArray("webapi-password")));

		auto proxy = new TestAuthenticationProxy(AuthType::Logon, credentials);
		ScriptedServerDevice device(authTypesMessage({AuthType::KeyFile, AuthType::Logon}) + ackMessage() +
									serverMessage({m_privateKey.toPublicKey().toPEM()}));
		QVERIFY(Connection(AuthType::KeyFile, proxy).authenticate(device));
		QCOMPARE(proxy->authenticationTypes(), (QList<AuthType>{AuthType::KeyFile, AuthType::Logon}));

		const auto messages = clientMessages(device.clientData());
		QCOMPARE(messages.value(0), (QVariantList{int(AuthType::Logon), QStringLiteral("webapi-user")}));
		CryptoCore::SecureArray decryptedPassword;
		QVERIFY(m_privateKey.decrypt(messages.value(1).value(0).toByteArray(), &decryptedPassword, CryptoCore::DefaultEncryptionAlgorithm));
		QCOMPARE(decryptedPassword.toByteArray(), QByteArray("webapi-password"));
	}

	void masterProxyTypeNotOffered_data()
	{
		QTest::addColumn<int>("selected");

		QTest::newRow("not offered") << int(AuthType::KeyFile);
		// no or an invalid method selected through the web API
		QTest::newRow("invalid") << int(AuthType::Invalid);
	}

	void masterProxyTypeNotOffered()
	{
		QFETCH(int, selected);

		auto proxy = new TestAuthenticationProxy(AuthType(selected), VeyonCore::authenticationCredentials());
		ScriptedServerDevice device(logonFlow());
		QCOMPARE(Connection(AuthType::Invalid, proxy).authenticate(device), false);
		QVERIFY(device.clientData().isEmpty());
	}

	// ----- server

	// 4.11 master, key file authentication
	void serverKeyFile()
	{
		VeyonCore::config().setAuthenticationMethod(VeyonCore::AuthenticationMethod::KeyFileAuthentication);
		ServerAuthenticationManager authenticationManager(nullptr);
		ServerConnection server(authenticationManager);

		QCOMPARE(server.startHandshake(), (QVariantList{1, int(AuthType::KeyFile)}));
		server.sendToServer(serverMessage({int(AuthType::KeyFile), QString::fromLatin1(Username)}));
		QCOMPARE(server.receiveFromServer(), QVariantList{});
		const auto challenge = server.receiveFromServer().value_or(QVariantList{}).value(0).toByteArray();
		QCOMPARE(challenge.size(), int(CryptoCore::ChallengeSize));

		server.sendToServer(serverMessage({QString::fromLatin1(KeyName),
										   m_privateKey.signMessage(challenge, CryptoCore::DefaultSignatureAlgorithm)}));
		QCOMPARE(server.readFromServer(4), authResultOk());
		QCOMPARE(server.client.authState(), VncServerClient::AuthState::Successful);
		QCOMPARE(server.client.username(), QString::fromLatin1(Username));
	}

	void serverKeyFileWrongSignature()
	{
		VeyonCore::config().setAuthenticationMethod(VeyonCore::AuthenticationMethod::KeyFileAuthentication);
		ServerAuthenticationManager authenticationManager(nullptr);
		ServerConnection server(authenticationManager);

		QVERIFY(server.startHandshake().has_value());
		server.sendToServer(serverMessage({int(AuthType::KeyFile), QString::fromLatin1(Username)}));
		QVERIFY(server.receiveFromServer().has_value());
		const auto challenge = server.receiveFromServer().value_or(QVariantList{}).value(0).toByteArray();

		auto otherChallenge = challenge;
		otherChallenge[0] = char(otherChallenge[0] ^ 1);
		server.sendToServer(serverMessage({QString::fromLatin1(KeyName),
										   m_privateKey.signMessage(otherChallenge, CryptoCore::DefaultSignatureAlgorithm)}));
		QVERIFY(server.isClosed());
		QCOMPARE(server.client.authState(), VncServerClient::AuthState::Failed);
	}

	// 4.11 master with a token (demo client against a Veyon server offering tokens)
	void serverToken_data()
	{
		QTest::addColumn<QByteArray>("token");
		QTest::addColumn<bool>("accepted");

		QTest::newRow("valid") << tokenValue() << true;
		QTest::newRow("invalid") << QByteArray("other") << false;
	}

	void serverToken()
	{
		QFETCH(QByteArray, token);
		QFETCH(bool, accepted);

		VeyonCore::authenticationCredentials().setToken(AuthenticationCredentials::Token(tokenValue()));
		ServerAuthenticationManager authenticationManager(nullptr);
		ServerConnection server(authenticationManager);

		QCOMPARE(server.startHandshake(), (QVariantList{2, int(AuthType::Logon), int(AuthType::Token)}));
		server.sendToServer(serverMessage({int(AuthType::Token), QString::fromLatin1(Username)}));
		QCOMPARE(server.receiveFromServer(), QVariantList{});
		server.sendToServer(serverMessage({token}));

		if (accepted)
		{
			QCOMPARE(server.readFromServer(4), authResultOk());
		}
		else
		{
			QVERIFY(server.isClosed());
		}
	}

	// 4.11 master, logon authentication (up to the platform's password check)
	void serverLogon()
	{
		ServerAuthenticationManager authenticationManager(nullptr);
		ServerConnection server(authenticationManager);

		QCOMPARE(server.startHandshake(), (QVariantList{1, int(AuthType::Logon)}));
		server.sendToServer(serverMessage({int(AuthType::Logon), QString::fromLatin1(Username)}));
		QCOMPARE(server.receiveFromServer(), QVariantList{});
		auto publicKey = CryptoCore::PublicKey::fromPEM(server.receiveFromServer().value_or(QVariantList{}).value(0).toString());
		QVERIFY(publicKey.canEncrypt());
		QCOMPARE(server.client.authState(), VncServerClient::AuthState::Password);

		const auto encryptedPassword = publicKey.encrypt(CryptoCore::SecureArray(QByteArray(Password)), CryptoCore::DefaultEncryptionAlgorithm);
		server.sendToServer(serverMessage({encryptedPassword.toByteArray()}));
		// no such user here
		QVERIFY(server.isClosed());
		QCOMPARE(server.client.authState(), VncServerClient::AuthState::Failed);
	}

	void serverRejectsUnofferedType()
	{
		ServerAuthenticationManager authenticationManager(nullptr);
		ServerConnection server(authenticationManager);

		QVERIFY(server.startHandshake().has_value());
		server.sendToServer(serverMessage({int(AuthType::KeyFile), QString::fromLatin1(Username)}));
		QVERIFY(server.isClosed());
	}

	// 64 KiB cap for the reply with the chosen type and for every authentication message
	void serverMessageSizeCap_data()
	{
		QTest::addColumn<bool>("authTypeReply");
		QTest::addColumn<qsizetype>("size");
		QTest::addColumn<bool>("accepted");

		for (const auto authTypeReply : {true, false})
		{
			const QByteArray name = authTypeReply ? "auth type reply" : "token message";
			QTest::newRow(QByteArray(name + " below").constData()) << authTypeReply << MaxSize - 1 << true;
			QTest::newRow(QByteArray(name + " at").constData()) << authTypeReply << MaxSize << true;
			QTest::newRow(QByteArray(name + " above").constData()) << authTypeReply << MaxSize + 1 << false;
			QTest::newRow(QByteArray(name + " far above").constData()) << authTypeReply << 16 * MaxSize << false;
		}
	}

	void serverMessageSizeCap()
	{
		QFETCH(bool, authTypeReply);
		QFETCH(qsizetype, size);
		QFETCH(bool, accepted);

		VeyonCore::authenticationCredentials().setToken(AuthenticationCredentials::Token(tokenValue()));
		ServerAuthenticationManager authenticationManager(nullptr);
		ServerConnection server(authenticationManager);
		QVERIFY(server.startHandshake().has_value());

		// trailing data after the expected values is ignored, as before
		server.sendToServer(serverMessage({int(AuthType::Token), QString::fromLatin1(Username)}, authTypeReply ? size : 0));
		if (authTypeReply && accepted == false)
		{
			QVERIFY(server.isClosed());
			QCOMPARE(server.client.authType(), AuthType::Invalid);
			return;
		}

		QCOMPARE(server.receiveFromServer(), QVariantList{});
		server.sendToServer(serverMessage({tokenValue()}, authTypeReply ? 0 : size));
		if (accepted)
		{
			QCOMPARE(server.readFromServer(4), authResultOk());
		}
		else
		{
			QVERIFY(server.isClosed());
			QCOMPARE(server.client.authState(), VncServerClient::AuthState::Token);
		}
	}

	void serverSendsWithinCap()
	{
		QBuffer buffer;
		buffer.open(QBuffer::ReadWrite);
		VariantArrayMessage message(&buffer, VariantArrayMessage::MaxAuthenticationMessageSize);
		message.write(QByteArray(MaxSize, 'x'));
		QCOMPARE(message.send(), false);
		QCOMPARE(buffer.size(), 0);

		// larger caps than the default are not possible
		VariantArrayMessage defaultMessage(&buffer, std::numeric_limits<VariantArrayMessage::MessageSize>::max());
		defaultMessage.write(QByteArray(MaxSize, 'x'));
		QVERIFY(defaultMessage.send());
	}
};


QTEST_GUILESS_MAIN(VeyonAuthenticationTest)
#include "VeyonAuthenticationTest.moc"
