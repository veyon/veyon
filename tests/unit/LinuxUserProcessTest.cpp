// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

// LinuxCoreFunctions::runProgramAsUser(): programs started for the user (and everything they
// start) must survive the exit of the starting process, e.g. a one-shot feature worker opening a
// website through a URL handler that writes diagnostics later on.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include "LinuxCoreFunctions.h"

#include <cerrno>
#include <csignal>
#include <pwd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

const auto InputPayload = QByteArrayLiteral("user process stdin must survive output redirection\n");

constexpr auto ReadyFileName = "ready";
constexpr auto InputFileName = "input";
constexpr auto ReleaseFileName = "release";
constexpr auto CompletedFileName = "completed";



bool writeFile(const QString& filePath, const QByteArray& contents)
{
	QFile file(filePath);
	return file.open(QFile::WriteOnly) && file.write(contents) == contents.size();
}



// started through runProgramAsUser(): reads stdin, reports its PID and writes to the given output
// channel once released, i.e. after the launcher has exited
int runWriter(const QDir& directory, int channel)
{
	QByteArray input;
	char buffer[128];
	for (;;)
	{
		const auto count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
		if (count < 0 && errno == EINTR)
		{
			continue;
		}
		if (count < 0)
		{
			return 31;
		}
		if (count == 0)
		{
			break;
		}
		input.append(buffer, count);
	}

	if (writeFile(directory.filePath(QLatin1String(InputFileName)), input) == false ||
		writeFile(directory.filePath(QLatin1String(ReadyFileName)), QByteArray::number(::getpid())) == false)
	{
		return 32;
	}

	bool released = false;
	for (int attempt = 0; attempt < 1000 && released == false; ++attempt)
	{
		released = QFileInfo::exists(directory.filePath(QLatin1String(ReleaseFileName)));
		::usleep(10000);
	}
	if (released == false)
	{
		return 33;
	}

	// like most helper programs (e.g. xdg-open, exo-open) don't ignore SIGPIPE
	std::signal(SIGPIPE, SIG_DFL);
	const auto diagnostic = QByteArrayLiteral("late diagnostic of a user program\n");
	if (::write(channel, diagnostic.constData(), size_t(diagnostic.size())) != diagnostic.size())
	{
		return 34;
	}

	return writeFile(directory.filePath(QLatin1String(CompletedFileName)), QByteArrayLiteral("yes")) ? 0 : 35;
}



// the starting process, e.g. a feature worker: starts the writer and exits once it is running
int runLauncher(QCoreApplication& application, const QDir& directory, int channel)
{
	const auto user = ::getpwuid(::getuid());
	if (user == nullptr)
	{
		return 21;
	}

	LinuxCoreFunctions coreFunctions;
	if (coreFunctions.runProgramAsUser(QCoreApplication::applicationFilePath(),
									   {QStringLiteral("--writer"), directory.path(), QString::number(channel)},
									   QString::fromLocal8Bit(user->pw_name), {}, InputPayload) == false)
	{
		return 22;
	}

	QTimer readyTimer;
	QObject::connect(&readyTimer, &QTimer::timeout, &application, [&application, directory]() {
		if (QFileInfo::exists(directory.filePath(QLatin1String(ReadyFileName))))
		{
			application.exit(0);
		}
	});
	readyTimer.start(5);
	QTimer::singleShot(5000, &application, [&application]() { application.exit(23); });

	return application.exec();
}

} // namespace



class LinuxUserProcessTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		// adopt the writer once the launcher has exited so it can be waited for
		QVERIFY(::prctl(PR_GET_CHILD_SUBREAPER, &m_previousSubreaper) == 0);
		QVERIFY(::prctl(PR_SET_CHILD_SUBREAPER, 1) == 0);
	}

	void cleanupTestCase()
	{
		QVERIFY(::prctl(PR_SET_CHILD_SUBREAPER, m_previousSubreaper) == 0);
	}

	void programCanWriteAfterLauncherExits_data()
	{
		QTest::addColumn<int>("channel");
		QTest::newRow("stdout") << STDOUT_FILENO;
		QTest::newRow("stderr") << STDERR_FILENO;
	}

	void programCanWriteAfterLauncherExits()
	{
		QFETCH(int, channel);

		QTemporaryDir temporaryDirectory;
		QVERIFY(temporaryDirectory.isValid());
		const QDir directory(temporaryDirectory.path());

		QProcess launcher;
		launcher.start(QCoreApplication::applicationFilePath(),
					   {QStringLiteral("--launcher"), directory.path(), QString::number(channel)});
		QVERIFY(launcher.waitForStarted());
		QVERIFY2(launcher.waitForFinished(8000), launcher.readAllStandardError().constData());
		QCOMPARE(launcher.exitStatus(), QProcess::NormalExit);
		QCOMPARE(launcher.exitCode(), 0);

		QFile readyFile(directory.filePath(QLatin1String(ReadyFileName)));
		QVERIFY(readyFile.open(QFile::ReadOnly));
		bool validPid = false;
		const auto pid = pid_t(readyFile.readAll().toLongLong(&validPid));
		QVERIFY(validPid && pid > 0);

		bool reaped = false;
		const auto killWriter = qScopeGuard([pid, &reaped]() {
			if (reaped == false)
			{
				::kill(pid, SIGKILL);
				while (::waitpid(pid, nullptr, 0) < 0 && errno == EINTR)
				{
				}
			}
		});

		QFile inputFile(directory.filePath(QLatin1String(InputFileName)));
		QVERIFY(inputFile.open(QFile::ReadOnly));
		QCOMPARE(inputFile.readAll(), InputPayload);

		QVERIFY(writeFile(directory.filePath(QLatin1String(ReleaseFileName)), QByteArrayLiteral("go")));

		int status = 0;
		QTRY_VERIFY_WITH_TIMEOUT(reaped || (reaped = ::waitpid(pid, &status, WNOHANG) == pid), 5000);
		QVERIFY2(WIFEXITED(status), qPrintable(QStringLiteral("user program terminated by signal %1").arg(WIFSIGNALED(status) ? WTERMSIG(status) : -1)));
		QCOMPARE(WEXITSTATUS(status), 0);
		QVERIFY(QFileInfo::exists(directory.filePath(QLatin1String(CompletedFileName))));
	}

private:
	int m_previousSubreaper = 0;
};



int main(int argc, char** argv)
{
	QCoreApplication application(argc, argv);

	const auto arguments = application.arguments();
	if (arguments.size() == 4 && arguments.at(1) == QLatin1String("--writer"))
	{
		return runWriter(QDir(arguments.at(2)), arguments.at(3).toInt());
	}
	if (arguments.size() == 4 && arguments.at(1) == QLatin1String("--launcher"))
	{
		return runLauncher(application, QDir(arguments.at(2)), arguments.at(3).toInt());
	}

	LinuxUserProcessTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "LinuxUserProcessTest.moc"
