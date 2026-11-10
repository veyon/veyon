#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QProcessEnvironment>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTemporaryDir>
#include <QThread>
#include <QtEndian>

#include <csignal>

#include "GreeterVncServer.h"
#include "CryptoCore.h"
#include "rfb/rfbproto.h"

class GreeterVncServerTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void x11UsesNativeCaptureThenFallback();
	void waylandUsesKWinCaptureThenFallback();
	void failedKWinCaptureKeepsTheLoginFallback();
	void otherSessionTypesDoNotUseDesktopPortals_data();
	void otherSessionTypesDoNotUseDesktopPortals();
	void failedCaptureCanFallBackWithPrivatePassword_data();
	void failedCaptureCanFallBackWithPrivatePassword();
	void hungBackendHasBoundedStartup();
	void unauthenticatedListenerIsNotReady();
	void transientCaptureFailureIsRetriedWithFreshEnvironment();
	void readyWaylandEnvironmentReplacesThePlaceholder();
};

void GreeterVncServerTest::x11UsesNativeCaptureThenFallback()
{
	const QList<GreeterVncServer::Backend> expected{GreeterVncServer::Backend::X11, GreeterVncServer::Backend::Headless};
	QVERIFY(GreeterVncServer::backendsForSession(QStringLiteral("x11")) == expected);
	QVERIFY(GreeterVncServer::backendsForSession(QStringLiteral("X11")) == expected);
}

void GreeterVncServerTest::otherSessionTypesDoNotUseDesktopPortals_data()
{
	QTest::addColumn<QString>("sessionType");
	QTest::newRow("unknown") << QStringLiteral("unspecified");
	QTest::newRow("empty") << QString();
}

void GreeterVncServerTest::waylandUsesKWinCaptureThenFallback()
{
	const QList<GreeterVncServer::Backend> expected{GreeterVncServer::Backend::KWin, GreeterVncServer::Backend::Headless};
	QVERIFY(GreeterVncServer::backendsForSession(QStringLiteral("wayland")) == expected);
	QCOMPARE(GreeterVncServer::backendName(GreeterVncServer::Backend::KWin), QStringLiteral("kwin"));
}

void GreeterVncServerTest::otherSessionTypesDoNotUseDesktopPortals()
{
	QFETCH(QString, sessionType);
	const QList<GreeterVncServer::Backend> expected{GreeterVncServer::Backend::Headless};
	QVERIFY(GreeterVncServer::backendsForSession(sessionType) == expected);
}

static quint16 unusedPort()
{
	QTcpServer reservation;
	if (!reservation.listen(QHostAddress::LocalHost, 0))
	{
		return 0;
	}
	return reservation.serverPort();
}

void GreeterVncServerTest::failedKWinCaptureKeepsTheLoginFallback()
{
	const auto port = unusedPort();
	QVERIFY(port != 0);
	QVERIFY(GreeterVncServer::runForSession(QCoreApplication::applicationFilePath(),
		QStringLiteral("wayland"), port, "testpass", 2000));
}

void GreeterVncServerTest::failedCaptureCanFallBackWithPrivatePassword_data()
{
	QTest::addColumn<QByteArray>("password");
	QTest::newRow("exit") << QByteArray("testpass");
	QTest::newRow("crash") << QByteArray("crashpwd");
}

void GreeterVncServerTest::failedCaptureCanFallBackWithPrivatePassword()
{
	QFETCH(QByteArray, password);
	const auto port = unusedPort();
	QVERIFY(port != 0);
	const auto program = QCoreApplication::applicationFilePath();
	// Native capture exits/crashes before opening its listener. The real
	// orchestration must start the fallback, which validates stdin transport
	// and serves an authenticated RFB handshake on the same port.
	QVERIFY(GreeterVncServer::runForSession(program, QStringLiteral("x11"), port, password, 2000));
}

void GreeterVncServerTest::hungBackendHasBoundedStartup()
{
	const auto port = unusedPort();
	QVERIFY(port != 0);
	QElapsedTimer elapsed;
	elapsed.start();
	QVERIFY(!GreeterVncServer::run(QCoreApplication::applicationFilePath(),
			GreeterVncServer::Backend::Headless, port, "hangpass", 150));
	QVERIFY(elapsed.elapsed() < 3000);
}

void GreeterVncServerTest::unauthenticatedListenerIsNotReady()
{
	const auto port = unusedPort();
	QVERIFY(port != 0);
	QVERIFY(!GreeterVncServer::run(QCoreApplication::applicationFilePath(),
			GreeterVncServer::Backend::Headless, port, "nonepass", 500));
}

void GreeterVncServerTest::transientCaptureFailureIsRetriedWithFreshEnvironment()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const auto port = unusedPort();
	QVERIFY(port != 0);
	int snapshots = 0;
	const auto environment = [&]() {
		auto result = QProcessEnvironment::systemEnvironment();
		result.insert(QStringLiteral("VEYON_CAPTURE_ATTEMPT"), QString::number(++snapshots));
		result.insert(QStringLiteral("VEYON_CAPTURE_RESULT"), directory.filePath(QStringLiteral("backend")));
		return result;
	};
	QVERIFY(GreeterVncServer::runForSession(QCoreApplication::applicationFilePath(),
		QStringLiteral("x11"), port, "retrypwd", 2000, environment));
	QFile result(directory.filePath(QStringLiteral("backend")));
	QVERIFY(result.open(QIODevice::ReadOnly));
	QCOMPARE(result.readAll(), QByteArrayLiteral("x11"));
}

void GreeterVncServerTest::readyWaylandEnvironmentReplacesThePlaceholder()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const auto port = unusedPort();
	QVERIFY(port != 0);
	int snapshots = 0;
	const auto environment = [&]() {
		auto result = QProcessEnvironment::systemEnvironment();
		result.remove(QStringLiteral("WAYLAND_DISPLAY"));
		result.insert(QStringLiteral("XDG_RUNTIME_DIR"), directory.path());
		if (++snapshots > 3) { result.insert(QStringLiteral("WAYLAND_DISPLAY"), QStringLiteral("wayland-late")); }
		result.insert(QStringLiteral("VEYON_CAPTURE_RESULT"), directory.filePath(QStringLiteral("backend")));
		return result;
	};
	QVERIFY(GreeterVncServer::runForSession(QCoreApplication::applicationFilePath(),
		QStringLiteral("wayland"), port, "latepwd1", 2000, environment));
	QFile result(directory.filePath(QStringLiteral("backend")));
	QVERIFY(result.open(QIODevice::ReadOnly));
	QCOMPARE(result.readAll(), QByteArrayLiteral("headless\nkwin\n"));
}

static QByteArray readExactly(QTcpSocket* socket, int size)
{
	QByteArray data;
	QElapsedTimer elapsed;
	elapsed.start();
	while (data.size() < size && elapsed.elapsed() < 2000)
	{
		data += socket->read(size - data.size());
		if (data.size() < size && !socket->waitForReadyRead(200))
		{
			break;
		}
	}
	return data;
}

static int runBackendFixture(const QStringList& arguments)
{
	if (arguments.size() != 4)
	{
		return 41;
	}
	QFile input;
	if (!input.open(stdin, QIODevice::ReadOnly))
	{
		return 42;
	}
	const auto password = input.read(8);
	if (password == "hangpass")
	{
		QThread::msleep(5000);
		return 43;
	}
	if (password != "testpass" && password != "crashpwd" && password != "nonepass" && password != "retrypwd" && password != "latepwd1")
	{
		return 44;
	}
	for (const auto& variable : QProcessEnvironment::systemEnvironment().toStringList())
	{
		if (variable.contains(QString::fromLatin1(password)))
		{
			return 48;
		}
	}
	if (arguments[2] == QLatin1String("kwin") &&
		(password != "latepwd1" || qEnvironmentVariable("WAYLAND_DISPLAY").isEmpty())) { return 45; }
	if (arguments[2] == QLatin1String("x11"))
	{
		if (password == "crashpwd")
		{
			std::raise(SIGKILL);
		}
		if (password != "retrypwd" || qEnvironmentVariableIntValue("VEYON_CAPTURE_ATTEMPT") < 2)
		{
			return 45;
		}
	}
	else if (password == "retrypwd")
	{
		// Do not let the fallback satisfy the native-recovery assertion.
		return 54;
	}
	if (password == "latepwd1" && arguments[2] == QLatin1String("headless"))
	{
		QFile previous(qEnvironmentVariable("VEYON_CAPTURE_RESULT"));
		if (previous.open(QIODevice::ReadOnly) && previous.readAll().contains("kwin\n")) { return 54; }
	}
	QTcpServer server;
	if (!server.listen(QHostAddress::LocalHost, arguments[3].toUShort()) || !server.waitForNewConnection(2000))
	{
		return 46;
	}
	auto socket = server.nextPendingConnection();
	socket->write("RFB 003.008\n");
	socket->flush();
	if (readExactly(socket, 12) != "RFB 003.008\n") { return 49; }
	const bool noAuthentication = password == "nonepass";
	socket->write(noAuthentication ? QByteArray::fromHex("0101") : QByteArray::fromHex("0102"));
	socket->flush();
	if (readExactly(socket, 1) != QByteArray(1, noAuthentication ? '\1' : '\2')) { return 50; }
	if (!noAuthentication)
	{
		socket->write(QByteArray(16, 'x'));
		socket->flush();
		if (readExactly(socket, 16).size() != 16) { return 51; }
	}
	socket->write(QByteArray(4, '\0'));
	socket->flush();
	if (readExactly(socket, 1).size() != 1) { return 52; }
	rfbServerInitMsg init{};
	init.framebufferWidth = qToBigEndian<quint16>(640);
	init.framebufferHeight = qToBigEndian<quint16>(480);
	init.format.bitsPerPixel = 32;
	init.format.depth = 24;
	init.format.trueColour = 1;
	init.format.redMax = init.format.greenMax = init.format.blueMax = qToBigEndian<quint16>(255);
	init.format.redShift = 16;
	init.format.greenShift = 8;
	socket->write(reinterpret_cast<const char*>(&init), sz_rfbServerInitMsg);
	if (!socket->waitForBytesWritten(2000)) { return 53; }
	if (password == "retrypwd")
	{
		QFile result(qEnvironmentVariable("VEYON_CAPTURE_RESULT"));
		if (!result.open(QIODevice::WriteOnly) || result.write("x11") != 3) { return 55; }
		result.close();
	}
	if (password == "latepwd1")
	{
		QFile result(qEnvironmentVariable("VEYON_CAPTURE_RESULT"));
		if (!result.open(QIODevice::Append)) { return 55; }
		result.write(arguments[2].toUtf8() + '\n');
		result.close();
		if (arguments[2] == QLatin1String("headless")) { QThread::msleep(10000); }
	}
	return 0;
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	QCA::Initializer qca;
	if (app.arguments().value(1) == QLatin1String("--greeter-vnc"))
	{
		return runBackendFixture(app.arguments());
	}
	GreeterVncServerTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "GreeterVncServerTest.moc"
