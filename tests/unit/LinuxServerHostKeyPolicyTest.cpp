// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "LinuxServerHostKeyPolicy.h"

// Whether a session server may get the TLS host key (threat model T-PROC-02, WP6 review M2/R1)
class LinuxServerHostKeyPolicyTest : public QObject
{
	Q_OBJECT
private:
	static constexpr auto ServerFilePath = "/usr/bin/veyon-server";

	static QString failure(bool statSucceeded, uid_t owner, mode_t mode, const QByteArray& suidDumpable)
	{
		return LinuxServerHostKeyPolicy::nonDumpableFromExecFailure(QLatin1String(ServerFilePath), statSucceeded,
																	owner, mode, suidDumpable);
	}

private Q_SLOTS:
	void nonDumpableFromExec_data()
	{
		QTest::addColumn<bool>("statSucceeded");
		QTest::addColumn<uint>("owner");
		QTest::addColumn<uint>("mode");
		QTest::addColumn<QByteArray>("suidDumpable");
		QTest::addColumn<bool>("holds");

		QTest::newRow("0711 root, suid_dumpable 0") << true << 0u << 0100711u << QByteArray("0\n") << true;
		QTest::newRow("0711 root, suid_dumpable 2") << true << 0u << 0100711u << QByteArray("2\n") << true;
		QTest::newRow("0700 root") << true << 0u << 0100700u << QByteArray("0\n") << true;
		QTest::newRow("0711 root, suid_dumpable 1") << true << 0u << 0100711u << QByteArray("1\n") << false;
		QTest::newRow("suid_dumpable unreadable") << true << 0u << 0100711u << QByteArray() << false;
		QTest::newRow("suid_dumpable garbage") << true << 0u << 0100711u << QByteArray("x") << false;
		QTest::newRow("0755 root") << true << 0u << 0100755u << QByteArray("0\n") << false;
		QTest::newRow("0751 root (group readable)") << true << 0u << 0100751u << QByteArray("0\n") << false;
		QTest::newRow("0715 root (other readable)") << true << 0u << 0100715u << QByteArray("0\n") << false;
		QTest::newRow("0711 owned by user") << true << 1000u << 0100711u << QByteArray("0\n") << false;
		QTest::newRow("stat failed") << false << 0u << 0100711u << QByteArray("0\n") << false;
	}

	void nonDumpableFromExec()
	{
		QFETCH(bool, statSucceeded);
		QFETCH(uint, owner);
		QFETCH(uint, mode);
		QFETCH(QByteArray, suidDumpable);
		QFETCH(bool, holds);

		QCOMPARE(failure(statSucceeded, uid_t(owner), mode_t(mode), suidDumpable).isEmpty(), holds);
	}

	void refusal_data()
	{
		QTest::addColumn<bool>("runAsSessionUser");
		QTest::addColumn<bool>("ptraceRestricted");
		QTest::addColumn<bool>("systemdRun");
		QTest::addColumn<bool>("nonDumpableFromExec");
		QTest::addColumn<bool>("allowed");
		QTest::addColumn<QString>("cause");

		// root server (X11)
		QTest::newRow("root") << false << false << false << false << true << QString();
		QTest::newRow("root, systemd-run") << false << false << true << false << true << QString();
		// Yama is sufficient on every launch path
		QTest::newRow("Yama, direct") << true << true << false << false << true << QString();
		QTest::newRow("Yama, systemd-run") << true << true << true << false << true << QString();
		// the execve-level condition only for the direct launch (R1)
		QTest::newRow("0711, direct") << true << false << false << true << true << QString();
		QTest::newRow("0711, systemd-run") << true << false << true << true << false << QStringLiteral("systemd-run");
		QTest::newRow("readable, direct") << true << false << false << false << false << QStringLiteral("mode 0711");
		QTest::newRow("readable, systemd-run") << true << false << true << false << false << QStringLiteral("systemd-run");
	}

	void refusal()
	{
		QFETCH(bool, runAsSessionUser);
		QFETCH(bool, ptraceRestricted);
		QFETCH(bool, systemdRun);
		QFETCH(bool, nonDumpableFromExec);
		QFETCH(bool, allowed);
		QFETCH(QString, cause);

		LinuxServerHostKeyPolicy::Conditions conditions;
		conditions.runAsSessionUser = runAsSessionUser;
		conditions.ptraceRestricted = ptraceRestricted;
		conditions.launchedThroughSystemdRun = systemdRun;
		conditions.nonDumpableFromExecFailure = nonDumpableFromExec ? QString() : failure(true, 0, 0100755, "0");

		const auto refusal = LinuxServerHostKeyPolicy::refusal(conditions);
		QCOMPARE(refusal.isEmpty(), allowed);
		if (allowed == false)
		{
			QVERIFY(refusal.contains(cause));
			QVERIFY(refusal.contains(QLatin1String("set kernel.yama.ptrace_scope >= 1")));
		}
	}

	// defaults refuse
	void defaults()
	{
		QVERIFY(LinuxServerHostKeyPolicy::refusal({}).isEmpty() == false);
	}
};


QTEST_GUILESS_MAIN(LinuxServerHostKeyPolicyTest)
#include "LinuxServerHostKeyPolicyTest.moc"
