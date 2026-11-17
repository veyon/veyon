// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "ServiceIpcCommandHandler.h"

using Veyon::Crypto::SecureBuffer;

// Service side of the IPC commands (docs/specs/02-ipc.md §6, §8.14)
class ServiceIpcCommandHandlerTest : public QObject
{
	Q_OBJECT
private:
	qint64 m_now = 1000;
	QStringList m_logMessages;

	static QStringList* s_logMessages;

	static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_logMessages->append(message);
	}

	ServiceIpcCommandHandler::Client client(int sessionId, bool loggedOnUser = false,
											const QString& platformSessionId = QStringLiteral("seat0")) const
	{
		ServiceIpcCommandHandler::Client client;
		client.sessionId = sessionId;
		client.platformSessionId = platformSessionId;
		client.hasLoggedOnUser = [loggedOnUser]() { return loggedOnUser; };
		return client;
	}

	static IpcMessage::Request request(QLatin1String command, const QJsonObject& arguments = {})
	{
		static int id = 0;
		return {++id, command, arguments};
	}

	static IpcMessage::Request setRequest(const QByteArray& data)
	{
		return request(IpcProtocol::Command::SetSessionData,
					   {{IpcProtocol::Key::Data, QString::fromLatin1(data.toBase64())}});
	}

	static QByteArray data(const IpcMessage::Response& response)
	{
		return QByteArray::fromBase64(response.result.value(IpcProtocol::Key::Data).toString().toLatin1());
	}

private Q_SLOTS:
	void init()
	{
		m_now = 1000;
		m_logMessages.clear();
		s_logMessages = &m_logMessages;
		qInstallMessageHandler(captureMessage);
	}

	void cleanup()
	{
		qInstallMessageHandler(nullptr);
	}

	void sessionData()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto loginScreen = client(1);
		auto otherSession = client(2);

		const auto setResponse = handler.handleRequest(loginScreen, setRequest("credentials"));
		QVERIFY(setResponse.ok);

		// another session ID gets no data
		const auto otherResponse = handler.handleRequest(otherSession, request(IpcProtocol::Command::GetSessionData));
		QVERIFY(otherResponse.ok);
		QVERIFY(otherResponse.result.isEmpty());

		// own session: once
		const auto firstRead = handler.handleRequest(loginScreen, request(IpcProtocol::Command::GetSessionData));
		QVERIFY(firstRead.ok);
		QCOMPARE(data(firstRead), QByteArray("credentials"));
		const auto secondRead = handler.handleRequest(loginScreen, request(IpcProtocol::Command::GetSessionData));
		QVERIFY(secondRead.ok);
		QVERIFY(secondRead.result.isEmpty());

		// the reading server may be a new server of the same session ID (after a display manager restart)
		QVERIFY(handler.handleRequest(loginScreen, setRequest("again")).ok);
		auto restartedServer = client(1);
		QCOMPARE(data(handler.handleRequest(restartedServer, request(IpcProtocol::Command::GetSessionData))),
				 QByteArray("again"));

		// no secrets in log lines
		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY(message.contains(QLatin1String("credentials")) == false);
			QVERIFY(message.contains(QString::fromLatin1(QByteArray("credentials").toBase64())) == false);
		}
	}

	// F3: a server of another platform session with the same (default) session ID gets nothing,
	// a server of a session with logged-on user can't read either
	void sessionDataBoundToPlatformSession()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto loginScreen = client(0, false, QStringLiteral("seat0"));
		auto otherSeat = client(0, false, QStringLiteral("seat1"));
		auto userSession = client(0, true, QStringLiteral("seat0"));

		QVERIFY(handler.handleRequest(loginScreen, setRequest("credentials")).ok);
		QVERIFY(handler.handleRequest(otherSeat, request(IpcProtocol::Command::GetSessionData)).result.isEmpty());

		const auto userResponse = handler.handleRequest(userSession, request(IpcProtocol::Command::GetSessionData));
		QVERIFY(userResponse.ok == false);
		QCOMPARE(userResponse.error, QStringLiteral("denied"));

		// still available for the login screen server of the right platform session
		auto newLoginScreen = client(0, false, QStringLiteral("seat0"));
		QCOMPARE(data(handler.handleRequest(newLoginScreen, request(IpcProtocol::Command::GetSessionData))),
				 QByteArray("credentials"));

		QVERIFY(handler.handleRequest(loginScreen, setRequest("again")).ok);
		handler.removeSessionData(loginScreen);
		QVERIFY(handler.handleRequest(newLoginScreen, request(IpcProtocol::Command::GetSessionData)).result.isEmpty());
	}

	void sessionDataDeniedWithLoggedOnUser()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto userSession = client(1, true);

		const auto response = handler.handleRequest(userSession, setRequest("credentials"));
		QVERIFY(response.ok == false);
		QCOMPARE(response.error, QStringLiteral("denied"));
		QCOMPARE(handler.handleRequest(userSession, request(IpcProtocol::Command::GetSessionData)).error,
				 QStringLiteral("denied"));

		// no callback ⇒ treated as logged on
		ServiceIpcCommandHandler::Client unknown;
		unknown.sessionId = 3;
		QCOMPARE(handler.handleRequest(unknown, setRequest("credentials")).error, QStringLiteral("denied"));
	}

	void sessionDataExpiry()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto loginScreen = client(1);

		QVERIFY(handler.handleRequest(loginScreen, setRequest("credentials")).ok);
		m_now += IpcProtocol::SessionDataLifetime - 1;
		auto clientCopy = client(1);
		QVERIFY(handler.handleRequest(loginScreen, setRequest("credentials2")).ok);
		m_now += IpcProtocol::SessionDataLifetime;
		QVERIFY(handler.handleRequest(clientCopy, request(IpcProtocol::Command::GetSessionData)).result.isEmpty());

		QVERIFY(handler.handleRequest(loginScreen, setRequest("credentials3")).ok);
		m_now += IpcProtocol::SessionDataLifetime - 1;
		QCOMPARE(data(handler.handleRequest(loginScreen, request(IpcProtocol::Command::GetSessionData))),
				 QByteArray("credentials3"));
	}

	void sessionDataClearAndMalformed()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto loginScreen = client(1);

		QVERIFY(handler.handleRequest(loginScreen, setRequest("credentials")).ok);
		QVERIFY(handler.handleRequest(loginScreen, setRequest({})).ok);
		QVERIFY(handler.handleRequest(loginScreen, request(IpcProtocol::Command::GetSessionData)).result.isEmpty());

		QCOMPARE(handler.handleRequest(loginScreen, request(IpcProtocol::Command::SetSessionData)).error,
				 QStringLiteral("bad-request"));
		QCOMPARE(handler.handleRequest(loginScreen, request(IpcProtocol::Command::SetSessionData, {{IpcProtocol::Key::Data, QStringLiteral("!!!")}}))
					 .error,
				 QStringLiteral("bad-request"));
		QCOMPARE(handler.handleRequest(loginScreen, request(IpcProtocol::Command::SetSessionData, {{IpcProtocol::Key::Data, 1}}))
					 .error,
				 QStringLiteral("bad-request"));
	}

	void unknownCommand()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto server = client(1);
		const auto response = handler.handleRequest(server, request(QLatin1String("ReadData")));
		QVERIFY(response.ok == false);
		QCOMPARE(response.error, QStringLiteral("unknown-command"));
	}

	void tlsHostKey()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });

		// no provider (TLS disabled / no certificate) ⇒ refused
		auto server = client(1);
		QCOMPARE(handler.handleRequest(server, request(IpcProtocol::Command::GetTlsHostKey)).error,
				 QStringLiteral("denied"));

		bool available = true;
		handler.setTlsHostKeyProvider([&available]() -> std::optional<ServiceIpcCommandHandler::TlsHostKey> {
			if (available == false)
			{
				return {};
			}
			return ServiceIpcCommandHandler::TlsHostKey{SecureBuffer::fromByteArray("secret-key-der"),
														"-----BEGIN CERTIFICATE-----"};
		});

		auto freshServer = client(1);
		const auto first = handler.handleRequest(freshServer, request(IpcProtocol::Command::GetTlsHostKey));
		QVERIFY(first.ok);
		QCOMPARE(QByteArray::fromBase64(first.result.value(IpcProtocol::Key::TlsKey).toString().toLatin1()),
				 QByteArray("secret-key-der"));
		QCOMPARE(first.result.value(IpcProtocol::Key::CertificateChain).toString(),
				 QStringLiteral("-----BEGIN CERTIFICATE-----"));
		QVERIFY(handler.handleRequest(freshServer, request(IpcProtocol::Command::GetTlsHostKey)).ok);
		// at most two calls per server start
		QCOMPARE(handler.handleRequest(freshServer, request(IpcProtocol::Command::GetTlsHostKey)).error,
				 QStringLiteral("denied"));

		available = false;
		auto otherServer = client(2);
		QCOMPARE(handler.handleRequest(otherServer, request(IpcProtocol::Command::GetTlsHostKey)).error,
				 QStringLiteral("denied"));

		// compat-bootstrapped servers get the key with a warning (02 §2a.2)
		available = true;
		m_logMessages.clear();
		auto weakServer = client(3);
		weakServer.weakBootstrap = true;
		QVERIFY(handler.handleRequest(weakServer, request(IpcProtocol::Command::GetTlsHostKey)).ok);
		QCOMPARE(m_logMessages.size(), 1);
		QVERIFY(m_logMessages.first().contains(QLatin1String("weak server bootstrap (D-Bus activation): host key exposure possible")));

		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY(message.contains(QLatin1String("secret-key-der")) == false);
		}
	}

	void externalVncServerPassword()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto server = client(1);
		QCOMPARE(handler.handleRequest(server, request(IpcProtocol::Command::GetExternalVncServerPassword)).error,
				 QStringLiteral("denied"));

		handler.setExternalVncServerPasswordProvider([]() -> std::optional<SecureBuffer> {
			return SecureBuffer::fromByteArray("vnc-password");
		});
		const auto response = handler.handleRequest(server, request(IpcProtocol::Command::GetExternalVncServerPassword));
		QVERIFY(response.ok);
		QCOMPARE(QByteArray::fromBase64(response.result.value(IpcProtocol::Key::Password).toString().toLatin1()),
				 QByteArray("vnc-password"));
	}
};

QStringList* ServiceIpcCommandHandlerTest::s_logMessages = nullptr;


QTEST_GUILESS_MAIN(ServiceIpcCommandHandlerTest)
#include "ServiceIpcCommandHandlerTest.moc"
