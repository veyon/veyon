// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMutex>
#include <QtTest>

#include "IpcAuthenticationRequest.h"
#include "IpcClient.h"
#include "IpcEndpointCredentials.h"
#include "IpcServer.h"
#include "LinuxAuthHelperClient.h"
#include "PlatformUserFunctions.h"
#include "ServiceDataManager.h"
#include "VeyonCore.h"

#include <unistd.h>

// Logon authentication of the session server through the service IPC (docs/specs/02-ipc.md §6b):
// the real platform plugin in a core with service IPC, connected to a real ServiceDataManager
// endpoint whose PAM backend is a fake. There is no system bus here, so a successful check can
// only have taken the IPC path.
class LinuxServiceAuthenticationTest : public QObject
{
	Q_OBJECT
private:
	struct Check
	{
		QString username;
		QByteArray password;
	};

	VeyonCore* m_core = nullptr;
	ServiceDataManager* m_dataManager = nullptr;
	std::shared_ptr<AuthenticationLimiter> m_limiter;

	// written in the IPC thread
	QMutex m_checksMutex;
	QList<Check> m_checks;
	std::atomic<qint64> m_now{1000000};

	QList<Check> checks()
	{
		QMutexLocker locker(&m_checksMutex);
		return m_checks;
	}

	static IpcEndpointCredentials parseLine(const ServiceDataManager::ServerEndpoint& endpoint)
	{
		auto line = endpoint.credentialsLine.clone();
		(void)line.resize(line.size() - 1);
		return *IpcEndpointCredentials::parse(line);
	}

	std::optional<ServiceDataManager::ServerEndpoint> createEndpoint(const IpcServer::PeerPolicy& policy,
																	 IpcServer::ProcessId processId)
	{
		auto endpoint = m_dataManager->createServerEndpoint(VeyonCore::sessionId(), QStringLiteral("seat0"), policy,
															[]() { return true; }, QString{});
		if (endpoint.has_value())
		{
			m_dataManager->setServerProcessId(endpoint->name, processId);
		}
		return endpoint;
	}

	static IpcServer::PeerPolicy serverPolicy()
	{
		IpcServer::PeerPolicy policy;
		policy.userId = 0;
		policy.checkProcessId = true;
		return policy;
	}

	static bool authenticate(const QString& username, const QByteArray& password)
	{
		return VeyonCore::platform().userFunctions().authenticate(username,
																  PlatformUserFunctions::Password(password));
	}

private Q_SLOTS:
	void initTestCase()
	{
		if (geteuid() != 0)
		{
			QSKIP("the service endpoint directory /run/veyon needs root");
		}

		// never the real system bus (LinuxAuthHelperClient uses it)
		qputenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/nonexistent/veyon-test-no-system-bus");

		// the system configuration is never written
		auto flags = VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc;
		flags.setFlag(VeyonCore::InitializationFlag::WritableConfiguration, false);

		VeyonCore::setupApplicationParameters();
		m_core = new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::Server,
							   QStringLiteral("LinuxServiceAuthenticationTest"), flags);

		m_dataManager = new ServiceDataManager;
		m_limiter = AuthenticationLimiter::create([this]() { return m_now.load(); });
		m_dataManager->setAuthenticator(
			[this](const QString& username, Veyon::Crypto::SecureBuffer&& password, QObject* context,
				   const ServiceIpcCommandHandler::AuthenticationResultHandler& resultHandler) {
			Q_UNUSED(context)
			const auto passwordData = password.toUnprotectedByteArray();
			{
				QMutexLocker locker(&m_checksMutex);
				m_checks.append({username, passwordData});
			}
			resultHandler(passwordData == "correct" ? ServiceIpcCommandHandler::AuthenticationResult::Authenticated : ServiceIpcCommandHandler::AuthenticationResult::Failed);
		},
			m_limiter);
	}

	void cleanupTestCase()
	{
		delete m_core;
		delete m_dataManager;
	}

	void init()
	{
		QMutexLocker locker(&m_checksMutex);
		m_checks.clear();
		m_now += AuthenticationLimiter::FailureResetInterval;
	}

	// F5: a process with service IPC capability but without connection (standalone server) does
	// not fall back to D-Bus
	void withoutServiceIpcClient()
	{
		QVERIFY(VeyonCore::serviceIpcClient() == nullptr);
		QVERIFY(authenticate(QStringLiteral("teacher"), "correct") == false);
		QVERIFY(checks().isEmpty());
	}

	void connectToService()
	{
		auto endpoint = createEndpoint(serverPolicy(), getpid());
		QVERIFY(endpoint.has_value());
		QVERIFY(m_core->connectToServiceIpc(parseLine(*endpoint)));
		QVERIFY(VeyonCore::serviceIpcClient() && VeyonCore::serviceIpcClient()->isConnected());
	}

	// the session server's Logon authentication uses the service IPC, for any account
	void serverUsesServiceIpc()
	{
		QVERIFY(authenticate(QStringLiteral("remote-teacher"), "correct"));
		QCOMPARE(checks().size(), 1);
		QCOMPARE(checks()[0].username, QStringLiteral("remote-teacher"));
		QCOMPARE(checks()[0].password, QByteArray("correct"));

		m_now += AuthenticationLimiter::MaximumFailureBackoff;
		QVERIFY(authenticate(QStringLiteral("root"), "wrong") == false);
		QCOMPARE(checks().size(), 2);
	}

	// failed checks of the session are limited in the service
	void serverBackoff()
	{
		QVERIFY(authenticate(QStringLiteral("teacher"), "wrong") == false);
		QVERIFY(authenticate(QStringLiteral("teacher"), "correct") == false);
		QCOMPARE(checks().size(), 1);

		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QVERIFY(authenticate(QStringLiteral("teacher"), "correct"));
		QCOMPARE(checks().size(), 2);
	}

	// invalid credentials never leave the server
	void invalidCredentials()
	{
		QVERIFY(authenticate(QString(), "correct") == false);
		QVERIFY(authenticate(QStringLiteral("teacher"), QByteArray()) == false);
		QVERIFY(authenticate(QString(AuthenticationLimiter::MaximumUsernameSize + 1, QLatin1Char('u')), "correct") ==
				false);
		QVERIFY(checks().isEmpty());
	}

	// malformed and oversized requests over a real connection: answered with bad-request
	void malformedRequests()
	{
		auto* client = VeyonCore::serviceIpcClient();

		const auto noArguments = client->call(IpcProtocol::Command::Authenticate);
		QVERIFY(noArguments.ok == false);
		QCOMPARE(noArguments.error, QStringLiteral("bad-request"));

		const auto oversized = client->call(
			IpcProtocol::Command::Authenticate,
			{{IpcProtocol::Key::Username, QStringLiteral("teacher")},
			 {IpcProtocol::Key::Password, QString::fromLatin1(QByteArray(30000, 'p').toBase64())}},
			IpcAuthenticationRequest::CallTimeout);
		QCOMPARE(oversized.error, QStringLiteral("bad-request"));

		QVERIFY(checks().isEmpty());

		// the connection stays usable
		QVERIFY(authenticate(QStringLiteral("teacher"), "correct"));
	}

	// without a reachable veyon-auth-helper (here: no system bus at all) there is no verdict
	void authHelperUnavailable()
	{
		QObject context;
		std::optional<ServiceIpcCommandHandler::AuthenticationResult> result;
		LinuxAuthHelperClient::authenticate(QStringLiteral("teacher"), Veyon::Crypto::SecureBuffer("pw", 2), &context,
											[&result](ServiceIpcCommandHandler::AuthenticationResult authenticationResult) {
			result = authenticationResult;
		});
		QTRY_VERIFY_WITH_TIMEOUT(result.has_value(), 10000);
		QCOMPARE(*result, ServiceIpcCommandHandler::AuthenticationResult::NoVerdict);
	}

	// a process that is not the started server can't connect, so it never reaches Authenticate
	void peerNotTheServer()
	{
		auto wrongProcess = createEndpoint(serverPolicy(), getpid() + 100000);
		QVERIFY(wrongProcess.has_value());
		IpcClient wrongProcessClient(IpcProtocol::Role::SessionServer, quint32(VeyonCore::sessionId()));
		QVERIFY(wrongProcessClient.connectAndHandshake(IpcServer::serviceEndpointDirectory(),
													   parseLine(*wrongProcess), 1000) == false);
		QVERIFY(IpcAuthenticationRequest::authenticate(wrongProcessClient, QStringLiteral("root"), "correct", 7) ==
				false);
		m_dataManager->removeServerEndpoint(wrongProcess->name);

		auto policy = serverPolicy();
		policy.userId = 65534;
		auto wrongUser = createEndpoint(policy, getpid());
		QVERIFY(wrongUser.has_value());
		IpcClient wrongUserClient(IpcProtocol::Role::SessionServer, quint32(VeyonCore::sessionId()));
		QVERIFY(wrongUserClient.connectAndHandshake(IpcServer::serviceEndpointDirectory(), parseLine(*wrongUser),
													1000) == false);
		m_dataManager->removeServerEndpoint(wrongUser->name);

		QVERIFY(checks().isEmpty());
	}
};


QTEST_GUILESS_MAIN(LinuxServiceAuthenticationTest)
#include "LinuxServiceAuthenticationTest.moc"
