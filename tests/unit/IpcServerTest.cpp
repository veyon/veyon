// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QLocalSocket>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtTest>

#include "IpcClient.h"
#include "IpcConnection.h"
#include "IpcServer.h"

#include <memory>
#include <sys/stat.h>
#include <unistd.h>

// Endpoint behaviour on Linux with real Unix domain sockets (docs/specs/02-ipc.md §1, §6a, §8.9)
class IpcServerTest : public QObject
{
	Q_OBJECT
private:
	static constexpr quint32 SessionId = 2;

	struct ClientResult
	{
		bool connected = false;
		IpcClient::Response response;
	};

	static IpcEndpointCredentials parseLine(const IpcServer& server)
	{
		auto line = server.credentialsLine();
		// strip the newline as readSecretLine() does
		(void)line.resize(line.size() - 1);
		return *IpcEndpointCredentials::parse(line);
	}

	// runs a blocking IpcClient in its own thread while the server side keeps its event loop
	static ClientResult runClient(const QString& directory, IpcEndpointCredentials&& credentials,
								  IpcProtocol::Role role = IpcProtocol::Role::SessionServer,
								  quint32 sessionId = SessionId)
	{
		ClientResult result;
		auto clientCredentials = std::make_shared<IpcEndpointCredentials>(std::move(credentials));
		std::unique_ptr<QThread> thread(QThread::create([&result, directory, clientCredentials, role, sessionId]() {
			IpcClient client(role, sessionId);
			result.connected = client.connectAndHandshake(directory, std::move(*clientCredentials));
			if (result.connected)
			{
				result.response = client.call(QLatin1String("Echo"), QJsonObject{{QStringLiteral("value"), 42}});
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

	static void serveEcho(IpcServer& server)
	{
		connect(&server, &IpcServer::connectionEstablished, &server, [](IpcConnection* connection) {
			connect(connection, &IpcConnection::messageReceived, connection, [connection](const QByteArray& message) {
				const auto request = IpcMessage::decodeRequest(message);
				if (request.has_value())
				{
					(void)connection->sendMessage(IpcMessage::encodeResponse(IpcMessage::success(request->id, request->arguments)));
				}
			});
		});
	}

	IpcServer::PeerPolicy ownPolicy() const
	{
		IpcServer::PeerPolicy policy;
		policy.userId = qint64(getuid());
		policy.checkProcessId = true;
		return policy;
	}

private Q_SLOTS:
	void establishedConnection()
	{
		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		QVERIFY(server.listen(directory.path(), ownPolicy()));
		server.setExpectedProcessId(QCoreApplication::applicationPid());
		serveEcho(server);

		const auto socketPath = IpcServer::endpointAddress(directory.path(), server.endpointName());
		struct stat status{};
		QCOMPARE(lstat(QFile::encodeName(socketPath).constData(), &status), 0);
		QVERIFY(S_ISSOCK(status.st_mode));
		QCOMPARE(int(status.st_mode & 07777), 0600);

		auto result = runClient(directory.path(), parseLine(server));
		QVERIFY(result.connected);
		QVERIFY(result.response.ok);
		QCOMPARE(result.response.result.value(QStringLiteral("value")).toInt(), 42);
		QVERIFY(server.connection() != nullptr);

		// token is gone and the listener closed (socket file removed) after the first connection
		QVERIFY(server.credentialsLine().isEmpty());
		QVERIFY(QFileInfo::exists(socketPath) == false);
		QVERIFY(server.hasFailed() == false);
	}

	// §8.9: second connection while the first is established (or in its handshake) is refused
	void secondConnectionRefused()
	{
		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		QVERIFY(server.listen(directory.path(), ownPolicy()));
		server.setExpectedProcessId(QCoreApplication::applicationPid());
		serveEcho(server);

		const auto address = IpcServer::endpointAddress(directory.path(), server.endpointName());

		// a silent connection occupies the handshake slot, a concurrent one is refused
		QLocalSocket silentSocket;
		silentSocket.connectToServer(address);
		QVERIFY(silentSocket.waitForConnected(1000));
		QTest::qWait(50);
		QLocalSocket concurrentSocket;
		concurrentSocket.connectToServer(address);
		QTRY_COMPARE(concurrentSocket.state(), QLocalSocket::UnconnectedState);
		// the silent one runs into the handshake deadline
		QTRY_COMPARE_WITH_TIMEOUT(silentSocket.state(), QLocalSocket::UnconnectedState, 5000);

		auto credentials = std::make_shared<IpcEndpointCredentials>(parseLine(server));
		QSemaphore connected;
		QSemaphore release;
		bool firstConnected = false;
		std::unique_ptr<QThread> thread(QThread::create([&]() {
			IpcClient client(IpcProtocol::Role::SessionServer, SessionId);
			firstConnected = client.connectAndHandshake(directory.path(), std::move(*credentials));
			connected.release();
			release.acquire();
		}));
		const auto releaseThread = qScopeGuard([&]() {
			release.release();
			thread->wait();
		});
		thread->start();
		while (connected.available() == 0)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		}
		QVERIFY(firstConnected);
		QVERIFY(server.connection() != nullptr);

		// the listener is closed: no new connections at all
		QLocalSocket secondSocket;
		secondSocket.connectToServer(address);
		QVERIFY(secondSocket.waitForConnected(100) == false);

		QCoreApplication::processEvents();
		QVERIFY(server.connection()->isClosed() == false);
	}

	// asynchronous calls (directory broker, 04-directory-broker.md §5) interleaved with synchronous
	// ones: every response reaches its caller exactly once, also when answered out of order or late
	void asynchronousCalls()
	{
		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		QVERIFY(server.listen(directory.path(), ownPolicy()));
		server.setExpectedProcessId(QCoreApplication::applicationPid());

		connect(&server, &IpcServer::connectionEstablished, &server, [](IpcConnection* connection) {
			connect(connection, &IpcConnection::messageReceived, connection, [connection](const QByteArray& message) {
				const auto request = IpcMessage::decodeRequest(message);
				if (request.has_value() == false)
				{
					return;
				}
				const auto response = IpcMessage::encodeResponse(IpcMessage::success(request->id, request->arguments));
				if (request->command == QLatin1String("Slow"))
				{
					QTimer::singleShot(200, connection, [connection, response]() { (void)connection->sendMessage(response); });
				}
				else
				{
					(void)connection->sendMessage(response);
				}
			});
		});

		auto credentials = std::make_shared<IpcEndpointCredentials>(parseLine(server));
		bool connected = false;
		QList<int> asyncValues;
		int syncValue = 0;
		int timedOutCalls = 0;
		int handlerCalls = 0;
		std::unique_ptr<QThread> thread(QThread::create([&]() {
			IpcClient client(IpcProtocol::Role::SessionServer, SessionId);
			connected = client.connectAndHandshake(directory.path(), std::move(*credentials));
			if (connected == false)
			{
				return;
			}

			QEventLoop loop;
			const auto valueHandler = [&](const IpcClient::Response& response) {
				++handlerCalls;
				asyncValues.append(response.ok ? response.result.value(QStringLiteral("value")).toInt() : -1);
			};
			client.callAsync(QStringLiteral("Slow"), {{QStringLiteral("value"), 1}}, valueHandler);
			client.callAsync(QStringLiteral("Echo"), {{QStringLiteral("value"), 2}}, valueHandler);
			client.callAsync(QStringLiteral("Slow"), {{QStringLiteral("value"), 4}}, [&](const IpcClient::Response& response) {
				++handlerCalls;
				if (response.ok == false && response.error.isEmpty())
				{
					++timedOutCalls;
				}
			}, 50);

			// synchronous call while asynchronous ones are in flight
			syncValue = client.call(QLatin1String("Echo"), {{QStringLiteral("value"), 3}}).result.value(QStringLiteral("value")).toInt();

			QTimer::singleShot(1000, &loop, &QEventLoop::quit);
			loop.exec();
		}));
		thread->start();
		while (thread->isFinished() == false)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		}
		thread->wait();

		QVERIFY(connected);
		QCOMPARE(syncValue, 3);
		QCOMPARE(asyncValues, (QList<int>{2, 1}));
		QCOMPARE(timedOutCalls, 1);
		QCOMPARE(handlerCalls, 3);
	}

	// DoD 3: neither the token nor message contents show up in log output
	void noSecretsInLogs()
	{
		static QStringList logMessages;
		logMessages.clear();
		qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& message) {
			logMessages.append(message);
		});
		const auto restoreHandler = qScopeGuard([]() { qInstallMessageHandler(nullptr); });

		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		QVERIFY(server.listen(directory.path(), ownPolicy()));
		server.setExpectedProcessId(QCoreApplication::applicationPid());
		serveEcho(server);

		auto credentials = parseLine(server);
		const auto tokenHex = QString::fromLatin1(credentials.token().toHex()->toUnprotectedByteArray());

		// failed attempt with a wrong role and a successful one
		QVERIFY(runClient(directory.path(), credentials.clone(), IpcProtocol::Role::Worker).connected == false);
		const auto result = runClient(directory.path(), std::move(credentials));
		QVERIFY(result.connected);
		QVERIFY(logMessages.isEmpty() == false);

		for (const auto& message : std::as_const(logMessages))
		{
			QVERIFY(message.contains(tokenHex, Qt::CaseInsensitive) == false);
			QVERIFY(message.contains(QLatin1String("Echo")) == false);
		}
	}

	void wrongPeer()
	{
		{
			// F5: connections of other processes (wrong pid) are dropped without counting as failed
			// attempts, so they can't make the endpoint of the genuine child fail
			QTemporaryDir directory;
			IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
			QSignalSpy failedSpy(&server, &IpcServer::failed);
			QVERIFY(server.listen(directory.path(), ownPolicy()));
			serveEcho(server);
			server.setExpectedProcessId(QCoreApplication::applicationPid() + 1);
			const auto credentials = parseLine(server);
			for (int i = 0; i < IpcProtocol::MaximumFailedHandshakes * 2; ++i)
			{
				QVERIFY(runClient(directory.path(), credentials.clone()).connected == false);
			}
			QCoreApplication::processEvents();
			QCOMPARE(failedSpy.count(), 0);
			QVERIFY(server.hasFailed() == false);

			server.setExpectedProcessId(QCoreApplication::applicationPid());
			QVERIFY(runClient(directory.path(), credentials.clone()).connected);
		}
		{
			// wrong uid
			QTemporaryDir directory;
			IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
			auto policy = ownPolicy();
			policy.userId = qint64(getuid()) + 1;
			QVERIFY(server.listen(directory.path(), policy));
			server.setExpectedProcessId(QCoreApplication::applicationPid());
			QVERIFY(runClient(directory.path(), parseLine(server)).connected == false);
			QVERIFY(server.connection() == nullptr);
		}
		{
			// F5: on Linux the PID is registered before the token is written, so a connection
			// before the registration is rejected right away (and not counted)
			QTemporaryDir directory;
			IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
			QVERIFY(server.listen(directory.path(), ownPolicy()));
			serveEcho(server);
			QElapsedTimer timer;
			timer.start();
			QVERIFY(runClient(directory.path(), parseLine(server)).connected == false);
			QVERIFY(timer.elapsed() < IpcProtocol::HandshakeTimeout);
			QVERIFY(server.hasFailed() == false);
			server.setExpectedProcessId(QCoreApplication::applicationPid());
			QVERIFY(runClient(directory.path(), parseLine(server)).connected);
		}
	}

	void wrongCredentials()
	{
		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		QSignalSpy failedSpy(&server, &IpcServer::failed);
		QVERIFY(server.listen(directory.path(), ownPolicy()));
		server.setExpectedProcessId(QCoreApplication::applicationPid());
		serveEcho(server);

		// wrong token
		auto line = SecureBuffer::fromByteArray(QByteArray("VEYONIPC1 ") + server.endpointName().toLatin1() + ' ' +
												QByteArray(64, 'a'));
		QVERIFY(runClient(directory.path(), *IpcEndpointCredentials::parse(line)).connected == false);
		// wrong role and wrong session ID
		QVERIFY(runClient(directory.path(), parseLine(server), IpcProtocol::Role::Worker).connected == false);
		QCOMPARE(failedSpy.count(), 0);
		QVERIFY(runClient(directory.path(), parseLine(server), IpcProtocol::Role::SessionServer, SessionId + 1).connected == false);
		QTRY_COMPARE(failedSpy.count(), 1);

		// no further connection after the endpoint failed, even with the right credentials
		QVERIFY(server.credentialsLine().isEmpty());
	}

	void unmanagedWorkerPolicy()
	{
		// unmanaged session workers: uid only
		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::Worker, SessionId);
		auto policy = ownPolicy();
		policy.checkProcessId = false;
		QVERIFY(server.listen(directory.path(), policy));
		serveEcho(server);
		QVERIFY(runClient(directory.path(), parseLine(server), IpcProtocol::Role::Worker).connected);
	}

	// §6a: endpoint directory check (wrong owner/mode ⇒ refused)
	void endpointDirectory()
	{
		QTemporaryDir base;
		const QString privateDirectory = base.path() + QStringLiteral("/veyon");
		QVERIFY(IpcServer::prepareDirectory(privateDirectory, IpcServer::DirectoryType::Private));
		struct stat status{};
		QCOMPARE(lstat(QFile::encodeName(privateDirectory).constData(), &status), 0);
		QCOMPARE(int(status.st_mode & 07777), 0700);
		QVERIFY(IpcServer::prepareDirectory(privateDirectory, IpcServer::DirectoryType::Private));

		QCOMPARE(chmod(QFile::encodeName(privateDirectory).constData(), 0711), 0);
		QVERIFY(IpcServer::prepareDirectory(privateDirectory, IpcServer::DirectoryType::Private) == false);
		QVERIFY(IpcServer::prepareDirectory(privateDirectory, IpcServer::DirectoryType::Shared));

		QCOMPARE(chmod(QFile::encodeName(privateDirectory).constData(), 0733), 0);
		QVERIFY(IpcServer::prepareDirectory(privateDirectory, IpcServer::DirectoryType::Shared) == false);

		// symlink instead of directory
		const QString link = base.path() + QStringLiteral("/link");
		QVERIFY(QFile::link(privateDirectory, link));
		QVERIFY(IpcServer::prepareDirectory(link, IpcServer::DirectoryType::Private) == false);
		QVERIFY(IpcServer::prepareDirectory(link, IpcServer::DirectoryType::Shared) == false);

		const QString sharedDirectory = base.path() + QStringLiteral("/shared");
		QVERIFY(IpcServer::prepareDirectory(sharedDirectory, IpcServer::DirectoryType::Shared));
		QCOMPARE(lstat(QFile::encodeName(sharedDirectory).constData(), &status), 0);
		QCOMPARE(int(status.st_mode & 07777), 0711);

		if (geteuid() == 0)
		{
			QCOMPARE(chown(QFile::encodeName(sharedDirectory).constData(), 65534, gid_t(-1)), 0);
			QVERIFY(IpcServer::prepareDirectory(sharedDirectory, IpcServer::DirectoryType::Shared) == false);
		}

		QVERIFY(IpcServer::prepareDirectory(QStringLiteral("relative/path"), IpcServer::DirectoryType::Private) == false);
	}

	void socketOwner()
	{
		if (geteuid() != 0)
		{
			QSKIP("needs root to chown the socket");
		}

		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		auto policy = ownPolicy();
		policy.socketOwnerUserId = 65534;
		QVERIFY(server.listen(directory.path(), policy));
		struct stat status{};
		QCOMPARE(lstat(QFile::encodeName(IpcServer::endpointAddress(directory.path(), server.endpointName())).constData(),
					   &status),
				 0);
		QCOMPARE(int(status.st_uid), 65534);
		QCOMPARE(int(status.st_mode & 07777), 0600);
	}

	void garbageBeforeHandshake()
	{
		QTemporaryDir directory;
		IpcServer server(IpcProtocol::Role::SessionServer, SessionId);
		QVERIFY(server.listen(directory.path(), ownPolicy()));
		server.setExpectedProcessId(QCoreApplication::applicationPid());

		QLocalSocket socket;
		socket.connectToServer(IpcServer::endpointAddress(directory.path(), server.endpointName()));
		QVERIFY(socket.waitForConnected(1000));
		socket.write(QByteArray(1024 * 1024, 'x'));
		socket.flush();
		QTRY_COMPARE(socket.state(), QLocalSocket::UnconnectedState);
		QVERIFY(server.connection() == nullptr);
	}

private:
	using SecureBuffer = Veyon::Crypto::SecureBuffer;
};


QTEST_GUILESS_MAIN(IpcServerTest)
#include "IpcServerTest.moc"
