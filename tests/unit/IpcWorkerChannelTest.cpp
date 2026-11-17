// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QLocalSocket>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

#include "FeatureMessage.h"
#include "IpcConnection.h"
#include "IpcServer.h"

#include <memory>
#include <unistd.h>

// Server ⇄ worker channel with a real child process (docs/specs/02-ipc.md §6a): endpoint created
// before the start, credentials via stdin only, PID check, feature messages of 256 KiB (file
// transfer chunks) in both directions. The child is this executable in helper mode and uses the
// same code path as veyon-worker (readFromStandardInput(), connectToEndpoint(), IpcConnection).
namespace {

const auto FeatureUid = QUuid(QStringLiteral("{3d2a9ec8-58ff-4c5b-8f1c-3c3ae8e2bb05}"));
constexpr int PayloadArgument = 0;
constexpr int ChunkSize = 256 * 1024;

int runWorkerHelper(int argc, char** argv)
{
	auto ipc = IpcEndpointCredentials::readFromStandardInput(5000);
	if (ipc.result != IpcStandardInputCredentials::Result::Valid)
	{
		return ipc.result == IpcStandardInputCredentials::Result::Absent ? 3 : 4;
	}

	QCoreApplication app(argc, argv);
	const auto directory = QString::fromLocal8Bit(argv[2]);

	auto socket = IpcConnection::connectToEndpoint(directory, ipc.credentials->endpointName(), 3000);
	if (socket == nullptr)
	{
		return 5;
	}

	IpcConnection connection(socket, IpcChannel::Side::Client, IpcProtocol::Role::Worker, 0,
							 std::move(*ipc.credentials));
	QObject::connect(&connection, &IpcConnection::messageReceived, &app, [&](const QByteArray& data) {
		FeatureMessage message;
		if (message.fromByteArray(data) == false)
		{
			app.exit(6);
			return;
		}
		// echo the payload reversed
		auto payload = message.argument(PayloadArgument).toByteArray();
		std::reverse(payload.begin(), payload.end());
		(void)connection.sendMessage(FeatureMessage(FeatureUid).addArgument(PayloadArgument, payload).toByteArray());
	});
	QObject::connect(&connection, &IpcConnection::closed, &app, [&app]() { app.exit(0); });
	connection.start();

	return app.exec();
}

} // namespace


class IpcWorkerChannelTest : public QObject
{
	Q_OBJECT
private:
	IpcServer::PeerPolicy managedWorkerPolicy() const
	{
		IpcServer::PeerPolicy policy;
		policy.userId = qint64(getuid());
		policy.checkProcessId = true;
		policy.maximumFailedHandshakes = 1;
		return policy;
	}

	static void startWorker(QProcess& process, IpcServer& endpoint, const QString& directory, bool sendCredentials = true)
	{
		process.setProcessChannelMode(QProcess::ForwardedChannels);
		auto line = std::make_shared<Veyon::Crypto::SecureBuffer>(endpoint.credentialsLine());
		QObject::connect(&process, &QProcess::started, &process, [&process, &endpoint, line, sendCredentials]() {
			endpoint.setExpectedProcessId(process.processId());
			if (sendCredentials)
			{
				process.write(line->constData(), line->size());
			}
			line->clear();
			process.closeWriteChannel();
		});
		process.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--worker-helper"), directory});
	}

private Q_SLOTS:
	void roundTripWithLargeFeatureMessage()
	{
		QTemporaryDir directory;
		IpcServer endpoint(IpcProtocol::Role::Worker, 0);
		QVERIFY(endpoint.listen(directory.path(), managedWorkerPolicy()));
		QVERIFY(endpoint.endpointName().startsWith(QStringLiteral("veyon-worker-0-")));

		QByteArray payload(ChunkSize, 0);
		for (int i = 0; i < payload.size(); ++i)
		{
			payload[i] = char(i * 7);
		}

		QList<QByteArray> replies;
		connect(&endpoint, &IpcServer::connectionEstablished, this, [&](IpcConnection* connection) {
			connect(connection, &IpcConnection::messageReceived, this, [&replies](const QByteArray& data) {
				FeatureMessage message;
				if (message.fromByteArray(data) && message.featureUid() == FeatureUid)
				{
					replies.append(message.argument(PayloadArgument).toByteArray());
				}
			});
			for (int i = 0; i < 3; ++i)
			{
				QVERIFY(connection->sendMessage(FeatureMessage(FeatureUid).addArgument(PayloadArgument, payload).toByteArray()));
			}
		});

		QProcess worker;
		startWorker(worker, endpoint, directory.path());
		QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 3, 10000);

		auto expected = payload;
		std::reverse(expected.begin(), expected.end());
		for (const auto& reply : std::as_const(replies))
		{
			QCOMPARE(reply, expected);
		}

		// the listener is closed after the worker connected
		QVERIFY(QFileInfo::exists(IpcServer::endpointAddress(directory.path(), endpoint.endpointName())) == false);

		// closing the endpoint makes the worker quit
		delete endpoint.connection();
		QVERIFY(worker.waitForFinished(5000));
		QCOMPARE(worker.exitCode(), 0);
	}

	void workerWithoutCredentialsExits()
	{
		QTemporaryDir directory;
		IpcServer endpoint(IpcProtocol::Role::Worker, 0);
		QVERIFY(endpoint.listen(directory.path(), managedWorkerPolicy()));

		QProcess worker;
		startWorker(worker, endpoint, directory.path(), false);
		QVERIFY(worker.waitForFinished(10000));
		QCOMPARE(worker.exitCode(), 3);
		QVERIFY(endpoint.connection() == nullptr);
	}

	void workerWithWrongProcessIdRejected()
	{
		QTemporaryDir directory;
		IpcServer endpoint(IpcProtocol::Role::Worker, 0);
		QVERIFY(endpoint.listen(directory.path(), managedWorkerPolicy()));

		QProcess worker;
		worker.setProcessChannelMode(QProcess::ForwardedChannels);
		const auto line = endpoint.credentialsLine();
		connect(&worker, &QProcess::started, this, [&]() {
			endpoint.setExpectedProcessId(worker.processId() + 1);
			worker.write(line.constData(), line.size());
			worker.closeWriteChannel();
		});
		worker.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--worker-helper"), directory.path()});
		QVERIFY(worker.waitForFinished(10000));
		QVERIFY(worker.exitCode() != 0 || endpoint.connection() == nullptr);
		QVERIFY(endpoint.connection() == nullptr);
	}

	void featureMessageSerialization()
	{
		FeatureMessage message(FeatureUid, FeatureMessage::Command::Default);
		message.addArgument(PayloadArgument, QByteArray(ChunkSize, 'x'));
		const auto data = message.toByteArray();
		QVERIFY(data.size() > ChunkSize);
		QVERIFY(data.size() <= int(IpcProtocol::maximumRecordPlaintextSize(IpcProtocol::Role::Worker)));

		FeatureMessage decoded;
		QVERIFY(decoded.fromByteArray(data));
		QCOMPARE(decoded.featureUid(), FeatureUid);
		QCOMPARE(decoded.argument(PayloadArgument).toByteArray(), QByteArray(ChunkSize, 'x'));

		// trailing data or truncated messages are rejected
		QVERIFY(FeatureMessage().fromByteArray(data + QByteArray("x")) == false);
		QVERIFY(FeatureMessage().fromByteArray(data.left(data.size() - 1)) == false);
		QVERIFY(FeatureMessage().fromByteArray(QByteArray()) == false);
	}
};


int main(int argc, char** argv)
{
	if (argc >= 3 && qstrcmp(argv[1], "--worker-helper") == 0)
	{
		return runWorkerHelper(argc, argv);
	}

	QCoreApplication app(argc, argv);
	IpcWorkerChannelTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "IpcWorkerChannelTest.moc"
