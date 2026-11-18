// SPDX-License-Identifier: GPL-2.0-or-later
#include <QBitArray>
#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <cerrno>
#include <csignal>
#include <pwd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "LinuxCoreFunctions.h"

namespace
{
const QByteArray InputPayload("user-process stdin must survive output redirection\n");

bool writeFile(const QString& path, const QByteArray& contents)
{
	QFile file(path);
	return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

int writer(const QString& directory, int channel)
{
	QByteArray input;
	char buffer[128];
	for (;;)
	{
		const auto count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
		if (count < 0 && errno == EINTR) continue;
		if (count < 0) return 31;
		if (count == 0) break;
		input.append(buffer, count);
	}
	if (!writeFile(directory + QStringLiteral("/input"), input) ||
		!writeFile(directory + QStringLiteral("/ready"), QByteArray::number(::getpid()))) return 32;
	bool released = false;
	for (int attempt = 0; attempt < 1000; ++attempt)
	{
		if (QFileInfo::exists(directory + QStringLiteral("/release"))) { released = true; break; }
		::usleep(10000);
	}
	if (!released) return 33;

	// Model helpers such as exo-open: a diagnostic after the one-shot
	// Veyon worker has exited must not terminate the helper with SIGPIPE.
	std::signal(SIGPIPE, SIG_DFL);
	const QByteArray diagnostic("late desktop helper diagnostic\n");
	if (::write(channel, diagnostic.constData(), diagnostic.size()) != diagnostic.size()) return 34;
	return writeFile(directory + QStringLiteral("/completed"), QByteArray("yes")) ? 0 : 35;
}

int launcher(QCoreApplication& application, const QString& directory, int channel)
{
	const auto user = ::getpwuid(::getuid());
	if (!user) return 21;
	LinuxCoreFunctions functions;
	if (!functions.runProgramAsUser(QCoreApplication::applicationFilePath(),
		{QStringLiteral("--writer"), directory, QString::number(channel)},
		QString::fromLocal8Bit(user->pw_name), QString(), InputPayload)) return 22;
	QTimer ready;
	QObject::connect(&ready, &QTimer::timeout, &application, [&application, directory]() {
		if (QFileInfo::exists(directory + QStringLiteral("/ready"))) application.exit(0);
	});
	ready.start(5);
	QTimer::singleShot(5000, &application, [&application]() { application.exit(23); });
	return application.exec();
}
}

class LinuxUserProcessTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(::prctl(PR_GET_CHILD_SUBREAPER, &m_previousSubreaper) == 0);
		QVERIFY(::prctl(PR_SET_CHILD_SUBREAPER, 1) == 0);
	}
	void launchedProcessCanWriteAfterLauncherExits_data()
	{
		QTest::addColumn<int>("channel");
		QTest::newRow("stdout") << STDOUT_FILENO;
		QTest::newRow("stderr") << STDERR_FILENO;
	}
	void launchedProcessCanWriteAfterLauncherExits()
	{
		QFETCH(int, channel);
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QProcess child;
		child.start(QCoreApplication::applicationFilePath(),
			{QStringLiteral("--launcher"), directory.path(), QString::number(channel)});
		QVERIFY(child.waitForStarted());
		QVERIFY2(child.waitForFinished(8000), child.readAllStandardError().constData());
		QCOMPARE(child.exitStatus(), QProcess::NormalExit);
		QCOMPARE(child.exitCode(), 0);
		QFile ready(directory.filePath(QStringLiteral("ready")));
		QVERIFY(ready.open(QIODevice::ReadOnly));
		bool validPid = false;
		const auto pid = ready.readAll().toLongLong(&validPid);
		QVERIFY(validPid && pid > 0);
		bool reaped = false;
		const auto cleanup = qScopeGuard([pid, &reaped]() {
			if (!reaped)
			{
				::kill(pid_t(pid), SIGKILL);
				while (::waitpid(pid_t(pid), nullptr, 0) < 0 && errno == EINTR) {}
			}
		});
		QFile input(directory.filePath(QStringLiteral("input")));
		QVERIFY(input.open(QIODevice::ReadOnly));
		QCOMPARE(input.readAll(), InputPayload);
		QVERIFY(writeFile(directory.filePath(QStringLiteral("release")), QByteArray("go")));
		int status = 0;
		pid_t waited = 0;
		QTRY_VERIFY_WITH_TIMEOUT(waited == pid_t(pid) || (waited = ::waitpid(pid_t(pid), &status, WNOHANG)) != 0, 5000);
		reaped = waited == pid_t(pid);
		QVERIFY2(reaped, qPrintable(QStringLiteral("waitpid failed: %1").arg(errno)));
		QVERIFY2(WIFEXITED(status), qPrintable(QStringLiteral("User helper died from signal %1 after launcher exit").arg(WIFSIGNALED(status) ? WTERMSIG(status) : -1)));
		QCOMPARE(WEXITSTATUS(status), 0);
		QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("completed"))));
	}
	void cleanupTestCase()
	{
		QVERIFY(::prctl(PR_SET_CHILD_SUBREAPER, m_previousSubreaper) == 0);
	}
private:
	int m_previousSubreaper = 0;
};

int main(int argc, char** argv)
{
	QCoreApplication application(argc, argv);
	const auto arguments = application.arguments();
	if (arguments.size() == 4 && arguments.at(1) == QLatin1String("--writer"))
		return writer(arguments.at(2), arguments.at(3).toInt());
	if (arguments.size() == 4 && arguments.at(1) == QLatin1String("--launcher"))
		return launcher(application, arguments.at(2), arguments.at(3).toInt());
	LinuxUserProcessTest test;
	return QTest::qExec(&test, argc, argv);
}
#include "LinuxUserProcessTest.moc"
