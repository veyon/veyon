// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QTimer>
#include "PortalInputDispatcher.h"

// A portal may execute concurrent requests in a different order. Hold the first
// reply so a fire-and-forget caller reliably submits later events too early.
class DelayedInputPortal : public QDBusVirtualObject
{
public:
	QStringList received, completed;
	int inFlight = 0, maximumInFlight = 0;
	bool rejectFirst = false, withholdReplies = false;
	QString introspect(const QString&) const override { return {}; }
	bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override
	{
		if (message.interface() != QStringLiteral("org.veyon.Test.Input")) return false;
		const auto label = message.arguments().first().toString();
		received.append(label);++inFlight;maximumInFlight = qMax(maximumInFlight, inFlight);
		QTimer::singleShot(received.size() == 1 ? 100 : 0, this, [this, message, connection, label]() {
			completed.append(label);--inFlight;
			if (!withholdReplies) connection.send(rejectFirst && completed.size() == 1
				? message.createErrorReply(QDBusError::AccessDenied, QStringLiteral("Session revoked")) : message.createReply());
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
		QVERIFY2(m_server.isConnected(), "Run under dbus-run-session");
		QVERIFY(m_client.isConnected());
		QVERIFY(m_server.registerService(m_service));
		QVERIFY(m_server.registerVirtualObject(QStringLiteral("/input"), &m_portal));
	}
	void init() { m_portal.received.clear();m_portal.completed.clear();m_portal.inFlight = 0;m_portal.maximumInFlight = 0;m_portal.rejectFirst = false;m_portal.withholdReplies = false; }
	void preservesKeyboardAndPointerOrderWithoutBlocking()
	{
		PortalInputDispatcher dispatcher;
		const QStringList events{QStringLiteral("motion"), QStringLiteral("button-down"), QStringLiteral("shift-down"),
			QStringLiteral("A-down"), QStringLiteral("A-up"), QStringLiteral("shift-up"),
			QStringLiteral("button-up"), QStringLiteral("scroll"), QStringLiteral("return")};
		int completedAtHeartbeat = -1;QTimer::singleShot(10, &dispatcher, [&]() { completedAtHeartbeat = m_portal.completed.size(); });
		for (const auto& event : events) dispatcher.enqueue([this, event]() { return send(event); });
		QTRY_VERIFY(completedAtHeartbeat >= 0);
		QVERIFY(completedAtHeartbeat < events.size());
		QTRY_COMPARE(m_portal.completed.size(), events.size());
		QCOMPARE(m_portal.maximumInFlight, 1);
		QCOMPARE(m_portal.received, events);QCOMPARE(m_portal.completed, events);
	}
	void closeDiscardsUnsentInputAndAllowsNewSession()
	{
		PortalInputDispatcher dispatcher;
		dispatcher.enqueue([this]() { return send(QStringLiteral("old-first")); });
		dispatcher.enqueue([this]() { return send(QStringLiteral("old-unsent")); });
		QTRY_VERIFY(!m_portal.received.isEmpty());
		dispatcher.clear();
		QTRY_VERIFY(!m_portal.completed.isEmpty());
		QCOMPARE(m_portal.received, QStringList{QStringLiteral("old-first")});
		dispatcher.enqueue([this]() { return send(QStringLiteral("new-session")); });
		QTRY_COMPARE(m_portal.completed.size(), 2);
		QCOMPARE(m_portal.completed, (QStringList{QStringLiteral("old-first"), QStringLiteral("new-session")}));
	}
	void portalFailureDiscardsRemainingInput_data()
	{
		QTest::addColumn<bool>("timeout");
		QTest::newRow("revoked-session") << false;
		QTest::newRow("no-reply") << true;
	}
	void portalFailureDiscardsRemainingInput()
	{
		QFETCH(bool, timeout);
		PortalInputDispatcher dispatcher;
		m_portal.rejectFirst = !timeout;m_portal.withholdReplies = timeout;
		QDBusError error;
		dispatcher.onError = [&](const QDBusError& value) { error = value; };
		dispatcher.enqueue([this]() { return send(QStringLiteral("first")); });
		dispatcher.enqueue([this]() { return send(QStringLiteral("must-not-follow-failure")); });
		QTRY_VERIFY_WITH_TIMEOUT(error.isValid(), 6500);
		QCOMPARE(error.type(), timeout ? QDBusError::NoReply : QDBusError::AccessDenied);
		QCOMPARE(m_portal.received, QStringList{QStringLiteral("first")});
		dispatcher.enqueue([this]() { return send(QStringLiteral("also-blocked")); });
		QTest::qWait(20);QCOMPARE(m_portal.received.size(), 1);
		m_portal.rejectFirst = false;m_portal.withholdReplies = false;dispatcher.clear();
		dispatcher.enqueue([this]() { return send(QStringLiteral("new-session")); });
		QTRY_COMPARE(m_portal.completed.size(), 2);
	}
	void alreadyCompletedReplies_data()
	{
		QTest::addColumn<bool>("errorReply");
		QTest::newRow("success") << false;
		QTest::newRow("failure-and-restart") << true;
	}
	void alreadyCompletedReplies()
	{
		QFETCH(bool, errorReply);
		PortalInputDispatcher dispatcher;
		bool failed = false;
		dispatcher.onError = [&](const QDBusError&) { failed = true;dispatcher.clear(); };
		dispatcher.enqueue([errorReply]() {
			if (errorReply) return QDBusPendingCall::fromError(QDBusError(QDBusError::AccessDenied, QStringLiteral("Revoked")));
			return QDBusPendingCall::fromCompletedCall(QDBusMessage::createMethodCall(QStringLiteral("org.veyon.Test.PortalInput"), QStringLiteral("/input"),
				QStringLiteral("org.veyon.Test.Input"), QStringLiteral("Completed")).createReply());
		});
		if (errorReply) QTRY_VERIFY(failed);
		dispatcher.enqueue([this]() { return send(QStringLiteral("after-completed-reply")); });
		QTRY_COMPARE(m_portal.completed, QStringList{QStringLiteral("after-completed-reply")});
		QCOMPARE(failed, errorReply);
	}
	void stalledPortalCannotAccumulateUnboundedInput()
	{
		PortalInputDispatcher dispatcher;
		int errors = 0;QDBusError error;
		dispatcher.onError = [&](const QDBusError& value) { ++errors;error = value; };
		for (int i = 0; i < 5000; ++i)
			dispatcher.enqueue([this]() { return send(QStringLiteral("input")); });
		QCOMPARE(errors, 1);QCOMPARE(error.type(), QDBusError::LimitsExceeded);
		QTRY_COMPARE(m_portal.completed.size(), 1);
		QCOMPARE(m_portal.received.size(), 1);
	}
private:
	QDBusPendingCall send(const QString& label)
	{
		auto message = QDBusMessage::createMethodCall(m_service, QStringLiteral("/input"),
			QStringLiteral("org.veyon.Test.Input"), QStringLiteral("Notify"));
		message.setArguments({label});return m_client.asyncCall(message);
	}
	const QString m_service = QStringLiteral("org.veyon.Test.PortalInput");
	QDBusConnection m_server = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("input-test-server"));
	QDBusConnection m_client = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("input-test-client"));
	DelayedInputPortal m_portal;
};
QTEST_GUILESS_MAIN(PortalInputDispatcherTest)
#include "PortalInputDispatcherTest.moc"
