// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDirIterator>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "Logger.h"
#include "VeyonCore.h"

#include <memory>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

// log file locations of the Logger with the Linux platform plugin (WP23). Each case runs the Logger
// in a child process of this test: as an unprivileged user (nobody if this test runs as root) with
// its log file directory in a shared, sticky directory, or as root in a private mount namespace with
// a tmpfs on /var/log, so that the real /var/log is never touched.
class LoggerTest : public QObject
{
	Q_OBJECT
private:
	static constexpr uid_t UnprivilegedUserId = 65534;
	static constexpr int SkipExitCode = 77;
	static constexpr int ChildTimeout = 60000;

	static constexpr auto ChildEnvironmentVariable = "VEYON_LOGGER_TEST_CHILD";
	static constexpr auto BaseDirectoryVariable = "VEYON_LOGGER_TEST_BASE";
	static constexpr auto ScenarioVariable = "VEYON_LOGGER_TEST_SCENARIO";

	static constexpr auto AppName = "LoggerTest";
	static constexpr auto LogFileName = "VeyonLoggerTest.log";

	struct ChildResult
	{
		int exitCode = -1;
		QString output;
	};

	static bool isRoot()
	{
		return ::geteuid() == 0;
	}

	static mode_t modeOf(const QString& path)
	{
		struct stat status{};
		return ::lstat(QFile::encodeName(path).constData(), &status) == 0 ? (status.st_mode & 07777) : 0;
	}

	static uid_t ownerOf(const QString& path)
	{
		struct stat status{};
		return ::lstat(QFile::encodeName(path).constData(), &status) == 0 ? status.st_uid : uid_t(-1);
	}

	static QString loginName(uid_t userId)
	{
		const auto entry = ::getpwuid(userId);
		return entry ? QString::fromLocal8Bit(entry->pw_name) : QString();
	}

	// uid of the logging child process
	static uid_t childUserId()
	{
		return isRoot() ? UnprivilegedUserId : ::geteuid();
	}

	static bool makeDirectory(const QString& path, mode_t mode, uid_t owner)
	{
		const auto encodedPath = QFile::encodeName(path);
		return ::mkdir(encodedPath.constData(), mode) == 0 && ::chmod(encodedPath.constData(), mode) == 0 &&
			(owner == ::geteuid() || ::lchown(encodedPath.constData(), owner, gid_t(-1)) == 0);
	}

	static bool makeFile(const QString& path, uid_t owner)
	{
		QFile file(path);
		if (file.open(QFile::WriteOnly) == false)
		{
			return false;
		}
		file.close();
		return owner == ::geteuid() || ::lchown(QFile::encodeName(path).constData(), owner, gid_t(-1)) == 0;
	}

	static QStringList logFilesBelow(const QString& path)
	{
		QStringList logFiles;
		QDirIterator iterator(path, {QStringLiteral("Veyon*.log*")}, QDir::Files | QDir::System | QDir::Hidden,
							  QDirIterator::Subdirectories);
		while (iterator.hasNext())
		{
			logFiles.append(iterator.next());
		}
		return logFiles;
	}

	// runs the given child test function with the given scenario
	ChildResult runChild(const QString& function, const QString& scenario, bool asRootInNamespace = false) const
	{
		auto environment = QProcessEnvironment::systemEnvironment();
		environment.insert(QLatin1String(ChildEnvironmentVariable), QStringLiteral("1"));
		environment.insert(QLatin1String(BaseDirectoryVariable), m_baseDirectory);
		environment.insert(QLatin1String(ScenarioVariable), scenario);
		environment.remove(QStringLiteral("VEYON_LOG_LEVEL"));
		environment.insert(QStringLiteral("TMPDIR"), m_baseDirectory);

		QString program;
		QStringList arguments;
		const auto self = QCoreApplication::applicationFilePath();

		if (asRootInNamespace)
		{
			program = QStringLiteral("unshare");
			arguments = QStringList{QStringLiteral("--mount"),
									QStringLiteral("sh"),
									QStringLiteral("-c"),
									QStringLiteral("mount -t tmpfs tmpfs /var/log || exit %1; exec \"$0\" \"$1\"")
										.arg(SkipExitCode),
									self,
									function};
		}
		else if (isRoot())
		{
			program = QStringLiteral("setpriv");
			arguments = QStringList{QStringLiteral("--reuid=%1").arg(UnprivilegedUserId),
									QStringLiteral("--regid=%1").arg(UnprivilegedUserId),
									QStringLiteral("--clear-groups"),
									self,
									function};
			environment.insert(QStringLiteral("HOME"), m_baseDirectory);
		}
		else
		{
			program = self;
			arguments = QStringList{function};
		}

		QProcess process;
		process.setProcessEnvironment(environment);
		process.setProcessChannelMode(QProcess::MergedChannels);
		process.start(program, arguments);
		if (process.waitForFinished(ChildTimeout) == false)
		{
			process.kill();
			process.waitForFinished();
			return {-1, QStringLiteral("child timed out")};
		}

		return {process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1,
				QString::fromUtf8(process.readAll())};
	}

	static std::unique_ptr<VeyonCore> createLoggingCore(const QString& baseDirectory, const QVariantMap& extraSettings)
	{
		QTemporaryDir configurationDirectory;
		if (configurationDirectory.isValid() == false)
		{
			return {};
		}
		QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, configurationDirectory.path());
		QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, configurationDirectory.path());

		VeyonCore::setupApplicationParameters();
		{
			QSettings settings(QSettings::SystemScope, QCoreApplication::organizationName(),
							   QCoreApplication::applicationName());
			settings.setValue(QStringLiteral("Logging/LogLevel"), int(Logger::LogLevel::Debug));
			settings.setValue(QStringLiteral("Logging/LogToStdErr"), false);
			settings.setValue(QStringLiteral("Logging/LogFileDirectory"), baseDirectory);
			for (auto it = extraSettings.constBegin(), end = extraSettings.constEnd(); it != end; ++it)
			{
				settings.setValue(it.key(), it.value());
			}
			settings.sync();
		}

		// the configuration is read here, the directory isn't needed afterwards
		return std::make_unique<VeyonCore>(QCoreApplication::instance(), VeyonCore::Component::Server,
										   QLatin1String(AppName), VeyonCore::minimalInitialization());
	}

	static void logMessages(int messageCount)
	{
		const QString message(1024, QLatin1Char('x'));
		for (int i = 0; i < messageCount; ++i)
		{
			// differing messages, so that they're not collapsed
			vCritical() << "logger test message" << i << message;
		}
	}

	// child side: configuration in a temporary system scope, then a VeyonCore with the Linux platform
	// plugin and logging, which writes some messages
	static void runLoggingCore(const QString& baseDirectory, const QVariantMap& extraSettings, int messageCount)
	{
		const auto core = createLoggingCore(baseDirectory, extraSettings);
		QVERIFY(core != nullptr);
		logMessages(messageCount);
	}

	static QString scenario()
	{
		return qEnvironmentVariable(ScenarioVariable);
	}

	static bool isChild()
	{
		return qEnvironmentVariableIsSet(ChildEnvironmentVariable);
	}

	QString userDirectory() const
	{
		return m_baseDirectory + QStringLiteral("/veyon-") + loginName(childUserId());
	}

	static constexpr int RotationMessageCount = 3000;
	static constexpr int FewMessages = 10;

private Q_SLOTS:
	void initTestCase()
	{
		if (isChild())
		{
			m_baseDirectory = qEnvironmentVariable(BaseDirectoryVariable);
		}
	}

	void init()
	{
		if (isChild())
		{
			return;
		}

		// a shared, sticky directory like /tmp below a directory everybody can traverse
		m_temporaryDirectory = std::make_unique<QTemporaryDir>();
		QVERIFY(m_temporaryDirectory->isValid());
		QVERIFY(::chmod(QFile::encodeName(m_temporaryDirectory->path()).constData(), 0755) == 0);
		m_baseDirectory = m_temporaryDirectory->filePath(QStringLiteral("tmp"));
		QVERIFY(makeDirectory(m_baseDirectory, 01777, ::geteuid()));
	}

	void cleanup()
	{
		m_temporaryDirectory.reset();
	}

	// --- unprivileged processes ---

	void userProcessLogsToUserDirectory()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("default"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));

		const QString logFilePath = userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY2(QFileInfo::exists(logFilePath), qPrintable(result.output));
		QCOMPARE(modeOf(userDirectory()), mode_t(0700));
		QCOMPARE(ownerOf(userDirectory()), childUserId());
		QCOMPARE(modeOf(logFilePath), mode_t(0600));
		QCOMPARE(ownerOf(logFilePath), childUserId());

		QFile logFile(logFilePath);
		QVERIFY(logFile.open(QFile::ReadOnly));
		QVERIFY(logFile.readAll().contains("logger test message"));

		// the shared base directory keeps its mode, nothing else is written
		QCOMPARE(modeOf(m_baseDirectory), mode_t(01777));
		QCOMPARE(logFilesBelow(m_baseDirectory), QStringList{logFilePath});
		QVERIFY2(result.output.contains(QLatin1String("file logging disabled")) == false, qPrintable(result.output));
	}

	void foreignFileInSharedDirectoryIgnored()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}
		if (isRoot() == false)
		{
			QSKIP("requires root to create a file of another user");
		}

		// pre-created by another user in the formerly shared location (log suppression)
		const QString legacyLogFilePath = m_baseDirectory + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY(makeFile(legacyLogFilePath, 0));

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("default"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));
		QVERIFY2(QFileInfo(userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName)).size() > 0,
				 qPrintable(result.output));
		QCOMPARE(QFileInfo(legacyLogFilePath).size(), 0);
	}

	void rotationInUserDirectory()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("rotation"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));

		const QString logFilePath = userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY(QFileInfo::exists(logFilePath));
		QVERIFY(QFileInfo::exists(logFilePath + QStringLiteral(".0")));
		QVERIFY(QFileInfo::exists(logFilePath + QStringLiteral(".1")));
		// at most LogFileRotationCount rotated files
		QVERIFY(QFileInfo::exists(logFilePath + QStringLiteral(".2")) == false);
		QCOMPARE(ownerOf(logFilePath + QStringLiteral(".0")), childUserId());
		QVERIFY(QFileInfo(logFilePath).size() <= 1024 * 1024 + 2048);
	}

	void clearingInUserDirectory()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("clearing"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));

		const QString logFilePath = userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY(QFileInfo::exists(logFilePath));
		QVERIFY(QFileInfo(logFilePath).size() <= 1024 * 1024 + 2048);
		QCOMPARE(logFilesBelow(m_baseDirectory), QStringList{logFilePath});
	}

	void foreignUserDirectoryRefused()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}
		if (isRoot() == false)
		{
			QSKIP("requires root to create a directory of another user");
		}

		// pre-created by root, so that the user's processes can't write it
		QVERIFY(makeDirectory(userDirectory(), 0700, 0));

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("default"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));
		QVERIFY2(result.output.contains(QStringLiteral("file logging disabled: log directory %1 is owned by uid 0 instead of %2")
											.arg(userDirectory())
											.arg(UnprivilegedUserId)),
				 qPrintable(result.output));
		QCOMPARE(result.output.count(QLatin1String("file logging disabled")), 1);
		// no fallback to a shared file
		QVERIFY(logFilesBelow(m_temporaryDirectory->path()).isEmpty());
	}

	void foreignFileInUserDirectoryRefused()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}
		if (isRoot() == false)
		{
			QSKIP("requires root to create a file of another user");
		}

		QVERIFY(makeDirectory(userDirectory(), 0700, UnprivilegedUserId));
		const QString logFilePath = userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY(makeFile(logFilePath, 0));

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("default"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));
		QVERIFY2(result.output.contains(QStringLiteral("file logging disabled: %1 could not be opened safely").arg(logFilePath)),
				 qPrintable(result.output));
		QCOMPARE(QFileInfo(logFilePath).size(), 0);
		QCOMPARE(ownerOf(logFilePath), uid_t(0));
	}

	void symlinkedUserDirectoryRefused()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}

		// a directory of the user elsewhere
		const QString target = m_baseDirectory + QStringLiteral("/target");
		QVERIFY(makeDirectory(target, 0700, childUserId()));
		QVERIFY(QFile::link(target, userDirectory()));

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("default"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));
		QVERIFY2(result.output.contains(QStringLiteral("file logging disabled: log directory %1 is a symlink").arg(userDirectory())),
				 qPrintable(result.output));
		// reported once although many messages were logged
		QCOMPARE(result.output.count(QLatin1String("file logging disabled")), 1);
		QVERIFY(logFilesBelow(m_temporaryDirectory->path()).isEmpty());
	}

	void symlinkedLogFileRefused()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}

		const auto result = runChild(QStringLiteral("childLogging"), QStringLiteral("symlinkedLogFile"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));

		const QString logFilePath = userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY2(result.output.contains(QStringLiteral("file logging disabled: %1 could not be opened safely").arg(logFilePath)),
				 qPrintable(result.output));
		QCOMPARE(QFileInfo(userDirectory() + QStringLiteral("/target.log")).size(), 0);
	}

	void childLogging()
	{
		if (isChild() == false)
		{
			QSKIP("child only");
		}

		QVariantMap settings;
		if (scenario() == QLatin1String("rotation"))
		{
			settings[QStringLiteral("Logging/LogFileSizeLimitEnabled")] = true;
			settings[QStringLiteral("Logging/LogFileSizeLimit")] = 1;
			settings[QStringLiteral("Logging/LogFileRotationEnabled")] = true;
			settings[QStringLiteral("Logging/LogFileRotationCount")] = 2;
		}
		else if (scenario() == QLatin1String("clearing"))
		{
			settings[QStringLiteral("Logging/LogFileSizeLimitEnabled")] = true;
			settings[QStringLiteral("Logging/LogFileSizeLimit")] = 1;
		}
		else if (scenario() == QLatin1String("symlinkedLogFile"))
		{
			// also report the reason to the logging system of the operating system
			settings[QStringLiteral("Logging/LogToSystem")] = true;

			// the directory exists and is fine, but the log file is a symlink to a file of the user
			const QString userDirectory = m_baseDirectory + QStringLiteral("/veyon-") + loginName(::geteuid());
			QVERIFY(makeDirectory(userDirectory, 0700, ::geteuid()));
			QVERIFY(makeFile(userDirectory + QStringLiteral("/target.log"), ::geteuid()));
			QVERIFY(QFile::link(userDirectory + QStringLiteral("/target.log"),
								userDirectory + QLatin1Char('/') + QLatin1String(LogFileName)));
		}

		const auto withLimit = settings.isEmpty() == false && scenario() != QLatin1String("symlinkedLogFile");
		runLoggingCore(m_baseDirectory, settings, withLimit ? RotationMessageCount : FewMessages);
	}

	void reopenRetriedAfterFailure()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}

		const auto result = runChild(QStringLiteral("childReopenRetry"), QStringLiteral("default"));
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));

		const QString logFilePath = userDirectory() + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY2(result.output.contains(QStringLiteral("file logging interrupted, retrying with the next message: %1 "
													   "could not be opened safely")
											.arg(logFilePath)),
				 qPrintable(result.output));
		QCOMPARE(result.output.count(QLatin1String("file logging")), 1);
	}

	void childReopenRetry()
	{
		if (isChild() == false)
		{
			QSKIP("child only");
		}

		// cleared (not rotated) when it exceeds 1 MB
		const auto core = createLoggingCore(m_baseDirectory, {{QStringLiteral("Logging/LogFileSizeLimitEnabled"), true}, {QStringLiteral("Logging/LogFileSizeLimit"), 1}});
		QVERIFY(core != nullptr);
		logMessages(FewMessages);

		const QString logFilePath = m_baseDirectory + QStringLiteral("/veyon-") + loginName(::geteuid()) +
			QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY(QFileInfo(logFilePath).size() > 0);

		// make reopening after clearing fail: the file is replaced by a directory, which can't be
		// removed and opened as the log file
		QVERIFY(QFile::remove(logFilePath));
		QVERIFY(QDir().mkdir(logFilePath));
		logMessages(RotationMessageCount);
		QVERIFY(QFileInfo(logFilePath).isDir());

		// the next message reopens the log file once possible again
		QVERIFY(QDir().rmdir(logFilePath));
		vCritical() << "logger test message after recovery";

		QFile logFile(logFilePath);
		QVERIFY(logFile.open(QFile::ReadOnly));
		QVERIFY(logFile.readAll().contains("logger test message after recovery"));
	}

	// --- privileged processes ---

	void rootProcessLogsToSystemDirectory()
	{
		if (isChild())
		{
			QSKIP("parent only");
		}
		if (isRoot() == false)
		{
			QSKIP("requires root and a private mount namespace");
		}
		if (QProcess::execute(QStringLiteral("unshare"), {QStringLiteral("--mount"), QStringLiteral("true")}) != 0)
		{
			QSKIP("could not create a private mount namespace");
		}

		// pre-created by another user in the formerly shared location (log suppression)
		const QString legacyLogFilePath = m_baseDirectory + QLatin1Char('/') + QLatin1String(LogFileName);
		QVERIFY(makeFile(legacyLogFilePath, UnprivilegedUserId));

		const auto result = runChild(QStringLiteral("childRootLogging"), QStringLiteral("default"), true);
		if (result.exitCode == SkipExitCode)
		{
			QSKIP("could not create a private mount namespace");
		}
		QVERIFY2(result.exitCode == 0, qPrintable(result.output));
		QVERIFY2(result.output.contains(QStringLiteral("log directory /var/log/veyon has mode 0777 and is writable by group "
													   "or others")),
				 qPrintable(result.output));

		// the configured directory is not used by privileged processes
		QCOMPARE(logFilesBelow(m_baseDirectory), QStringList{legacyLogFilePath});
		QCOMPARE(QFileInfo(legacyLogFilePath).size(), 0);
	}

	void childRootLogging()
	{
		if (isChild() == false)
		{
			QSKIP("child only");
		}

		// never touch the real /var/log: this runs on a fresh tmpfs only
		QVERIFY(::geteuid() == 0);
		QVERIFY2(QDir(QStringLiteral("/var/log")).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden).isEmpty(),
				 "/var/log is not a fresh tmpfs");

		const auto systemDirectory = QStringLiteral("/var/log/veyon");
		const QString logFilePath = systemDirectory + QLatin1Char('/') + QLatin1String(LogFileName);

		runLoggingCore(m_baseDirectory, {}, FewMessages);
		QCOMPARE(modeOf(systemDirectory), mode_t(0750));
		QCOMPARE(ownerOf(systemDirectory), uid_t(0));
		QCOMPARE(modeOf(logFilePath), mode_t(0600));
		QVERIFY(QFileInfo(logFilePath).size() > 0);
		QVERIFY(QFileInfo::exists(m_baseDirectory + QStringLiteral("/veyon-root")) == false);

		// rotation in the system directory
		runLoggingCore(m_baseDirectory,
					   {{QStringLiteral("Logging/LogFileSizeLimitEnabled"), true},
						{QStringLiteral("Logging/LogFileSizeLimit"), 1},
						{QStringLiteral("Logging/LogFileRotationEnabled"), true},
						{QStringLiteral("Logging/LogFileRotationCount"), 1}},
					   RotationMessageCount);
		QVERIFY(QFileInfo::exists(logFilePath + QStringLiteral(".0")));
		QVERIFY(QFileInfo::exists(logFilePath + QStringLiteral(".1")) == false);
		QCOMPARE(modeOf(logFilePath + QStringLiteral(".0")), mode_t(0600));

		// an existing, unsafe directory is refused, not fixed
		QVERIFY(::chmod(QFile::encodeName(systemDirectory).constData(), 0777) == 0);
		const auto size = QFileInfo(logFilePath).size();
		runLoggingCore(m_baseDirectory, {}, FewMessages);
		QCOMPARE(QFileInfo(logFilePath).size(), size);
		QCOMPARE(modeOf(systemDirectory), mode_t(0777));
	}

private:
	std::unique_ptr<QTemporaryDir> m_temporaryDirectory;
	QString m_baseDirectory;
};


QTEST_GUILESS_MAIN(LoggerTest)
#include "LoggerTest.moc"
