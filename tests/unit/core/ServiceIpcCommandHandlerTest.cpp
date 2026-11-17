// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "IpcAuthenticationRequest.h"
#include "ServiceIpcCommandHandler.h"

#include <memory>

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
		client.tlsHostKeyAllowed = true;
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

	// fake PAM backend: answers when complete() is called
	struct FakeAuthenticator
	{
		struct Call
		{
			QString username;
			QByteArray password;
			QObject* context = nullptr;
			ServiceIpcCommandHandler::AuthenticationResultHandler resultHandler;
		};

		QList<Call> calls;

		ServiceIpcCommandHandler::Authenticator authenticator()
		{
			return [this](const QString& username, SecureBuffer&& password, QObject* context,
						  const ServiceIpcCommandHandler::AuthenticationResultHandler& resultHandler) {
				calls.append({username, password.toUnprotectedByteArray(), context, resultHandler});
			};
		}

		void complete(int index, ServiceIpcCommandHandler::AuthenticationResult result)
		{
			auto resultHandler = calls[index].resultHandler;
			calls[index].resultHandler = nullptr;
			resultHandler(result);
		}

		void complete(int index, bool authenticated)
		{
			complete(index, authenticated ? ServiceIpcCommandHandler::AuthenticationResult::Authenticated : ServiceIpcCommandHandler::AuthenticationResult::Failed);
		}
	};

	static IpcMessage::Request authenticateRequest(const QString& username, const QByteArray& password)
	{
		return request(IpcProtocol::Command::Authenticate,
					   IpcAuthenticationRequest::encode(username, password.constData(), password.size()));
	}

	// collects the responses of the asynchronous overload
	struct Responses
	{
		QList<IpcMessage::Response> received;

		ServiceIpcCommandHandler::ResponseHandler handler()
		{
			return [this](const IpcMessage::Response& response) { received.append(response); };
		}
	};

	static bool isAuthenticated(const IpcMessage::Response& response)
	{
		return response.ok && response.result.value(IpcProtocol::Key::Authenticated).toBool();
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

		// a server whose memory may be readable by other processes (T-PROC-02) never gets the key,
		// which is also the default
		auto exposedServer = client(4);
		exposedServer.tlsHostKeyAllowed = false;
		QCOMPARE(handler.handleRequest(exposedServer, request(IpcProtocol::Command::GetTlsHostKey)).error,
				 QStringLiteral("denied"));
		QVERIFY(m_logMessages.last().contains(QLatin1String("not handing out the TLS host key")));

		auto defaultClient = ServiceIpcCommandHandler::Client{};
		defaultClient.sessionId = 5;
		QCOMPARE(handler.handleRequest(defaultClient, request(IpcProtocol::Command::GetTlsHostKey)).error,
				 QStringLiteral("denied"));

		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY(message.contains(QLatin1String("secret-key-der")) == false);
		}
	}

	void authenticate()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		FakeAuthenticator backend;
		handler.setAuthenticator(backend.authenticator(), AuthenticationLimiter::create([this]() { return m_now; }));

		auto server = client(1, true);
		QObject context;
		Responses responses;

		const auto authenticateTeacher = authenticateRequest(QStringLiteral("teacher"), "s3cr3t-pw");
		handler.handleRequest(server, authenticateTeacher, &context, responses.handler());

		// answered asynchronously
		QVERIFY(responses.received.isEmpty());
		QCOMPARE(backend.calls.size(), 1);
		QCOMPARE(backend.calls[0].username, QStringLiteral("teacher"));
		QCOMPARE(backend.calls[0].password, QByteArray("s3cr3t-pw"));
		QCOMPARE(backend.calls[0].context, &context);

		backend.complete(0, true);
		QCOMPARE(responses.received.size(), 1);
		QCOMPARE(responses.received[0].id, authenticateTeacher.id);
		QVERIFY(isAuthenticated(responses.received[0]));

		// failed check: ok with authenticated = false
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "wrong"), &context,
							  responses.handler());
		backend.complete(1, false);
		QCOMPARE(responses.received.size(), 2);
		QVERIFY(responses.received[1].ok);
		QCOMPARE(responses.received[1].result.value(IpcProtocol::Key::Authenticated), QJsonValue(false));

		// synchronous commands are answered immediately through the handler as well
		handler.handleRequest(server, request(QLatin1String("ReadData")), &context, responses.handler());
		QCOMPARE(responses.received.size(), 3);
		QCOMPARE(responses.received[2].error, QStringLiteral("unknown-command"));

		// the password never appears in log lines
		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY(message.contains(QLatin1String("s3cr3t-pw")) == false);
			QVERIFY(message.contains(QString::fromLatin1(QByteArray("s3cr3t-pw").toBase64())) == false);
		}
	}

	void authenticateWithoutAuthenticator()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		auto server = client(1, true);
		QObject context;
		Responses responses;

		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "secret"), &context,
							  responses.handler());
		QCOMPARE(responses.received.size(), 1);
		QCOMPARE(responses.received[0].error, QStringLiteral("denied"));

		// the synchronous overload can't answer it
		QCOMPARE(handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "secret")).error,
				 QStringLiteral("denied"));
	}

	void authenticateMalformed_data()
	{
		QTest::addColumn<QJsonObject>("arguments");

		const auto password = QString::fromLatin1(QByteArray("secret").toBase64());
		const auto username = QStringLiteral("teacher");

		QTest::newRow("no arguments") << QJsonObject{};
		QTest::newRow("no username") << QJsonObject{{IpcProtocol::Key::Password, password}};
		QTest::newRow("no password") << QJsonObject{{IpcProtocol::Key::Username, username}};
		QTest::newRow("username not a string")
			<< QJsonObject{{IpcProtocol::Key::Username, 1}, {IpcProtocol::Key::Password, password}};
		QTest::newRow("password not a string")
			<< QJsonObject{{IpcProtocol::Key::Username, username}, {IpcProtocol::Key::Password, QJsonArray{1}}};
		QTest::newRow("extra member") << QJsonObject{{IpcProtocol::Key::Username, username},
													 {IpcProtocol::Key::Password, password},
													 {QStringLiteral("service"), QStringLiteral("login")}};
		QTest::newRow("invalid base64")
			<< QJsonObject{{IpcProtocol::Key::Username, username}, {IpcProtocol::Key::Password, QStringLiteral("!!!!")}};
		QTest::newRow("base64 without padding")
			<< QJsonObject{{IpcProtocol::Key::Username, username}, {IpcProtocol::Key::Password, QStringLiteral("c2VjcmV0").left(7)}};
		QTest::newRow("empty username")
			<< QJsonObject{{IpcProtocol::Key::Username, QString()}, {IpcProtocol::Key::Password, password}};
		QTest::newRow("empty password")
			<< QJsonObject{{IpcProtocol::Key::Username, username}, {IpcProtocol::Key::Password, QString()}};
		QTest::newRow("control character in username")
			<< QJsonObject{{IpcProtocol::Key::Username, QStringLiteral("tea\ncher")}, {IpcProtocol::Key::Password, password}};
		QTest::newRow("NUL in password")
			<< QJsonObject{{IpcProtocol::Key::Username, username},
						   {IpcProtocol::Key::Password, QString::fromLatin1(QByteArray("sec\0ret", 7).toBase64())}};
		QTest::newRow("oversized username")
			<< QJsonObject{{IpcProtocol::Key::Username, QString(AuthenticationLimiter::MaximumUsernameSize + 1, QLatin1Char('u'))},
						   {IpcProtocol::Key::Password, password}};
		QTest::newRow("oversized password")
			<< QJsonObject{{IpcProtocol::Key::Username, username},
						   {IpcProtocol::Key::Password,
							QString::fromLatin1(QByteArray(AuthenticationLimiter::MaximumPasswordSize + 1, 'p').toBase64())}};
		// rejected by length before decoding
		QTest::newRow("huge encoded password")
			<< QJsonObject{{IpcProtocol::Key::Username, username}, {IpcProtocol::Key::Password, QString(60000, QLatin1Char('A'))}};
	}

	void authenticateMalformed()
	{
		QFETCH(QJsonObject, arguments);

		ServiceIpcCommandHandler handler([this]() { return m_now; });
		FakeAuthenticator backend;
		handler.setAuthenticator(backend.authenticator(), AuthenticationLimiter::create([this]() { return m_now; }));
		auto server = client(1, true);
		QObject context;
		Responses responses;

		handler.handleRequest(server, request(IpcProtocol::Command::Authenticate, arguments), &context,
							  responses.handler());
		QCOMPARE(responses.received.size(), 1);
		QCOMPARE(responses.received[0].error, QStringLiteral("bad-request"));
		QVERIFY(backend.calls.isEmpty());

		// a malformed request does not count as failed check
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "secret"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 1);
	}

	void authenticateEncodeRejectsInvalid()
	{
		QVERIFY(IpcAuthenticationRequest::encode(QString(), "secret", 6).isEmpty());
		QVERIFY(IpcAuthenticationRequest::encode(QStringLiteral("teacher"), "", 0).isEmpty());

		const auto encoded = IpcAuthenticationRequest::encode(QStringLiteral("teacher"), "secret", 6);
		const auto decoded = IpcAuthenticationRequest::decode(encoded);
		QVERIFY(decoded.has_value());
		QCOMPARE(decoded->username, QStringLiteral("teacher"));
		QCOMPARE(decoded->password.toUnprotectedByteArray(), QByteArray("secret"));

		// maximum sizes round-trip
		const QByteArray longPassword(AuthenticationLimiter::MaximumPasswordSize, '\xff');
		const QString longUsername(AuthenticationLimiter::MaximumUsernameSize, QLatin1Char('u'));
		const auto longDecoded = IpcAuthenticationRequest::decode(
			IpcAuthenticationRequest::encode(longUsername, longPassword.constData(), longPassword.size()));
		QVERIFY(longDecoded.has_value());
		QCOMPARE(longDecoded->password.toUnprotectedByteArray(), longPassword);
	}

	void authenticateRateLimitedPerSession()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		FakeAuthenticator backend;
		const auto limiter = AuthenticationLimiter::create([this]() { return m_now; });
		handler.setAuthenticator(backend.authenticator(), limiter);

		auto server = client(1, true);
		auto otherSeatServer = client(1, true, QStringLiteral("seat1"));
		QObject context;
		Responses responses;

		// at most one outstanding request per server session
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "wrong"), &context,
							  responses.handler());
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "wrong2"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 1);
		QCOMPARE(responses.received.size(), 1);
		QCOMPARE(responses.received[0].error, QStringLiteral("rate-limited"));

		// another session has its own budget
		handler.handleRequest(otherSeatServer, authenticateRequest(QStringLiteral("teacher"), "pw"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 2);
		backend.complete(1, true);

		// failure ⇒ backoff
		backend.complete(0, false);
		responses.received.clear();
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "again"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 2);
		QCOMPARE(responses.received.last().error, QStringLiteral("rate-limited"));

		m_now += AuthenticationLimiter::InitialFailureBackoff;
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "again"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 3);

		// a server restarted in the same session keeps the budget
		auto restartedServer = client(1, true);
		handler.handleRequest(restartedServer, authenticateRequest(QStringLiteral("teacher"), "again"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 3);
		QCOMPARE(responses.received.last().error, QStringLiteral("rate-limited"));

		// rejections are logged once per window
		int rejectionLogLines = 0;
		for (const auto& message : std::as_const(m_logMessages))
		{
			if (message.contains(QLatin1String("rejecting authentication request")))
			{
				++rejectionLogLines;
			}
		}
		QCOMPARE(rejectionLogLines, 1);
	}

	// F1 over the IPC path (remote Logon auth): a successful check of the attacker's own account
	// does not reset the backoff of guesses for another account
	void authenticateAlternatingAccounts()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		FakeAuthenticator backend;
		handler.setAuthenticator(backend.authenticator(), AuthenticationLimiter::create([this]() { return m_now; }));
		auto server = client(1, true);
		QObject context;
		Responses responses;

		handler.handleRequest(server, authenticateRequest(QStringLiteral("victim"), "guess1"), &context,
							  responses.handler());
		backend.complete(0, false);
		m_now += AuthenticationLimiter::InitialFailureBackoff;

		handler.handleRequest(server, authenticateRequest(QStringLiteral("student"), "own"), &context,
							  responses.handler());
		backend.complete(1, true);

		handler.handleRequest(server, authenticateRequest(QStringLiteral("victim"), "guess2"), &context,
							  responses.handler());
		backend.complete(2, false);

		// second failure in a row: 2 s, not 1 s
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		handler.handleRequest(server, authenticateRequest(QStringLiteral("student"), "own"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 3);
		QCOMPARE(responses.received.last().error, QStringLiteral("rate-limited"));
	}

	// F4: a helper error or timeout answers "not authenticated" without a backoff
	void authenticateWithoutVerdict()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		FakeAuthenticator backend;
		const auto limiter = AuthenticationLimiter::create([this]() { return m_now; });
		handler.setAuthenticator(backend.authenticator(), limiter);
		auto server = client(1, true);
		QObject context;
		Responses responses;

		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "pw"), &context,
							  responses.handler());
		backend.complete(0, ServiceIpcCommandHandler::AuthenticationResult::NoVerdict);
		QCOMPARE(responses.received.size(), 1);
		QVERIFY(responses.received[0].ok);
		QCOMPARE(responses.received[0].result.value(IpcProtocol::Key::Authenticated), QJsonValue(false));
		QCOMPARE(limiter->runningCount(), 0);

		// no backoff: the next request is admitted right away
		handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "pw"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), 2);
	}

	void authenticateConcurrencyLimit()
	{
		ServiceIpcCommandHandler handler([this]() { return m_now; });
		FakeAuthenticator backend;
		const auto limiter = AuthenticationLimiter::create([this]() { return m_now; });
		handler.setAuthenticator(backend.authenticator(), limiter);
		QObject context;
		Responses responses;

		QList<ServiceIpcCommandHandler::Client> servers;
		for (int i = 0; i <= AuthenticationLimiter::MaximumConcurrentAuthentications; ++i)
		{
			servers.append(client(i, true));
		}

		for (auto& server : servers)
		{
			handler.handleRequest(server, authenticateRequest(QStringLiteral("teacher"), "pw"), &context,
								  responses.handler());
		}

		QCOMPARE(backend.calls.size(), AuthenticationLimiter::MaximumConcurrentAuthentications);
		QCOMPARE(responses.received.size(), 1);
		QCOMPARE(responses.received[0].error, QStringLiteral("rate-limited"));

		// a result handler that is destroyed unanswered (context gone) frees its slot
		backend.calls.removeFirst();
		QCOMPARE(limiter->runningCount(), AuthenticationLimiter::MaximumConcurrentAuthentications - 1);
		handler.handleRequest(servers.last(), authenticateRequest(QStringLiteral("teacher"), "pw"), &context,
							  responses.handler());
		QCOMPARE(backend.calls.size(), AuthenticationLimiter::MaximumConcurrentAuthentications);
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
