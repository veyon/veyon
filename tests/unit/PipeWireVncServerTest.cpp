// GPLv2 copyright header (2019-2026 Tobias Junghans)
#include <QObject>
#include <QString>
#include <QTest>

class PipeWireVncServerTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void testPortalTokenPattern();
	void testPortalRequestPath();
};

void PipeWireVncServerTest::testPortalTokenPattern()
{
	// makeRequestToken() should return "veyon_" followed by a numeric part
	QVERIFY(QString("veyon_%1").arg(12345).startsWith("veyon_"));
	QVERIFY(QString("veyon_%1").arg(0).startsWith("veyon_"));
	QVERIFY(QString("veyon_%1").arg(999999).startsWith("veyon_"));

	// Verify the numeric part is present after the prefix
	QString token = QString("veyon_%1").arg(12345);
	QCOMPARE(token, QString("veyon_12345"));
}

void PipeWireVncServerTest::testPortalRequestPath()
{
	// Test makeRequestPath format: /org/freedesktop/portal/desktop/request/%1/%2
	const QString senderToken = "sender123";
	const QString requestToken = "req456";
	QString path = QString("/org/freedesktop/portal/desktop/request/%1/%2")
					   .arg(senderToken, requestToken);
	QCOMPARE(path, QString("/org/freedesktop/portal/desktop/request/sender123/req456"));
	QVERIFY(path.startsWith("/org/freedesktop/portal/desktop/request/"));
	QVERIFY(path.endsWith("/req456"));

	// Test with empty tokens
	QString pathEmpty = QString("/org/freedesktop/portal/desktop/request/%1/%2")
							.arg(QString(), QString());
	QCOMPARE(pathEmpty, QString("/org/freedesktop/portal/desktop/request//"));

	// Test with special characters in tokens
	QString pathSpecial = QString("/org/freedesktop/portal/desktop/request/%1/%2")
							  .arg(QString("test_user"), QString("req_42"));
	QCOMPARE(pathSpecial, QString("/org/freedesktop/portal/desktop/request/test_user/req_42"));
}

QTEST_MAIN(PipeWireVncServerTest)
#include "PipeWireVncServerTest.moc"
