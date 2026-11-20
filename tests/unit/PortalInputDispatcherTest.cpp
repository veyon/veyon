// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

// PortalInputDispatcher against a D-Bus service on a private session bus (dbus-run-session)
// which replies to the first request late, as a portal handling requests concurrently may do

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QRegularExpression>
#include <QTimer>
#include <QtTest>

#include "PortalInputDispatcher.h"

#include <memory>


namespace {

const auto ServiceName = QStringLiteral("io.veyon.Test.PortalInput");
const auto ObjectPath = QStringLiteral("/input");
const auto InterfaceName = QStringLiteral("io.veyon.Test.Input");

} // namespace



class DelayedInputService : public QDBusVirtualObject
{
public:
	enum class ReplyMode {
		Reply,
		ErrorForFirst,
		// error for every request whose label does not start with "ok"
		ErrorUnlessOk,
		NoReply,
		// no reply for every request whose label starts with "hang"
		NoReplyForHang,
		// "Invalid position" for every request whose label starts with "motion", like xdg-desktop-portal
		InvalidPositionForMotion,
	};

	QStringList received;
	QStringList completed;
	int inFlight = 0;
	int maximumInFlight = 0;
	ReplyMode replyMode = ReplyMode::Reply;

	void reset()
	{
		received.clear();
		completed.clear();
		inFlight = 0;
		maximumInFlight = 0;
		replyMode = ReplyMode::Reply;
	}

	QString introspect(const QString& path) const override
	{
		Q_UNUSED(path)
		return {};
	}

	bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override
	{
		if (message.interface() != InterfaceName)
		{
			return false;
		}

		const auto label = message.arguments().value(0).toString();
		received.append(label);
		++inFlight;
		maximumInFlight = qMax(maximumInFlight, inFlight);

		// handle the first request slowly so that requests sent concurrently would overtake it
		QTimer::singleShot(received.size() == 1 ? 100 : 0, this, [this, message, connection, label]() {
			completed.append(label);
			--inFlight;
			switch (replyMode)
			{
			case ReplyMode::Reply: connection.send(message.createReply()); break;
			case ReplyMode::ErrorForFirst:
				connection.send(completed.size() == 1 ? message.createErrorReply(QDBusError::AccessDenied,
																				 QStringLiteral("no keyboard access")) :
														message.createReply());
				break;
			case ReplyMode::ErrorUnlessOk:
				connection.send(label.startsWith(QLatin1String("ok")) ?
									message.createReply() :
									message.createErrorReply(QDBusError::AccessDenied, QStringLiteral("no access")));
				break;
			case ReplyMode::NoReply: break;
			case ReplyMode::NoReplyForHang:
				if (label.startsWith(QLatin1String("hang")) == false)
				{
					connection.send(message.createReply());
				}
				break;
			case ReplyMode::InvalidPositionForMotion:
				connection.send(label.startsWith(QLatin1String("motion")) ?
									message.createErrorReply(QDBusError::Failed, QStringLiteral("Invalid position")) :
									message.createReply());
				break;
			}
		});

		return true;
	}
};



class PortalInputDispatcherTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		if (qEnvironmentVariableIsEmpty("VEYON_TEST_PRIVATE_SESSION_BUS"))
		{
			QSKIP("needs a private session bus (run through dbus-run-session)");
		}

		m_serverConnection = std::make_unique<QDBusConnection>(
			QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("input-test-server")));
		m_clientConnection = std::make_unique<QDBusConnection>(
			QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("input-test-client")));
		QVERIFY(m_serverConnection->isConnected());
		QVERIFY(m_clientConnection->isConnected());
		QVERIFY(m_serverConnection->registerService(ServiceName));
		QVERIFY(m_serverConnection->registerVirtualObject(ObjectPath, &m_service));
	}

	void init()
	{
		m_service.reset();
	}

	void sendsRequestsInOrder()
	{
		PortalInputDispatcher dispatcher;
		const QStringList events{
			QStringLiteral("button-down"),
			QStringLiteral("shift-down"),
			QStringLiteral("a-down"),
			QStringLiteral("a-up"),
			QStringLiteral("shift-up"),
			QStringLiteral("button-up"),
			QStringLiteral("scroll"),
			QStringLiteral("return-down"),
			QStringLiteral("return-up"),
		};

		// enqueue() must not block until the requests are finished
		for (const auto& event : events)
		{
			dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this, event]() { return send(event); });
		}
		QVERIFY(m_service.completed.size() < events.size());

		QTRY_COMPARE(m_service.completed.size(), events.size());
		QCOMPARE(m_service.maximumInFlight, 1);
		QCOMPARE(m_service.received, events);
		QCOMPARE(m_service.completed, events);
	}

	// what PortalSession did before: sending without waiting lets later requests overtake
	void concurrentRequestsOvertake()
	{
		const QStringList events{QStringLiteral("shift-down"), QStringLiteral("a-down"), QStringLiteral("a-up")};
		for (const auto& event : events)
		{
			send(event);
		}

		QTRY_COMPARE(m_service.completed.size(), events.size());
		QVERIFY(m_service.maximumInFlight > 1);
		QVERIFY(m_service.completed != events);
	}

	void coalescesPendingPointerMotion()
	{
		PortalInputDispatcher dispatcher;
		const auto enqueue = [&](PortalInputDispatcher::RequestType type, const QString& event) {
			dispatcher.enqueue(type, [this, event]() { return send(event); });
		};
		using Type = PortalInputDispatcher::RequestType;

		enqueue(Type::PointerMotion, QStringLiteral("motion-1")); // sent at once
		enqueue(Type::PointerMotion, QStringLiteral("motion-2"));
		enqueue(Type::PointerMotion, QStringLiteral("motion-3"));
		enqueue(Type::Default, QStringLiteral("button-down"));
		enqueue(Type::PointerMotion, QStringLiteral("motion-4"));
		enqueue(Type::PointerMotion, QStringLiteral("motion-5"));
		enqueue(Type::Default, QStringLiteral("button-up"));
		QCOMPARE(dispatcher.pendingRequestCount(), 4);

		const QStringList expected{
			QStringLiteral("motion-1"),
			QStringLiteral("motion-3"),
			QStringLiteral("button-down"),
			QStringLiteral("motion-5"),
			QStringLiteral("button-up"),
		};
		QTRY_COMPARE(m_service.completed.size(), expected.size());
		QCOMPARE(m_service.received, expected);
		QCOMPARE(m_service.completed, expected);
	}

	void errorReplyDropsOnlyThatRequest()
	{
		PortalInputDispatcher dispatcher;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });
		m_service.replyMode = DelayedInputService::ReplyMode::ErrorForFirst;

		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("denied")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("next")); });

		QTRY_COMPARE(m_service.completed, (QStringList{QStringLiteral("denied"), QStringLiteral("next")}));
		QCOMPARE(m_service.maximumInFlight, 1);
		QCOMPARE(failures, 0);
		QCOMPARE(dispatcher.hasFailed(), false);
	}

	void consecutiveErrorsFail()
	{
		PortalInputDispatcher dispatcher;
		QDBusError error;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError& failure) {
			error = failure;
			++failures;
		});
		m_service.replyMode = DelayedInputService::ReplyMode::ErrorUnlessOk;

		// a success in between restarts the count, and the error is reported only once per session
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("portal input request failed")));
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
		QTest::failOnWarning(QRegularExpression(QStringLiteral("portal input request failed")));
#endif
		QStringList events;
		for (int i = 0; i < PortalInputDispatcher::MaximumConsecutiveErrors - 1; ++i)
		{
			events.append(QStringLiteral("denied-a%1").arg(i));
		}
		events.append(QStringLiteral("ok"));
		for (int i = 0; i < PortalInputDispatcher::MaximumConsecutiveErrors - 1; ++i)
		{
			events.append(QStringLiteral("denied-b%1").arg(i));
		}
		for (const auto& event : std::as_const(events))
		{
			dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this, event]() { return send(event); });
		}
		QTRY_COMPARE(m_service.completed.size(), events.size());
		QTest::qWait(50);
		QCOMPARE(failures, 0);
		QCOMPARE(dispatcher.hasFailed(), false);

		// one more error in a row
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("portal input failed")));
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("denied-c")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("ok-discarded")); });
		QTRY_COMPARE(failures, 1);
		QCOMPARE(error.type(), QDBusError::AccessDenied);
		QVERIFY(dispatcher.hasFailed());
		QTest::qWait(50);
		QCOMPARE(m_service.received.size(), events.size() + 1);
	}

	void unavailablePortalFails()
	{
		PortalInputDispatcher dispatcher;
		QDBusError error;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError& failure) {
			error = failure;
			++failures;
		});

		// the bus itself replies to calls to a service nobody owns
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() {
			return m_clientConnection->asyncCall(QDBusMessage::createMethodCall(
				QStringLiteral("io.veyon.Test.NoSuchPortal"), ObjectPath, InterfaceName, QStringLiteral("Notify")));
		});
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("discarded")); });

		QTRY_COMPARE(failures, 1);
		QCOMPARE(error.type(), QDBusError::ServiceUnknown);
		QVERIFY(dispatcher.hasFailed());
		QTest::qWait(50);
		QVERIFY(m_service.received.isEmpty());
	}

	// P23-2: a portal that stalls briefly must not restart the session
	void singleMissingReplyDropsPendingRequests()
	{
		PortalInputDispatcher dispatcher;
		dispatcher.setReplyTimeout(300);
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });
		m_service.replyMode = DelayedInputService::ReplyMode::NoReplyForHang;

		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no reply to portal input request, dropping 3")));
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("hang")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("discarded-1")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::PointerMotion,
						   [this]() { return send(QStringLiteral("discarded-2")); });
		QCOMPARE(dispatcher.pendingRequestCount(), 2);

		QTRY_COMPARE_WITH_TIMEOUT(dispatcher.pendingRequestCount(), 0, 3000);
		QCOMPARE(failures, 0);
		QCOMPARE(dispatcher.hasFailed(), false);

		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("next")); });
		QTRY_COMPARE(m_service.completed, (QStringList{QStringLiteral("hang"), QStringLiteral("next")}));
		QCOMPARE(m_service.received, (QStringList{QStringLiteral("hang"), QStringLiteral("next")}));
		QCOMPARE(failures, 0);
	}

	void replyResetsMissingReplyCount()
	{
		PortalInputDispatcher dispatcher;
		dispatcher.setReplyTimeout(300);
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });
		m_service.replyMode = DelayedInputService::ReplyMode::NoReplyForHang;

		for (int i = 0; i < PortalInputDispatcher::MaximumConsecutiveTimeouts + 1; ++i)
		{
			QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no reply to portal input request")));
			const auto hang = QStringLiteral("hang-%1").arg(i);
			const auto ok = QStringLiteral("ok-%1").arg(i);
			dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this, hang]() { return send(hang); });
			QTest::qWait(500);
			dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this, ok]() { return send(ok); });
			QTRY_VERIFY(m_service.completed.contains(ok));
		}

		QCOMPARE(failures, 0);
		QCOMPARE(dispatcher.hasFailed(), false);
	}

	// P23-3: dropping releases would leave keys or buttons pressed in the compositor
	void missingReplyKeepsPendingReleases()
	{
		PortalInputDispatcher dispatcher;
		dispatcher.setReplyTimeout(300);
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });
		m_service.replyMode = DelayedInputService::ReplyMode::NoReplyForHang;

		QList<bool> releaseResults;
		const auto enqueue = [&](PortalInputDispatcher::RequestType type, const QString& event) {
			dispatcher.enqueue(type, [this, event]() { return send(event); },
							   [&releaseResults](bool accepted) { releaseResults.append(accepted); });
		};
		using Type = PortalInputDispatcher::RequestType;

		QTest::ignoreMessage(QtWarningMsg,
							 QRegularExpression(QStringLiteral("no reply to portal input request, dropping 4 .* keeping 2")));
		enqueue(Type::Release, QStringLiteral("hang-release"));
		enqueue(Type::Default, QStringLiteral("press"));
		enqueue(Type::Release, QStringLiteral("release-1"));
		enqueue(Type::PointerMotion, QStringLiteral("motion"));
		enqueue(Type::Default, QStringLiteral("scroll"));
		enqueue(Type::Release, QStringLiteral("release-2"));

		QTRY_COMPARE_WITH_TIMEOUT(m_service.completed.size(), 3, 3000);
		QCOMPARE(m_service.received, (QStringList{QStringLiteral("hang-release"), QStringLiteral("release-1"),
												  QStringLiteral("release-2")}));
		// the release without reply is reported as not accepted, so that its sender can retry it
		QTRY_COMPARE(releaseResults, (QList<bool>{false, true, true}));
		QCOMPARE(failures, 0);
		QCOMPARE(dispatcher.hasFailed(), false);
	}

	void completionHandlerReportsResult()
	{
		PortalInputDispatcher dispatcher;
		m_service.replyMode = DelayedInputService::ReplyMode::ErrorUnlessOk;

		QStringList results;
		for (const auto& event : {QStringLiteral("ok-1"), QStringLiteral("denied"), QStringLiteral("ok-2")})
		{
			dispatcher.enqueue(PortalInputDispatcher::RequestType::Release, [this, event]() { return send(event); },
							   [&results, event](bool accepted) {
				results.append(QStringLiteral("%1:%2").arg(event).arg(accepted));
			});
		}

		QTRY_COMPARE(results, (QStringList{QStringLiteral("ok-1:1"), QStringLiteral("denied:0"),
										   QStringLiteral("ok-2:1")}));

		// not called for requests discarded by clear()
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Release, [this]() { return send(QStringLiteral("ok-3")); },
						   [&results](bool) { results.append(QStringLiteral("cleared")); });
		dispatcher.clear();
		QTest::qWait(100);
		QCOMPARE(results.size(), 3);
	}

	void consecutiveMissingRepliesFail()
	{
		PortalInputDispatcher dispatcher;
		dispatcher.setReplyTimeout(300);
		QDBusError error;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError& failure) {
			error = failure;
			++failures;
		});
		m_service.replyMode = DelayedInputService::ReplyMode::NoReply;

		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no reply to portal input request, dropping 2")));
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("first")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("discarded")); });
		QTRY_COMPARE_WITH_TIMEOUT(dispatcher.pendingRequestCount(), 0, 3000);
		QCOMPARE(failures, 0);

		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("portal input failed")));
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("second")); });
		QTRY_COMPARE_WITH_TIMEOUT(failures, 1, 3000);
		QCOMPARE(error.type(), QDBusError::NoReply);
		QVERIFY(dispatcher.hasFailed());
		QCOMPARE(dispatcher.pendingRequestCount(), 0);

		// ignored until reset
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("ignored")); });
		QTest::qWait(50);
		QCOMPARE(m_service.received, (QStringList{QStringLiteral("first"), QStringLiteral("second")}));

		m_service.replyMode = DelayedInputService::ReplyMode::Reply;
		dispatcher.reset();
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("after-reset")); });
		QTRY_VERIFY(m_service.completed.contains(QStringLiteral("after-reset")));
		QCOMPARE(failures, 1);
	}

	// e.g. a VNC client with a stale framebuffer size
	void rejectedPositionsKeepSession()
	{
		PortalInputDispatcher dispatcher;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });
		m_service.replyMode = DelayedInputService::ReplyMode::InvalidPositionForMotion;

		// reported once, rate-limited
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("portal input request failed.*Invalid position")));
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
		QTest::failOnWarning(QRegularExpression(QStringLiteral("portal input")));
#endif
		const auto count = PortalInputDispatcher::MaximumConsecutiveErrors * 3;
		for (int i = 0; i < count; ++i)
		{
			const auto motion = QStringLiteral("motion-%1").arg(i);
			dispatcher.enqueue(PortalInputDispatcher::RequestType::PointerMotion, [this, motion]() { return send(motion); });
			QTRY_COMPARE(m_service.completed.size(), i + 1);
		}

		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("button")); });
		QTRY_COMPARE(m_service.completed.size(), count + 1);
		QTest::qWait(50);
		QCOMPARE(failures, 0);
		QCOMPARE(dispatcher.hasFailed(), false);
	}

	// a closed session rejects pointer motions as well, but as AccessDenied
	void deniedPointerMotionsFail()
	{
		PortalInputDispatcher dispatcher;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });
		m_service.replyMode = DelayedInputService::ReplyMode::ErrorUnlessOk;

		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("portal input request failed")));
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("portal input failed")));
		for (int i = 0; i < PortalInputDispatcher::MaximumConsecutiveErrors; ++i)
		{
			const auto motion = QStringLiteral("motion-%1").arg(i);
			dispatcher.enqueue(PortalInputDispatcher::RequestType::PointerMotion, [this, motion]() { return send(motion); });
			QTRY_COMPARE(m_service.completed.size(), i + 1);
		}

		QTRY_COMPARE(failures, 1);
		QVERIFY(dispatcher.hasFailed());
	}

	void clearDiscardsPendingRequests()
	{
		PortalInputDispatcher dispatcher;
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("sent")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("unsent")); });
		QTRY_COMPARE(m_service.received.size(), 1);

		dispatcher.clear();
		QCOMPARE(dispatcher.pendingRequestCount(), 0);

		// the late reply to the abandoned request must not trigger anything
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("new")); });
		QTRY_COMPARE(m_service.completed.size(), 2);
		QTest::qWait(50);
		QCOMPARE(m_service.received, (QStringList{QStringLiteral("sent"), QStringLiteral("new")}));
	}

	void finishedCallsAreHandled_data()
	{
		QTest::addColumn<bool>("errorReply");
		QTest::newRow("reply") << false;
		QTest::newRow("error") << true;
	}

	void finishedCallsAreHandled()
	{
		QFETCH(bool, errorReply);

		PortalInputDispatcher dispatcher;
		int failures = 0;
		dispatcher.setFailureHandler([&](const QDBusError&) { ++failures; });

		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [errorReply]() {
			if (errorReply)
			{
				return QDBusPendingCall::fromError(QDBusError(QDBusError::AccessDenied, QStringLiteral("denied")));
			}
			return QDBusPendingCall::fromCompletedCall(
				QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, QStringLiteral("Notify"))
					.createReply());
		});
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("next")); });

		QTRY_COMPARE(m_service.completed, QStringList{QStringLiteral("next")});
		QCOMPARE(failures, 0);
	}

	void stalledPortalCannotAccumulateInput()
	{
		PortalInputDispatcher dispatcher;
		int failures = 0;
		QDBusError error;
		dispatcher.setFailureHandler([&](const QDBusError& failure) {
			error = failure;
			++failures;
		});

		for (int i = 0; i < PortalInputDispatcher::MaximumPendingRequests + 100; ++i)
		{
			dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("input")); });
		}

		QCOMPARE(failures, 1);
		QCOMPARE(error.type(), QDBusError::LimitsExceeded);
		QCOMPARE(dispatcher.pendingRequestCount(), 0);
		QTRY_COMPARE(m_service.completed.size(), 1);
		QTest::qWait(50);
		QCOMPARE(m_service.received.size(), 1);
	}

private:
	QDBusPendingCall send(const QString& label)
	{
		auto message = QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, QStringLiteral("Notify"));
		message.setArguments({label});
		return m_clientConnection->asyncCall(message);
	}

	std::unique_ptr<QDBusConnection> m_serverConnection;
	std::unique_ptr<QDBusConnection> m_clientConnection;
	DelayedInputService m_service;
};


QTEST_GUILESS_MAIN(PortalInputDispatcherTest)
#include "PortalInputDispatcherTest.moc"
