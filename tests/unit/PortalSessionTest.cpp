// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QDBusVirtualObject>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <fcntl.h>
#include <unistd.h>
#include "PortalSession.h"

// Separate process: PortalSession synchronously obtains the PipeWire FD, so its
// fake service must keep processing D-Bus independently of the test event loop.
class FakeSessionPortal : public QDBusVirtualObject
{
public:
	QString introspect(const QString&) const override { return {}; }
	bool handleMessage(const QDBusMessage& message, const QDBusConnection& bus) override
	{
		const auto method = message.member();
		if (method == QStringLiteral("Close")) {
			m_closed.append(message.path());bus.send(message.createReply());return true;
		}
		if (method == QStringLiteral("NotifyKeyboardKeysym")) {
			const auto session = qvariant_cast<QDBusObjectPath>(message.arguments().first()).path();
			if (session.endsWith(QStringLiteral("/1"))) {
				bus.send(message.createErrorReply(QDBusError::AccessDenied, QStringLiteral("Input session revoked")));
			} else {
				m_inputSessions.append(session);bus.send(message.createReply());
			}
			return true;
		}
		if (method == QStringLiteral("State")) {
			bus.send(message.createReply({m_closed, m_inputSessions}));return true;
		}
		if (method == QStringLiteral("OpenPipeWireRemote")) {
			const int fd = ::open("/dev/null", O_RDONLY);
			QDBusUnixFileDescriptor descriptor(fd);::close(fd);
			bus.send(message.createReply({QVariant::fromValue(descriptor)}));return true;
		}
		if (method != QStringLiteral("CreateSession") && method != QStringLiteral("SelectDevices") &&
			method != QStringLiteral("SelectSources") && method != QStringLiteral("Start")) return false;
		const auto options = qdbus_cast<QVariantMap>(message.arguments().last());
		const auto sender = QString(message.service()).remove(QLatin1Char(':')).replace(QLatin1Char('.'), QLatin1Char('_'));
		const auto path = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
			.arg(sender, options.value(QStringLiteral("handle_token")).toString());
		QVariantMap results;
		if (method == QStringLiteral("CreateSession")) {
			results.insert(QStringLiteral("session_handle"), QStringLiteral("/org/freedesktop/portal/desktop/session/test/%1").arg(++m_sessions));
		}
		bus.send(message.createReply({QVariant::fromValue(QDBusObjectPath(path))}));
		QTimer::singleShot(10, this, [bus, path, results]() {
			auto response = QDBusMessage::createSignal(path, QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"));
			response.setArguments({0u, results});bus.send(response);
		});
		return true;
	}
private:
	int m_sessions = 0;
	QStringList m_closed, m_inputSessions;
};
class PortalSessionTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void failedInputClosesSessionAndRestartAcceptsKeys()
	{
		QTemporaryDir home;QVERIFY(home.isValid());
		const auto previousPath = qgetenv("PATH"), previousState = qgetenv("XDG_STATE_HOME");
		struct Restore { QByteArray path, state;~Restore() { qputenv("PATH", path);qputenv("XDG_STATE_HOME", state); } } restore{previousPath, previousState};
		// Keep the constructor's optional flatpak preauthorization inside this fixture.
		QFile flatpak(home.filePath(QStringLiteral("flatpak")));QVERIFY(flatpak.open(QIODevice::WriteOnly));
		flatpak.write("#!/bin/sh\nexit 0\n");flatpak.close();QVERIFY(flatpak.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
		qputenv("PATH", home.path().toUtf8() + ':' + previousPath);qputenv("XDG_STATE_HOME", home.path().toUtf8());
		QProcess server;
		struct Stop { QProcess& process;~Stop() { process.terminate();if (!process.waitForFinished(2000)) {process.kill();process.waitForFinished();} } } stop{server};
		server.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--fake-portal")});
		QVERIFY(server.waitForStarted());QVERIFY(server.waitForReadyRead());QCOMPARE(server.readAllStandardOutput().trimmed(), QByteArrayLiteral("ready"));
		PortalSession session(false);
		QSignalSpy started(&session, &PortalSession::started), failed(&session, &PortalSession::failed);
		session.start();QTRY_COMPARE(started.size(), 1);QCOMPARE(session.state(), PortalSession::State::Running);
		::close(session.pipewireFd());
		session.notifyKeyboardKeysym('a', true);
		QTRY_COMPARE(failed.size(), 1);QCOMPARE(session.state(), PortalSession::State::Failed);
		auto state = []() {
			auto request = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
				QStringLiteral("/org/freedesktop/portal/desktop"), QStringLiteral("org.veyon.Test.State"), QStringLiteral("State"));
			return QDBusConnection::sessionBus().call(request).arguments();
		};
		QTRY_COMPARE(state().value(0).toStringList(), QStringList{QStringLiteral("/org/freedesktop/portal/desktop/session/test/1")});
		// This is the production retry entry point used by PipeWireVncServer.
		session.start();QTRY_COMPARE(started.size(), 2);QCOMPARE(session.state(), PortalSession::State::Running);
		::close(session.pipewireFd());
		session.notifyKeyboardKeysym('b', true);session.notifyKeyboardKeysym('b', false);
		QTRY_COMPARE(state().value(1).toStringList().size(), 2);
		QCOMPARE(state().value(1).toStringList(), (QStringList{QStringLiteral("/org/freedesktop/portal/desktop/session/test/2"), QStringLiteral("/org/freedesktop/portal/desktop/session/test/2")}));
		QCOMPARE(failed.size(), 1);
	}
};
int main(int argc, char** argv)
{
	QGuiApplication app(argc, argv);QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.PortalSessionTest"));
	if (app.arguments().contains(QStringLiteral("--fake-portal"))) {
		FakeSessionPortal portal;auto bus = QDBusConnection::sessionBus();
		if (!bus.registerService(QStringLiteral("org.freedesktop.portal.Desktop")) ||
			!bus.registerVirtualObject(QStringLiteral("/org/freedesktop/portal"), &portal, QDBusConnection::SubPath)) return 1;
		::write(STDOUT_FILENO, "ready\n", 6);return app.exec();
	}
	PortalSessionTest test;return QTest::qExec(&test, argc, argv);
}
#include "PortalSessionTest.moc"
