/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LightDmSession.h"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

class LightDmSessionTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void rejectsUnknownSessionWithoutChangingTheDefault()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QVERIFY(LightDmSession::isAvailable(QString(), {}, {directory.path()}));
		QVERIFY(!LightDmSession::isAvailable(QStringLiteral("nonexistent-pr1208"), {}, {directory.path()}));
		QVERIFY(!LightDmSession::isAvailable(QStringLiteral("../plasma"), {}, {directory.path()}));
	}

	void rejectsAnUnresolvableDaemon()
	{
		QVERIFY(!LightDmSession::isAvailable(QStringLiteral("plasma"), 0));
		QVERIFY(LightDmSession::isAvailable(QString(), 0));
	}

	void acceptsInstalledSessionWithACommand()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QFile session(directory.filePath(QStringLiteral("plasmax11.desktop")));
		QVERIFY(session.open(QIODevice::WriteOnly));
		session.write("[Desktop Entry]\nName=Plasma\n");
		session.close();
		QVERIFY(!LightDmSession::isAvailable(QStringLiteral("plasmax11"), {}, {directory.path()}));
		QVERIFY(session.open(QIODevice::Append));
		session.write("Exec=startplasma-x11\n");
		session.close();
		QVERIFY(LightDmSession::isAvailable(QStringLiteral("plasmax11"), {}, {directory.path()}));
	}

	void usesConfiguredDirectoriesInPrecedenceOrder()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QDir root(directory.path());
		QVERIFY(root.mkdir(QStringLiteral("sessions")));
		QFile session(directory.filePath(QStringLiteral("sessions/plasma.desktop")));
		QVERIFY(session.open(QIODevice::WriteOnly));
		session.write("[Desktop Entry]\nExec=startplasma-wayland\n");
		session.close();
		QFile config(directory.filePath(QStringLiteral("lightdm.conf")));
		QVERIFY(config.open(QIODevice::WriteOnly));
		config.write("[LightDM]\nsessions-directory=" + directory.filePath(QStringLiteral("sessions")).toUtf8() + "\n");
		config.close();
		QVERIFY(LightDmSession::isAvailable(QStringLiteral("plasma"), {config.fileName()}, {}));
		QFile overrideConfig(directory.filePath(QStringLiteral("override.conf")));
		QVERIFY(overrideConfig.open(QIODevice::WriteOnly));
		overrideConfig.write("[LightDM]\nsessions-directory=/nonexistent-veyon-session-directory\n");
		overrideConfig.close();
		QVERIFY(!LightDmSession::isAvailable(QStringLiteral("plasma"), {config.fileName(), overrideConfig.fileName()}, {directory.filePath(QStringLiteral("sessions"))}));
	}
};

QTEST_GUILESS_MAIN(LightDmSessionTest)
#include "LightDmSessionTest.moc"
