/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KdeSessionControl.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusError>
#include <QDBusReply>
#include <QThread>
#include <QUuid>
#include <QtTest>

static QProcessEnvironment kdeEnvironment()
{
	QProcessEnvironment environment;
	environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"), QStringLiteral("KDE"));
	return environment;
}

class ModernShutdown : public QObject, protected QDBusContext
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.Shutdown")
public:
	explicit ModernShutdown(QDBusError::ErrorType error = QDBusError::NoError) : m_error(error) {}
public Q_SLOTS:
	void logout() { record(QStringLiteral("logout")); }
	void logoutAndReboot() { record(QStringLiteral("logoutAndReboot")); }
	void logoutAndShutdown() { record(QStringLiteral("logoutAndShutdown")); }
	QStringList requests() const { return m_requests; }
private:
	void record(const QString& method)
	{
		m_requests.append(method);
		if (m_error != QDBusError::NoError) { sendErrorReply(m_error, QStringLiteral("request failed in test service")); }
	}
	QDBusError::ErrorType m_error;
	QStringList m_requests;
};

class LegacyShutdown : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.KSMServerInterface")
public Q_SLOTS:
	void logout(int confirm, int type, int mode)
	{
		m_requests.append(QStringLiteral("%1,%2,%3").arg(confirm).arg(type).arg(mode));
	}
	QStringList requests() const { return m_requests; }
private:
	QStringList m_requests;
};

class UnsupportedShutdown : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.Shutdown")
public Q_SLOTS:
	QStringList requests() const { return {}; }
};

// Both well-known names are owned by this fake peer, preventing activation of
// any installed Plasma service. No test connects to the system bus.
class SessionPeer
{
public:
	explicit SessionPeer(QObject* modern) :
		connectionName(QUuid::createUuid().toString()),
		bus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, connectionName)),
		modern(modern), legacy(new LegacyShutdown)
	{
		modern->moveToThread(&thread);
		legacy->moveToThread(&thread);
		QObject::connect(&thread, &QThread::finished, modern, &QObject::deleteLater);
		QObject::connect(&thread, &QThread::finished, legacy, &QObject::deleteLater);
		thread.start();
		valid = bus.isConnected() && bus.registerService(QStringLiteral("org.kde.Shutdown")) &&
			bus.registerService(QStringLiteral("org.kde.ksmserver")) &&
			bus.registerObject(QStringLiteral("/Shutdown"), modern, QDBusConnection::ExportAllSlots) &&
			bus.registerObject(QStringLiteral("/KSMServer"), legacy, QDBusConnection::ExportAllSlots);
	}
	~SessionPeer()
	{
		bus.unregisterObject(QStringLiteral("/Shutdown"));
		bus.unregisterObject(QStringLiteral("/KSMServer"));
		bus.unregisterService(QStringLiteral("org.kde.Shutdown"));
		bus.unregisterService(QStringLiteral("org.kde.ksmserver"));
		thread.quit();
		thread.wait();
		QDBusConnection::disconnectFromBus(connectionName);
	}
	QStringList requests(bool old) const
	{
		auto call = QDBusMessage::createMethodCall(old ? QStringLiteral("org.kde.ksmserver") : QStringLiteral("org.kde.Shutdown"),
			old ? QStringLiteral("/KSMServer") : QStringLiteral("/Shutdown"),
			old ? QStringLiteral("org.kde.KSMServerInterface") : QStringLiteral("org.kde.Shutdown"), QStringLiteral("requests"));
		const QDBusReply<QStringList> reply = QDBusConnection::sessionBus().call(call);
		return reply.isValid() ? reply.value() : QStringList{QStringLiteral("invalid reply")};
	}
	QString connectionName;
	QDBusConnection bus;
	QThread thread;
	QObject* modern;
	LegacyShutdown* legacy;
	bool valid{false};
};

class KdeSessionControlTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void modernMethodsHaveNoArguments_data()
	{
		QTest::addColumn<int>("operation");
		QTest::addColumn<QString>("method");
		QTest::newRow("logout") << int(KdeSessionControl::Operation::Logout) << QStringLiteral("logout");
		QTest::newRow("reboot") << int(KdeSessionControl::Operation::Reboot) << QStringLiteral("logoutAndReboot");
		QTest::newRow("shutdown") << int(KdeSessionControl::Operation::PowerOff) << QStringLiteral("logoutAndShutdown");
	}
	void modernMethodsHaveNoArguments()
	{
		QFETCH(int, operation);
		QFETCH(QString, method);
		SessionPeer peer(new ModernShutdown);
		QVERIFY(peer.valid);
		const auto reply = KdeSessionControl::request(KdeSessionControl::Operation(operation), QDBusConnection::sessionBus(), kdeEnvironment());
		QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
		QCOMPARE(peer.requests(false), QStringList{method});
		QVERIFY(peer.requests(true).isEmpty());
	}
	void legacyFallbackUsesThreeIntegers_data()
	{
		QTest::addColumn<int>("operation");
		QTest::addColumn<QString>("arguments");
		QTest::newRow("logout") << int(KdeSessionControl::Operation::Logout) << QStringLiteral("0,3,2");
		QTest::newRow("reboot") << int(KdeSessionControl::Operation::Reboot) << QStringLiteral("0,1,2");
		QTest::newRow("shutdown") << int(KdeSessionControl::Operation::PowerOff) << QStringLiteral("0,2,2");
	}
	void legacyFallbackUsesThreeIntegers()
	{
		QFETCH(int, operation);
		QFETCH(QString, arguments);
		SessionPeer peer(new UnsupportedShutdown);
		QVERIFY(peer.valid);
		const auto reply = KdeSessionControl::request(KdeSessionControl::Operation(operation), QDBusConnection::sessionBus(), kdeEnvironment());
		QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
		QCOMPARE(peer.requests(true), QStringList{arguments});
	}
	void failedRequestDoesNotTryLegacyLogout_data()
	{
		QTest::addColumn<int>("error");
		QTest::newRow("denied") << int(QDBusError::AccessDenied);
		QTest::newRow("no-reply") << int(QDBusError::NoReply);
	}
	void failedRequestDoesNotTryLegacyLogout()
	{
		QFETCH(int, error);
		SessionPeer peer(new ModernShutdown(QDBusError::ErrorType(error)));
		QVERIFY(peer.valid);
		const auto reply = KdeSessionControl::request(KdeSessionControl::Operation::Reboot, QDBusConnection::sessionBus(), kdeEnvironment());
		QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
		QCOMPARE(QDBusError(reply).type(), QDBusError::ErrorType(error));
		QCOMPARE(peer.requests(false), QStringList{QStringLiteral("logoutAndReboot")});
		QVERIFY(peer.requests(true).isEmpty());
	}
	void ignoresKdeServicesOutsideKde_data()
	{
		QTest::addColumn<QString>("current");
		QTest::addColumn<QString>("session");
		QTest::addColumn<QString>("legacy");
		QTest::addColumn<QString>("fullSession");
		QTest::newRow("xfce-with-stale-kde-flags") << QStringLiteral("XFCE") << QStringLiteral("plasma") << QStringLiteral("plasma") << QStringLiteral("true");
		QTest::newRow("gnome") << QStringLiteral("ubuntu:GNOME") << QString() << QString() << QString();
		QTest::newRow("session-overrides-legacy") << QString() << QStringLiteral("xfce") << QStringLiteral("plasma") << QStringLiteral("true");
		QTest::newRow("unknown") << QString() << QString() << QString() << QString();
	}
	void ignoresKdeServicesOutsideKde()
	{
		QFETCH(QString, current);
		QFETCH(QString, session);
		QFETCH(QString, legacy);
		QFETCH(QString, fullSession);
		QProcessEnvironment environment;
		environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"), current);
		environment.insert(QStringLiteral("XDG_SESSION_DESKTOP"), session);
		environment.insert(QStringLiteral("DESKTOP_SESSION"), legacy);
		environment.insert(QStringLiteral("KDE_FULL_SESSION"), fullSession);
		SessionPeer peer(new ModernShutdown);
		QVERIFY(peer.valid);
		for (const auto operation : {KdeSessionControl::Operation::Logout, KdeSessionControl::Operation::Reboot, KdeSessionControl::Operation::PowerOff})
		{
			QVERIFY(KdeSessionControl::isUnavailable(KdeSessionControl::request(operation, QDBusConnection::sessionBus(), environment)));
		}
		QVERIFY(peer.requests(false).isEmpty());
		QVERIFY(peer.requests(true).isEmpty());
	}
	void acceptsPlasmaSessionIdentifiers_data()
	{
		QTest::addColumn<QString>("variable");
		QTest::addColumn<QString>("value");
		QTest::newRow("desktop-list") << QStringLiteral("XDG_CURRENT_DESKTOP") << QStringLiteral("KDE:Plasma");
		QTest::newRow("x11") << QStringLiteral("DESKTOP_SESSION") << QStringLiteral("plasmax11");
		QTest::newRow("wayland") << QStringLiteral("XDG_SESSION_DESKTOP") << QStringLiteral("plasmawayland");
		QTest::newRow("legacy") << QStringLiteral("KDE_FULL_SESSION") << QStringLiteral("true");
	}
	void acceptsPlasmaSessionIdentifiers()
	{
		QFETCH(QString, variable);
		QFETCH(QString, value);
		QProcessEnvironment environment;
		environment.insert(variable, value);
		SessionPeer peer(new ModernShutdown);
		QVERIFY(peer.valid);
		const auto reply = KdeSessionControl::request(KdeSessionControl::Operation::Logout, QDBusConnection::sessionBus(), environment);
		QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
		QCOMPARE(peer.requests(false), QStringList{QStringLiteral("logout")});
	}
};

QTEST_GUILESS_MAIN(KdeSessionControlTest)
#include "KdeSessionControlTest.moc"
