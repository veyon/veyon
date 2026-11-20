// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

// PortalSession input forwarding against a fake XDG Desktop Portal on a private session bus
// (dbus-run-session). The fake portal runs in a child process since PortalSession waits for
// OpenPipeWireRemote() synchronously.

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QDBusVirtualObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
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
const auto RegistryInterface = QStringLiteral("org.freedesktop.host.portal.Registry");
const auto TestAppId = QStringLiteral("io.veyon.PortalSessionTest");
const auto MutterService = QStringLiteral("org.gnome.Mutter.RemoteDesktop");

// keysyms the fake portal handles specially
constexpr quint32 SlowKeySym = 0xffe1; // Shift_L: forwarded late, like by a busy portal thread
constexpr quint32 DeniedKeySym = 'x';  // error reply, e.g. no access to the keyboard
constexpr quint32 HangingKeySym = 'h'; // never replied to

constexpr int SlowKeyDelay = 100;
constexpr int TestReplyTimeout = 300;

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
	enum class Registry {
		Available,
		// xdg-desktop-portal < 1.19.4
		Missing,
		// replies like a portal that is still starting
		NoReply
	};

	explicit FakePortal(Registry registry) :
		m_registry(registry)
	{
	}

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
			connection.send(message.createReply({m_closedSessions, m_forwardedInput, m_rejectedInput,
												 m_registeredAppIds, m_registerCalls, m_firstCalls,
												 m_restoreTokens}));
			return true;
		}

		if (message.interface() == TestInterface && method == QLatin1String("Quit"))
		{
			connection.send(message.createReply());
			QTimer::singleShot(0, qApp, &QCoreApplication::quit);
			return true;
		}

		if (message.interface() != TestInterface && trackAppId(message, connection))
		{
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

		if (message.interface() == TestInterface && method == QLatin1String("HangMotions"))
		{
			m_hangMotions = message.arguments().value(0).toInt();
			connection.send(message.createReply());
			return true;
		}

		if (message.interface() == TestInterface && method == QLatin1String("SetMutter"))
		{
			// what PortalSession takes as sign of mutter
			if (message.arguments().value(0).toBool())
			{
				QDBusConnection(connection).registerService(MutterService);
			}
			else
			{
				QDBusConnection(connection).unregisterService(MutterService);
			}
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
	// like xdg-desktop-portal: the app ID of a connection is registered with its first call or
	// derived from it otherwise, and can't be registered afterwards
	bool trackAppId(const QDBusMessage& message, const QDBusConnection& connection)
	{
		const auto sender = message.service();
		const auto firstCall = m_appIds.contains(sender) == false;
		if (firstCall)
		{
			m_firstCalls.append(message.member());
		}

		if (message.interface() == RegistryInterface && message.member() == QLatin1String("Register"))
		{
			++m_registerCalls;
			if (m_registry == Registry::Missing)
			{
				// answered with UnknownMethod
				return false;
			}
			if (m_registry == Registry::NoReply)
			{
				connection.send(message.createErrorReply(QDBusError::NoReply, QStringLiteral("no reply")));
				return true;
			}
			if (firstCall == false)
			{
				connection.send(message.createErrorReply(QStringLiteral("org.freedesktop.portal.Error.Failed"),
														 QStringLiteral("Connection already associated with an application ID")));
				return true;
			}
			const auto appId = message.arguments().value(0).toString();
			m_appIds[sender] = appId;
			m_registeredAppIds.append(appId);
			connection.send(message.createReply());
			return true;
		}

		if (firstCall)
		{
			m_appIds[sender] = QString();
		}

		return false;
	}

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

		const auto event = QStringLiteral("%1:motion:%2,%3")
							   .arg(sessionNumber(arguments.value(0)))
							   .arg(x, 0, 'f', 2)
							   .arg(y, 0, 'f', 2);

		if (m_hangMotions > 0)
		{
			--m_hangMotions;
			m_rejectedInput.append(event);
			return;
		}

		// the check of xdg-desktop-portal >= 1.14 against the stream size from the Start response,
		// which rejects every position without a size
		if (stream != StreamNodeId || m_rejectAllMotions || m_streamSize.isEmpty() ||
			x < 0 || x >= m_streamSize.width() || y < 0 || y >= m_streamSize.height())
		{
			m_rejectedInput.append(event);
			connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("Invalid position")));
			return;
		}

		m_forwardedInput.append(event);
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

		if (message.member() == QLatin1String("SelectDevices"))
		{
			m_restoreTokens.append(options.value(QStringLiteral("restore_token")).toString());
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

	const Registry m_registry;
	QHash<QString, QString> m_appIds;
	QStringList m_registeredAppIds;
	int m_registerCalls = 0;
	// the method of the first call of each connection
	QStringList m_firstCalls;
	QStringList m_restoreTokens;
	int m_sessionCount = 0;
	bool m_denyNextStart = false;
	QPoint m_streamPosition;
	QSize m_streamSize{1920, 1080};
	bool m_rejectAllMotions = false;
	int m_hangMotions = 0;
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
		QVERIFY(startPortal());
	}

	void cleanupTestCase()
	{
		stopPortal();
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
		QTest::qWait(TestReplyTimeout + 300);
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
		QTest::qWait(TestReplyTimeout + 300);
		QCOMPARE(failedSpy.size(), 0);
		session.notifyKeyboardKeysym(HangingKeySym, false);

		QTRY_COMPARE_WITH_TIMEOUT(failedSpy.size(), 1, TestReplyTimeout + 3000);
		QCOMPARE(session.state(), PortalSession::State::Failed);
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(failedSessionNumber)));

		// how PipeWireVncServer retries
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

	// mutter takes positions in buffer pixels and divides by the scale itself
	void pointerOnMutterIsNotScaled()
	{
		setStream({}, {2560, 1440});
		setMutter(true);

		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		QCOMPARE(session.pointerMappingMode(), PortalPointerMappingMode::ClampOnly);

		const QSize framebufferSize(3840, 2160);
		session.notifyPointer(0, 1920, 1080, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{pointerEvent(sessionNumber, 1920, 1080)});

		// beyond the logical size: unreachable through Notify*, clamped to the stream
		session.notifyPointer(0, 3000, 2000, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 2);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), pointerEvent(sessionNumber, 2559, 1439));
		QCOMPARE(failedSpy.size(), 0);

		setMutter(false);
		session.close();

		PortalSession kdeSession(false);
		QVERIFY(startSession(kdeSession));
		QCOMPARE(kdeSession.pointerMappingMode(), PortalPointerMappingMode::Scale);

		setStream({}, {1920, 1080});
	}

	void pointerWithoutStreamSize()
	{
		setStream({}, {});

		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no stream size")));
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		QVERIFY(session.streamGeometry().size.isEmpty());

		// passed unscaled (portals < 1.14), but rejected by newer ones; the session stays usable
		session.notifyPointer(0, 3000, 2000, {3840, 2160});
		QTRY_COMPARE(rejectedInput(sessionNumber), QStringList{pointerEvent(sessionNumber, 3000, 2000)});
		session.notifyPointer(0, 4000, 2000, {3840, 2160});
		QTRY_COMPARE(rejectedInput(sessionNumber).size(), 2);
		QCOMPARE(rejectedInput(sessionNumber).constLast(), pointerEvent(sessionNumber, 3839, 2000));

		session.notifyKeyboardKeysym('w', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'w', 1)});
		QCOMPARE(failedSpy.size(), 0);

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

		// the release of 't' is not replied to, the release of 'a' and a motion are pending meanwhile
		faultReleases('t', 0, 1);
		session.notifyKeyboardKeysym('t', false);
		session.notifyKeyboardKeysym('a', true);
		session.notifyKeyboardKeysym('a', false);
		session.notifyPointer(0, 100, 100, {1920, 1080});

		QTest::qWait(TestReplyTimeout + 300);
		// the press of 'a' is dropped, the release, the motion and 't' again arrive
		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{keyEvent(sessionNumber, 't', 1), keyEvent(sessionNumber, 'a', 0),
								  pointerEvent(sessionNumber, 100, 100), keyEvent(sessionNumber, 't', 0)}));
		QCOMPARE(rejectedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 't', 0)});
		QCOMPARE(failedSpy.size(), 0);

		// nothing is held anymore, so closing sends no further releases
		session.close();
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
		QCOMPARE(forwardedInput(sessionNumber).size(), 4);
	}

	void releaseWithoutReplyIsReleasedOnClose()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym('t', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 't', 1)});

		// the release and its retry are not replied to, the release of 'a' in between is
		faultReleases('t', 0, 2);
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not release key")));
		session.notifyKeyboardKeysym('t', false);
		session.notifyKeyboardKeysym('a', true);
		session.notifyKeyboardKeysym('a', false);
		QTRY_COMPARE_WITH_TIMEOUT(rejectedInput(sessionNumber).size(), 2, 3000);
		QTest::qWait(TestReplyTimeout + 300);
		QCOMPARE(forwardedInput(sessionNumber),
				 (QStringList{keyEvent(sessionNumber, 't', 1), keyEvent(sessionNumber, 'a', 0)}));
		QCOMPARE(failedSpy.size(), 0);

		session.close();
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
		QCOMPARE(forwardedInput(sessionNumber),
				 (QStringList{keyEvent(sessionNumber, 't', 1), keyEvent(sessionNumber, 'a', 0),
							  keyEvent(sessionNumber, 't', 0)}));
	}

	// an error reply would be repeated, e.g. without access to the keyboard
	void rejectedReleaseIsNotRetried()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		faultReleases('r', 1, 0);
		session.notifyKeyboardKeysym('r', true);
		session.notifyKeyboardKeysym('r', false);
		QTRY_COMPARE(rejectedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'r', 0)});
		QTest::qWait(100);
		QCOMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'r', 1)});

		// held button
		session.notifyPointer(1, 10, 10, {1920, 1080});
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 3);
		QCOMPARE(forwardedInput(sessionNumber).constLast(), buttonEvent(sessionNumber, BTN_LEFT, 1));
		QCOMPARE(failedSpy.size(), 0);

		// the rejected release isn't sent again, the button is released
		session.close();
		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
		const auto input = forwardedInput(sessionNumber);
		QCOMPARE(input, (QStringList{keyEvent(sessionNumber, 'r', 1), pointerEvent(sessionNumber, 10, 10),
									 buttonEvent(sessionNumber, BTN_LEFT, 1), buttonEvent(sessionNumber, BTN_LEFT, 0)}));
		QCOMPARE(rejectedInput(sessionNumber).size(), 1);
	}

	// a click without moving after a missing reply must land at the client's position
	void pointerPositionSurvivesMissingReply()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		const QSize framebufferSize(1920, 1080);

		// the motion itself is not replied to
		hangMotions(1);
		session.notifyPointer(0, 500, 400, framebufferSize);
		QTRY_COMPARE(rejectedInput(sessionNumber), QStringList{pointerEvent(sessionNumber, 500, 400)});
		QTest::qWait(TestReplyTimeout + 300);
		session.notifyPointer(1, 500, 400, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber),
					 (QStringList{pointerEvent(sessionNumber, 500, 400), buttonEvent(sessionNumber, BTN_LEFT, 1)}));

		// the motion is pending while another request is not replied to
		session.notifyKeyboardKeysym(HangingKeySym, true);
		session.notifyPointer(1, 600, 300, framebufferSize);
		QTest::qWait(TestReplyTimeout + 300);
		session.notifyPointer(0, 600, 300, framebufferSize);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 4);
		QCOMPARE(forwardedInput(sessionNumber).mid(2),
				 (QStringList{pointerEvent(sessionNumber, 600, 300), buttonEvent(sessionNumber, BTN_LEFT, 0)}));
		QCOMPARE(failedSpy.size(), 0);
	}

	// close() waits for the releases, but not for long if the portal doesn't reply
	void closeWaitsBoundedForReleases()
	{
		PortalSession session(false);
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();

		session.notifyKeyboardKeysym('z', true);
		session.notifyKeyboardKeysym('y', true);
		QTRY_COMPARE(forwardedInput(sessionNumber).size(), 2);

		faultReleases('z', 0, 1);
		QElapsedTimer timer;
		timer.start();
		session.close();
		const auto elapsed = timer.elapsed();
		QVERIFY2(elapsed < PortalSession::ReleaseOnCloseTimeout + 500, qPrintable(QString::number(elapsed)));

		QTRY_VERIFY(closedSessions().contains(SessionPathPrefix + QString::number(sessionNumber)));
		QCOMPARE(rejectedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'z', 0)});
		QCOMPARE(forwardedInput(sessionNumber).constLast(), keyEvent(sessionNumber, 'y', 0));
		QCOMPARE(session.state(), PortalSession::State::Idle);
	}

	// every start registers first, and again on the same portal it is no reason to warn
	void registersAppIdOnEveryStart()
	{
		failOnAppIdWarning();

		const auto calls = registerCalls();
		PortalSession session(false);
		QVERIFY(startSession(session));
		session.close();
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		session.notifyKeyboardKeysym('g', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'g', 1)});

		QCOMPARE(registerCalls(), calls + 2);
		QCOMPARE(registeredAppIds(), QStringList{TestAppId});
		QCOMPARE(firstCalls(), QStringList{QStringLiteral("Register")});
	}

	void restoreTokenFormats()
	{
		// xdg-desktop-portal >= 1.22.1: D-Bus path element characters, 22 for 16 random bytes
		const auto pathElementToken = QStringLiteral("AbCdEfGhIjKlMnOpQr_t9z");
		const auto uuidToken = QStringLiteral("0f2d8a5e-3c1b-4b6a-9e7d-2a4c6e8f0b1d");

		const QList<QPair<QString, QString>> tokens{
			{pathElementToken, pathElementToken},
			{uuidToken, uuidToken},
			// the portal would fail SelectDevices for these
			{QLatin1Char('{') + uuidToken + QLatin1Char('}'), {}},
			{QStringLiteral("AbCdEfGhIjKl-MnOpQr+t9z"), {}},
			{QString(MaximumRestoreTokenLength + 1, QLatin1Char('a')), {}},
		};

		for (const auto& token : tokens)
		{
			writeRestoreToken(token.first);
			PortalSession session(true);
			QVERIFY(startSession(session));
			session.close();
			QCOMPARE(restoreTokens().constLast(), token.second);
		}

		QFile::remove(restoreTokenFilePath());
	}

	// systemctl --user restart xdg-desktop-portal: the stream ends and the session is closed while
	// the new portal is running already, which knows no registration of the connection
	void registersWithRestartedPortal()
	{
		failOnAppIdWarning();

		PortalSession session(false);
		QVERIFY(startSession(session));
		session.notifyKeyboardKeysym('n', true);
		QTRY_COMPARE(forwardedInput(currentSessionNumber()), QStringList{keyEvent(currentSessionNumber(), 'n', 1)});

		restartPortal();

		// the release and the closing go to the old portal, not to the new one before the registration
		session.close();
		QTest::qWait(200);
		QCOMPARE(firstCalls(), QStringList{});

		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		session.notifyKeyboardKeysym('n', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'n', 1)});

		QCOMPARE(firstCalls(), QStringList{QStringLiteral("Register")});
		QCOMPARE(registeredAppIds(), QStringList{TestAppId});
		QCOMPARE(registerCalls(), 1);
	}

	// the portal went away and the session is closed while none is running: only the registration
	// of the next start may start the portal (D-Bus activation, see CMakeLists.txt)
	void onlyRegistrationStartsPortal()
	{
		failOnAppIdWarning();

		PortalSession session(false);
		QVERIFY(startSession(session));
		session.notifyKeyboardKeysym('p', true);
		QTRY_COMPARE(forwardedInput(currentSessionNumber()), QStringList{keyEvent(currentSessionNumber(), 'p', 1)});

		stopPortal();
		session.close();
		QTest::qWait(300);
		QCOMPARE(portalRunning(), false);

		m_sessionNumber = 0;
		QVERIFY(startSession(session));
		QVERIFY(portalRunning());
		QCOMPARE(firstCalls(), QStringList{QStringLiteral("Register")});
		QCOMPARE(registeredAppIds(), QStringList{TestAppId});

		session.close();
		restartPortal();
	}

	// e.g. at login: the start fails while the portal doesn't reply, the restart succeeds
	void waitsForPortal()
	{
		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);

		restartPortal({QStringLiteral("--unreachable-registry")});
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("not reachable for registering")));
		session.start();
		QCOMPARE(failedSpy.size(), 1);
		QCOMPARE(session.state(), PortalSession::State::Failed);
		QCOMPARE(firstCalls(), QStringList{QStringLiteral("Register")});
		QCOMPARE(registerCalls(), 1);

		restartPortal();
		QVERIFY(startSession(session));
		QCOMPARE(firstCalls(), QStringList{QStringLiteral("Register")});
		QCOMPARE(registeredAppIds(), QStringList{TestAppId});
		QCOMPARE(failedSpy.size(), 1);
	}

	// xdg-desktop-portal < 1.19.4
	void worksWithoutRegistry()
	{
		failOnAppIdWarning();

		PortalSession session(false);
		QSignalSpy failedSpy(&session, &PortalSession::failed);

		restartPortal({QStringLiteral("--without-registry")});
		QVERIFY(startSession(session));
		const auto sessionNumber = currentSessionNumber();
		session.notifyKeyboardKeysym('o', true);
		QTRY_COMPARE(forwardedInput(sessionNumber), QStringList{keyEvent(sessionNumber, 'o', 1)});
		session.close();
		QVERIFY(startSession(session));

		QCOMPARE(registeredAppIds(), QStringList{});
		QCOMPARE(registerCalls(), 2);
		QCOMPARE(failedSpy.size(), 0);

		// updated to a portal with registry
		restartPortal();
		session.close();
		QVERIFY(startSession(session));
		QCOMPARE(firstCalls(), QStringList{QStringLiteral("Register")});
		QCOMPARE(registeredAppIds(), QStringList{TestAppId});
	}

private:
	// see PortalSession
	static constexpr int MaximumRestoreTokenLength = 64;

	static void failOnAppIdWarning()
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
		QTest::failOnWarning(QRegularExpression(QStringLiteral("app ID")));
#endif
	}

	static bool portalRunning()
	{
		return QDBusConnection::sessionBus().interface()->isServiceRegistered(FakePortalService);
	}

	bool startPortal(const QStringList& arguments = {})
	{
		m_portal.start(QCoreApplication::applicationFilePath(), QStringList{QStringLiteral("--fake-portal")} + arguments);
		m_sessionNumber = 0;
		return m_portal.waitForStarted() && m_portal.waitForReadyRead() &&
			   m_portal.readAllStandardOutput().trimmed() == QByteArrayLiteral("ready");
	}

	void stopPortal()
	{
		if (m_portal.state() != QProcess::NotRunning)
		{
			m_portal.terminate();
			if (m_portal.waitForFinished(2000) == false)
			{
				m_portal.kill();
				m_portal.waitForFinished();
			}
		}
		else if (portalRunning())
		{
			// started by the bus
			auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
														  QStringLiteral("Quit"));
			request.setAutoStartService(false);
			QDBusConnection::sessionBus().call(request);
			QTRY_VERIFY(portalRunning() == false);
		}
	}

	void restartPortal(const QStringList& arguments = {})
	{
		stopPortal();
		QVERIFY(startPortal(arguments));
		// let the sessions see the change of the owner
		QTest::qWait(200);
	}

	static QString restoreTokenFilePath()
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
		return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
			   QStringLiteral("/veyon/portal-restore-token");
#else
		return QDir::homePath() + QStringLiteral("/.local/share/veyon/portal-restore-token");
#endif
	}

	static void writeRestoreToken(const QString& token)
	{
		const auto path = restoreTokenFilePath();
		QDir().mkpath(QFileInfo(path).path());
		QFile file(path);
		QVERIFY(file.open(QFile::WriteOnly | QFile::Truncate));
		file.write(token.toUtf8() + '\n');
	}

	static QStringList registeredAppIds()
	{
		return portalState().value(3).toStringList();
	}

	static int registerCalls()
	{
		return portalState().value(4).toInt();
	}

	static QStringList firstCalls()
	{
		return portalState().value(5).toStringList();
	}

	static QStringList restoreTokens()
	{
		return portalState().value(6).toStringList();
	}

	static void setMutter(bool mutter)
	{
		auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
													  QStringLiteral("SetMutter"));
		request.setArguments({mutter});
		QDBusConnection::sessionBus().call(request);
	}

	static void hangMotions(int count)
	{
		auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
													  QStringLiteral("HangMotions"));
		request.setArguments({count});
		QDBusConnection::sessionBus().call(request);
	}

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
		session.setInputReplyTimeout(TestReplyTimeout);
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
		auto request = QDBusMessage::createMethodCall(FakePortalService, FakePortalObjectPath, TestInterface,
													  QStringLiteral("State"));
		// the portal under test is started by PortalSession only
		request.setAutoStartService(false);
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
	QGuiApplication::setDesktopFileName(TestAppId);

	if (application.arguments().contains(QStringLiteral("--fake-portal")))
	{
		qDBusRegisterMetaType<IntPair>();
		qDBusRegisterMetaType<Stream>();
		qDBusRegisterMetaType<QList<Stream>>();

		auto registry = FakePortal::Registry::Available;
		if (application.arguments().contains(QStringLiteral("--without-registry")))
		{
			registry = FakePortal::Registry::Missing;
		}
		else if (application.arguments().contains(QStringLiteral("--unreachable-registry")))
		{
			registry = FakePortal::Registry::NoReply;
		}
		FakePortal portal(registry);
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
