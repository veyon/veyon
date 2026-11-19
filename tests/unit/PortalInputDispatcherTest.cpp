// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

// PortalInputDispatcher against a D-Bus service on a private session bus (dbus-run-session)
// which replies to the first request late, as a portal handling requests concurrently may do

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
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
		NoReply,
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
			case ReplyMode::NoReply: break;
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

	void missingReplyFails()
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

		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("first")); });
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("discarded")); });

		QTRY_COMPARE_WITH_TIMEOUT(failures, 1, 3000);
		QCOMPARE(error.type(), QDBusError::NoReply);
		QVERIFY(dispatcher.hasFailed());
		QCOMPARE(dispatcher.pendingRequestCount(), 0);

		// ignored until reset
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("ignored")); });
		QTest::qWait(50);
		QCOMPARE(m_service.received, QStringList{QStringLiteral("first")});

		m_service.replyMode = DelayedInputService::ReplyMode::Reply;
		dispatcher.reset();
		dispatcher.enqueue(PortalInputDispatcher::RequestType::Default, [this]() { return send(QStringLiteral("after-reset")); });
		QTRY_COMPARE(m_service.completed, (QStringList{QStringLiteral("first"), QStringLiteral("after-reset")}));
		QCOMPARE(failures, 1);
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
