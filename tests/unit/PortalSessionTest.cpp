// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

// PortalSession input forwarding against a fake XDG Desktop Portal on a private session bus
// (dbus-run-session). The fake portal runs in a child process since PortalSession waits for
// OpenPipeWireRemote() synchronously.

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QDBusVirtualObject>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include "PortalSession.h"

#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>


namespace {

const auto FakePortalService = QStringLiteral("org.freedesktop.portal.Desktop");
const auto FakePortalObjectPath = QStringLiteral("/org/freedesktop/portal/desktop");
const auto SessionPathPrefix = QStringLiteral("/org/freedesktop/portal/desktop/session/test/");
const auto TestInterface = QStringLiteral("io.veyon.Test.FakePortal");

// keysyms the fake portal handles specially
constexpr quint32 SlowKeySym = 0xffe1; // Shift_L: forwarded late, like by a busy portal thread
constexpr quint32 DeniedKeySym = 'x';  // error reply, e.g. no access to the keyboard
constexpr quint32 HangingKeySym = 'h'; // never replied to

constexpr int SlowKeyDelay = 100;

constexpr uint StreamNodeId = 42;

// the D-Bus types of the streams in the Start response, a(ua{sv}) with (ii) for position and size
struct IntPair
{
	int first = 0;
	int second = 0;
};

struct Stream
{
	uint nodeId = 0;
	QVariantMap properties;
};

} // namespace

Q_DECLARE_METATYPE(IntPair)
Q_DECLARE_METATYPE(Stream)

QDBusArgument& operator<<(QDBusArgument& argument, const IntPair& pair)
{
	argument.beginStructure();
	argument << pair.first << pair.second;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, IntPair& pair)
{
	argument.beginStructure();
	argument >> pair.first >> pair.second;
	argument.endStructure();
	return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const Stream& stream)
{
	argument.beginStructure();
	argument << stream.nodeId << stream.properties;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, Stream& stream)
{
	argument.beginStructure();
	argument >> stream.nodeId >> stream.properties;
	argument.endStructure();
	return argument;
}



class FakePortal : public QDBusVirtualObject
{
public:
	QString introspect(const QString& path) const override
	{
		Q_UNUSED(path)
		return {};
	}

	bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override
	{
		const auto method = message.member();

		if (message.interface() == TestInterface && method == QLatin1String("State"))
		{
			connection.send(message.createReply({m_closedSessions, m_forwardedInput, m_rejectedInput}));
			return true;
		}

		if (message.interface() == TestInterface && method == QLatin1String("SetStream"))
		{
			const auto arguments = message.arguments();
			m_streamPosition = QPoint(arguments.value(0).toInt(), arguments.value(1).toInt());
			m_streamSize = QSize(arguments.value(2).toInt(), arguments.value(3).toInt());
			connection.send(message.createReply());
			return true;
		}

		if (message.interface() == TestInterface && method == QLatin1String("FaultReleases"))
		{
			// keysym, number of releases to reply to with an error, then number not to reply to
			const auto arguments = message.arguments();
			m_releaseFaults[arguments.value(0).toUInt()] = {arguments.value(1).toInt(), arguments.value(2).toInt()};
			connection.send(message.createReply());
			return true;
		}

		if (message.interface() == TestInterface && method == QLatin1String("SetRejectAllMotions"))
		{
			m_rejectAllMotions = message.arguments().value(0).toBool();
			connection.send(message.createReply());
			return true;
		}

		if (message.interface() == TestInterface && method == QLatin1String("DenyNextStart"))
		{
			m_denyNextStart = true;
			connection.send(message.createReply());
			return true;
		}

		if (method == QLatin1String("Close"))
		{
			m_closedSessions.append(message.path());
			connection.send(message.createReply());
			return true;
		}

		if (method == QLatin1String("NotifyKeyboardKeysym"))
		{
			handleKeysym(message, connection);
			return true;
		}

		if (method == QLatin1String("NotifyPointerMotionAbsolute"))
		{
			handlePointerMotion(message, connection);
			return true;
		}

		if (method == QLatin1String("NotifyPointerButton"))
		{
			const auto arguments = message.arguments();
			m_forwardedInput.append(QStringLiteral("%1:button:%2:%3")
										.arg(sessionNumber(arguments.value(0)))
										.arg(arguments.value(2).toInt())
										.arg(arguments.value(3).toUInt()));
			connection.send(message.createReply());
			return true;
		}

		if (method == QLatin1String("OpenPipeWireRemote"))
		{
			const int fd = ::open("/dev/null", O_RDONLY);
			const QDBusUnixFileDescriptor descriptor(fd);
			::close(fd);
			connection.send(message.createReply({QVariant::fromValue(descriptor)}));
			return true;
		}

		if (method == QLatin1String("CreateSession") || method == QLatin1String("SelectDevices") ||
			method == QLatin1String("SelectSources") || method == QLatin1String("Start"))
		{
			handleRequest(message, connection);
			return true;
		}

		return false;
	}

private:
	static QString sessionNumber(const QVariant& sessionHandle)
	{
		return qvariant_cast<QDBusObjectPath>(sessionHandle).path().mid(SessionPathPrefix.size());
	}

	void handlePointerMotion(const QDBusMessage& message, const QDBusConnection& connection)
	{
		const auto arguments = message.arguments();
		const auto stream = arguments.value(2).toUInt();
		const auto x = arguments.value(3).toDouble();
		const auto y = arguments.value(4).toDouble();

		// the check of xdg-desktop-portal >= 1.19 against the stream size from the Start response
		if (stream != StreamNodeId || m_rejectAllMotions ||
			(m_streamSize.isEmpty() == false &&
			 (x < 0 || x >= m_streamSize.width() || y < 0 || y >= m_streamSize.height())))
		{
			connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("Invalid position")));
			return;
		}

		m_forwardedInput.append(QStringLiteral("%1:motion:%2,%3")
									.arg(sessionNumber(arguments.value(0)))
									.arg(x, 0, 'f', 2)
									.arg(y, 0, 'f', 2));
		connection.send(message.createReply());
	}

	void handleKeysym(const QDBusMessage& message, const QDBusConnection& connection)
	{
		const auto arguments = message.arguments();
		const auto sessionPath = qvariant_cast<QDBusObjectPath>(arguments.value(0)).path();
		const auto keySym = arguments.value(2).toUInt();
		const auto state = arguments.value(3).toUInt();
		const auto event = QStringLiteral("%1:%2:%3")
							   .arg(sessionPath.mid(SessionPathPrefix.size()))
							   .arg(keySym, 0, 16)
							   .arg(state);

		if (state == 0 && m_releaseFaults.contains(keySym))
		{
			auto& faults = m_releaseFaults[keySym];
			if (faults.first > 0)
			{
				--faults.first;
				m_rejectedInput.append(event);
				connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("release failed")));
				return;
			}
			if (faults.second > 0)
			{
				--faults.second;
				m_rejectedInput.append(event);
				return;
			}
		}

		if (keySym == DeniedKeySym)
		{
			connection.send(message.createErrorReply(QDBusError::AccessDenied, QStringLiteral("no keyboard access")));
		}
		else if (keySym == HangingKeySym)
		{
			// no reply
		}
		else if (keySym == SlowKeySym)
		{
			QTimer::singleShot(SlowKeyDelay, this, [this, message, connection, event]() {
				m_forwardedInput.append(event);
				connection.send(message.createReply());
			});
		}
		else
		{
			m_forwardedInput.append(event);
			connection.send(message.createReply());
		}
	}

	void handleRequest(const QDBusMessage& message, const QDBusConnection& connection)
	{
		const auto options = qdbus_cast<QVariantMap>(message.arguments().constLast());
		const auto sender = QString(message.service()).remove(QLatin1Char(':')).replace(QLatin1Char('.'), QLatin1Char('_'));
		const auto requestPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
									 .arg(sender, options.value(QStringLiteral("handle_token")).toString());

		// 2 = cancelled by the user
		uint response = 0;
		if (message.member() == QLatin1String("Start") && m_denyNextStart)
		{
			m_denyNextStart = false;
			response = 2;
		}

		QVariantMap results;
		if (message.member() == QLatin1String("CreateSession"))
		{
			results[QStringLiteral("session_handle")] = QString(SessionPathPrefix + QString::number(++m_sessionCount));
		}
		else if (message.member() == QLatin1String("Start"))
		{
			Stream stream{StreamNodeId, {}};
			stream.properties[QStringLiteral("position")] =
				QVariant::fromValue(IntPair{m_streamPosition.x(), m_streamPosition.y()});
			// old portals don't report the size
			if (m_streamSize.isEmpty() == false)
			{
				stream.properties[QStringLiteral("size")] =
					QVariant::fromValue(IntPair{m_streamSize.width(), m_streamSize.height()});
			}
			results[QStringLiteral("streams")] = QVariant::fromValue(QList<Stream>{stream});
		}

		connection.send(message.createReply({QVariant::fromValue(QDBusObjectPath(requestPath))}));

		QTimer::singleShot(10, this, [connection, requestPath, response, results]() {
			auto responseSignal = QDBusMessage::createSignal(requestPath, QStringLiteral("org.freedesktop.portal.Request"),
															 QStringLiteral("Response"));
			responseSignal.setArguments({response, results});
			connection.send(responseSignal);
		});
	}

	int m_sessionCount = 0;
	bool m_denyNextStart = false;
	QPoint m_streamPosition;
	QSize m_streamSize{1920, 1080};
	bool m_rejectAllMotions = false;
	QStringList m_closedSessions;
	QStringList m_forwardedInput;
	QStringList m_rejectedInput;
	QHash<quint32, QPair<int, int>> m_releaseFaults;
};



class PortalSessionTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		if (qEnvironmentVariableIsSet("VEYON_TEST_PRIVATE_SESSION_BUS") == false)
		{
			QSKIP("requires a private session bus (run through ctest with dbus-run-session)");
		}

		QVERIFY(m_home.isValid());

		// keep the restore token file and the KDE pre-authorization (flatpak) inside the fixture
		QFile flatpak(m_home.filePath(QStringLiteral("flatpak")));
		QVERIFY(flatpak.open(QFile::WriteOnly));
		flatpak.write("#!/bin/sh\nexit 0\n");
		flatpak.close();
		QVERIFY(flatpak.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
		qputenv("PATH", m_home.path().toUtf8() + ':' + qgetenv("PATH"));
		qputenv("HOME", m_home.path().toUtf8());
		qputenv("XDG_STATE_HOME", m_home.filePath(QStringLiteral("state")).toUtf8());

		m_portal.setProcessChannelMode(QProcess::ForwardedErrorChannel);
		m_portal.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--fake-portal")});
		QVERIFY(m_portal.waitForStarted());
		QVERIFY(m_portal.waitForReadyRead());
		QCOMPARE(m_portal.readAllStandardOutput().trimmed(), QByteArrayLiteral("ready"));
	}

	void cleanupTestCase()
	{
		m_portal.terminate();
		if (m_portal.waitForFinished(2000) == false)
		{
			m_portal.kill();
			m_portal.waitForFinished();
		}
	}

	void keepsInputOrder()
	{
		PortalSession session(false);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym(SlowKeySym, true);
		session.notifyKeyboardKeysym('A', true);
		session.notifyKeyboardKeysym('A', false);
		session.notifyKeyboardKeysym(SlowKeySym, false);

		const QStringList expected{
			keyEvent(sessionNumber, SlowKeySym, 1),
			keyEvent(sessionNumber, 'A', 1),
			keyEvent(sessionNumber, 'A', 0),
			keyEvent(sessionNumber, SlowKeySym, 0),
		};
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), expected.size());
		QCOMPARE(forwardedInput(sessionNumber), expected);
	}

	void errorReplyKeepsSession()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym(DeniedKeySym, true);
		session.notifyKeyboardKeysym('b', true);
		session.notifyKeyboardKeysym('b', false);

		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{keyEvent(sessionNumber, 'b', 1), keyEvent(sessionNumber, 'b', 0)}));
		QCOMPARE(failedSpy.size(), 0);
		QCOMPARE(session.state(), PortalSession::State::Running);
	}

	// P23-2: a portal that stalls briefly must not restart the session
	void singleMissingReplyKeepsSession()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym(HangingKeySym, true);
		session.notifyKeyboardKeysym('c', true);
		QTest::qWait(PortalInputDispatcher::DefaultReplyTimeout + 500);
		QCOMPARE(failedSpy.size(), 0);
		QCOMPARE(session.state(), PortalSession::State::Running);

		// the pending input has been dropped
		session.notifyKeyboardKeysym('d', true);
		session.notifyKeyboardKeysym('d', false);
		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{keyEvent(sessionNumber, 'd', 1), keyEvent(sessionNumber, 'd', 0)}));
		QCOMPARE(failedSpy.size(), 0);
		QVERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)) == false);
	}

	void consecutiveMissingRepliesCloseSessionAndRestartWorks()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto failedSessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym(HangingKeySym, true);
		session.notifyKeyboardKeysym('c', true);
		QTest::qWait(PortalInputDispatcher::DefaultReplyTimeout + 500);
		QCOMPARE(failedSpy.size(), 0);
		session.notifyKeyboardKeysym(HangingKeySym, false);

		QTRY_COMPARE_WITH_TIMEOUT(failedSpy.size(), 1, PortalInputDispatcher::DefaultReplyTimeout + 5000);
		QCOMPARE(session.state(), PortalSession::State::Failed);
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(failedSessionNumber)));

		// how PipeWireCaptureServer retries
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		QVERIFY(sessionNumber != failedSessionNumber);

		session.notifyKeyboardKeysym('d', true);
		session.notifyKeyboardKeysym('d', false);
		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{keyEvent(sessionNumber, 'd', 1), keyEvent(sessionNumber, 'd', 0)}));
		// the press of 'c' has been dropped, but whatever is held is released when closing
		QCOMPARE(forwardedInput(failedSessionNumber), QStringList{keyEvent(failedSessionNumber, 'c', 0)});
		QCOMPARE(failedSpy.size(), 1);
	}

	// the report from KDE Plasma: a client moving the pointer outside of the stream made the
	// session fail and restart
	void rejectedPositionsKeepSession()
	{
		setStream({}, {1920, 1080});

		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		// the fake portal rejects every position now, as with a stream size that doesn't match
		setRejectAllMotions(true);
		for (int i = 0; i < PortalInputDispatcher::MaximumConsecutiveErrors * 2; ++i)
		{
			session.notifyPointer(0, 100 + i, 100, {1920, 1080});
			QTest::qWait(10);
		}
		session.notifyKeyboardKeysym('m', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'm', 1)});
		setRejectAllMotions(false);

		session.notifyPointer(0, 500, 400, {1920, 1080});
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 2);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), pointerEvent(sessionNumber, 500, 400));
		QCOMPARE(failedSpy.size(), 0);
		QCOMPARE(session.state(), PortalSession::State::Running);
	}

	void restartClosesFailedSession()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);

		QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(
			FakePortalService, FakePortalObjectPath, TestInterface, QStringLiteral("DenyNextStart")));
		session.start();
		QTRY_COMPARE(failedSpy.size(), 1);
		QCOMPARE(session.state(), PortalSession::State::Failed);
		const auto failedSessionNumber = ++m_sessionNumber;
		const QString failedSessionPath = SessionPathPrefix + QString::number(failedSessionNumber);
		QVERIFY(closedSessions().contains(failedSessionPath) == false);

		QVERIFY(startSession(session));
		QVERIFY(closedSessions().contains(failedSessionPath));

		const auto sessionNumber = currentSessionNumber();
		session.notifyKeyboardKeysym('e', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'e', 1)});
	}

	void consecutiveErrorsCloseSession()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		for (int i = 0; i < PortalInputDispatcher::MaximumConsecutiveErrors; ++i)
		{
			session.notifyKeyboardKeysym(DeniedKeySym, i % 2 == 0);
		}

		QTRY_COMPARE(failedSpy.size(), 1);
		QCOMPARE(session.state(), PortalSession::State::Failed);
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
	}

	// KDE Plasma with a 4K output at 150 %: logical stream size, buffers in physical pixels
	void pointerUsesStreamCoordinates()
	{
		setStream({3840, 0}, {2560, 1440});

		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		QCOMPARE(session.pipeWireNodeId(), StreamNodeId);
		QCOMPARE(session.streamGeometry().position, QPoint(3840, 0));
		QCOMPARE(session.streamGeometry().size, QSize(2560, 1440));

		const QSize framebufferSize(3840, 2160);
		session.notifyPointer(0, 1920, 1080, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{pointerEvent(sessionNumber, 1280, 720)});

		// beyond the logical width: "Invalid position" before
		session.notifyPointer(0, 3000, 2000, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 2);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), pointerEvent(sessionNumber, 2000, 2000.0 * 1440 / 2160));

		// the last pixel and anything beyond is clamped to the stream
		session.notifyPointer(0, 3839, 2159, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 3);
		QCOMPARE(forwardedInput(sessionNumber).constLast(),
				 pointerEvent(sessionNumber, 3839.0 * 2560 / 3840, 2159.0 * 1440 / 2160));
		session.notifyPointer(1, 5000, 3000, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 4);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), buttonEvent(sessionNumber, BTN_LEFT, 1));

		// framebuffer resized at runtime
		session.notifyPointer(0, 960, 540, {1920, 1080});
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 6);
		QCOMPARE(forwardedInput(sessionNumber).mid(4),
				 (QStringList{pointerEvent(sessionNumber, 1280, 720), buttonEvent(sessionNumber, BTN_LEFT, 0)}));

		QCOMPARE(failedSpy.size(), 0);
		QCOMPARE(session.state(), PortalSession::State::Running);
	}

	void pointerWithoutStreamSize()
	{
		setStream({}, {});

		PortalSession session(false);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		QVERIFY(session.streamGeometry().size.isEmpty());

		session.notifyPointer(0, 3000, 2000, {3840, 2160});
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{pointerEvent(sessionNumber, 3000, 2000)});
		session.notifyPointer(0, 4000, 2000, {3840, 2160});
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 2);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), pointerEvent(sessionNumber, 3839, 2000));

		setStream({}, {1920, 1080});
	}

	// P23-3: a missing reply must not drop releases, else keys stay pressed in the compositor
	void timeoutKeepsAndRetriesReleases()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym('t', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 't', 1)});

		// the release of 't' is not replied to, the release of 'a' is pending meanwhile
		faultReleases('t', 0, 1);
		session.notifyKeyboardKeysym('t', false);
		session.notifyKeyboardKeysym('a', true);
		session.notifyKeyboardKeysym('a', false);
		session.notifyPointer(0, 100, 100, {1920, 1080});

		QTest::qWait(PortalInputDispatcher::DefaultReplyTimeout + 500);
		// the press of 'a' and the motion are dropped, both releases arrive, 't' is sent again
		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{keyEvent(sessionNumber, 't', 1), keyEvent(sessionNumber, 'a', 0),
								  keyEvent(sessionNumber, 't', 0)}));
		QCOMPARE(rejectedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 't', 0)});
		QCOMPARE(failedSpy.size(), 0);

		// nothing is held anymore, so closing sends no further releases
		session.close();
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
		QCOMPARE(forwardedInput(sessionNumber).size(), 3);
	}

	void failedReleaseIsRetriedOnce()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		faultReleases('r', 1, 0);
		session.notifyKeyboardKeysym('r', true);
		session.notifyKeyboardKeysym('r', false);
		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{keyEvent(sessionNumber, 'r', 1), keyEvent(sessionNumber, 'r', 0)}));
		QCOMPARE(rejectedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'r', 0)});

		// the retry fails as well: released when the session is closed
		faultReleases('s', 2, 0);
		session.notifyKeyboardKeysym('s', true);
		session.notifyKeyboardKeysym('s', false);
		QTRY_COMPARE(rejectedInput(sessionNumber).size(), 3);
		session.notifyKeyboardKeysym('u', true);
		session.notifyKeyboardKeysym('u', false);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 5);
		QCOMPARE(forwardedInput(sessionNumber).mid(2),
				 (QStringList{keyEvent(sessionNumber, 's', 1), keyEvent(sessionNumber, 'u', 1),
							  keyEvent(sessionNumber, 'u', 0)}));

		// held button
		session.notifyPointer(1, 10, 10, {1920, 1080});
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 7);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), buttonEvent(sessionNumber, BTN_LEFT, 1));
		QCOMPARE(failedSpy.size(), 0);

		session.close();
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
		const auto input = forwardedInput(sessionNumber);
		QCOMPARE(input.size(), 9);
		QVERIFY(input.mid(7).contains(keyEvent(sessionNumber, 's', 0)));
		QVERIFY(input.mid(7).contains(buttonEvent(sessionNumber, BTN_LEFT, 0)));

		// no duplicate releases
		for (const auto& event : input)
		{
			QCOMPARE(input.count(event), 1);
		}
	}

private:
	static void faultReleases(quint32 keySym, int errors, int missingReplies)
	{
		auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
													  QStringLiteral("FaultReleases"));
		request.setArguments({keySym, errors, missingReplies});
		QDBusConnection::sessionBus().call(request);
	}

	static QStringList rejectedInput(int sessionNumber)
	{
		const auto prefix = QStringLiteral("%1:").arg(sessionNumber);
		return portalState().value(2).toStringList().filter(QRegularExpression(QLatin1Char('^') + prefix));
	}

	static void setStream(QPoint position, QSize size)
	{
		auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
													  QStringLiteral("SetStream"));
		request.setArguments({position.x(), position.y(), size.width(), size.height()});
		QDBusConnection::sessionBus().call(request);
	}

	static void setRejectAllMotions(bool reject)
	{
		auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
													  QStringLiteral("SetRejectAllMotions"));
		request.setArguments({reject});
		QDBusConnection::sessionBus().call(request);
	}

	static QString pointerEvent(int sessionNumber, double x, double y)
	{
		return QStringLiteral("%1:motion:%2,%3").arg(sessionNumber).arg(x, 0, 'f', 2).arg(y, 0, 'f', 2);
	}

	static QString buttonEvent(int sessionNumber, int button, uint state)
	{
		return QStringLiteral("%1:button:%2:%3").arg(sessionNumber).arg(button).arg(state);
	}

	bool startSession(PortalSession& session)
	{
		QSignalSpy startedSpy(&session, &PortalSession::started);
		session.start();
		if (startedSpy.wait(5000) == false || session.state() != PortalSession::State::Running)
		{
			return false;
		}
		// PipeWireFramebuffer takes ownership of the descriptor
		::close(session.pipewireFd());
		++m_sessionNumber;
		return true;
	}

	int currentSessionNumber() const
	{
		return m_sessionNumber;
	}

	static QString keyEvent(int sessionNumber, quint32 keySym, uint state)
	{
		return QStringLiteral("%1:%2:%3").arg(sessionNumber).arg(keySym, 0, 16).arg(state);
	}

	static QVariantList portalState()
	{
		const auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
															QStringLiteral("State"));
		return QDBusConnection::sessionBus().call(request).arguments();
	}

	static QStringList closedSessions()
	{
		return portalState().value(0).toStringList();
	}

	static QStringList forwardedInput(int sessionNumber)
	{
		const auto prefix = QStringLiteral("%1:").arg(sessionNumber);
		return portalState().value(1).toStringList().filter(QRegularExpression(QLatin1Char('^') + prefix));
	}

	QTemporaryDir m_home;
	QProcess m_portal;
	int m_sessionNumber = 0;
};



int main(int argc, char** argv)
{
	QGuiApplication application(argc, argv);
	QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.PortalSessionTest"));

	if (application.arguments().contains(QStringLiteral("--fake-portal")))
	{
		qDBusRegisterMetaType<IntPair>();
		qDBusRegisterMetaType<Stream>();
		qDBusRegisterMetaType<QList<Stream>>();

		FakePortal portal;
		auto connection = QDBusConnection::sessionBus();
		if (connection.registerService(FakePortalService) == false ||
			connection.registerVirtualObject(QStringLiteral("/org/freedesktop/portal"), &portal,
											 QDBusConnection::SubPath) == false)
		{
			return 1;
		}
		if (::write(STDOUT_FILENO, "ready\n", 6) != 6)
		{
			return 1;
		}
		return application.exec();
	}

	PortalSessionTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "PortalSessionTest.moc"
