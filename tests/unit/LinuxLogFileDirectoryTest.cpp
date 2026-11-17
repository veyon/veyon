// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDirIterator>
#include <QTemporaryDir>
#include <QtTest>

#include "LinuxFilesystemFunctions.h"
#include "LinuxLogFileDirectory.h"

#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

// log file directories of Linux processes (WP23): /var/log/veyon for root, <base>/veyon-<login name>
// for everyone else; the system directory is passed in, so the real /var/log is never touched
class LinuxLogFileDirectoryTest : public QObject
{
	Q_OBJECT
private:
	static constexpr uid_t ForeignUserId = 65534;
	static constexpr uid_t SomeUserId = 1234;

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

	static bool makeDirectory(const QString& path, mode_t mode)
	{
		return ::mkdir(QFile::encodeName(path).constData(), mode) == 0 &&
			::chmod(QFile::encodeName(path).constData(), mode) == 0;
	}

	static bool changeOwner(const QString& path, uid_t owner)
	{
		return ::lchown(QFile::encodeName(path).constData(), owner, gid_t(-1)) == 0;
	}

	static bool isRoot()
	{
		return ::geteuid() == 0;
	}

	QString ownLoginName() const
	{
		return LinuxLogFileDirectory::sanitizedLoginName(LinuxLogFileDirectory::loginName(::geteuid()), ::geteuid());
	}

private Q_SLOTS:
	void init()
	{
		// nothing must depend on the umask
		m_previousUmask = ::umask(0);
		m_temporaryDirectory = std::make_unique<QTemporaryDir>();
		QVERIFY(m_temporaryDirectory->isValid());
	}

	void cleanup()
	{
		m_temporaryDirectory.reset();
		::umask(m_previousUmask);
	}

	void sanitizedLoginName_data()
	{
		QTest::addColumn<QString>("loginName");
		QTest::addColumn<QString>("expected");

		QTest::newRow("plain") << QStringLiteral("alice") << QStringLiteral("alice");
		QTest::newRow("domain") << QStringLiteral("alice@example.org") << QStringLiteral("alice@example.org");
		QTest::newRow("dot inside") << QStringLiteral("alice.smith") << QStringLiteral("alice.smith");
		QTest::newRow("wildcard") << QStringLiteral("*") << QStringLiteral("*");
		QTest::newRow("empty") << QString() << QStringLiteral("uid-1234");
		QTest::newRow("slash") << QStringLiteral("a/b") << QStringLiteral("uid-1234");
		QTest::newRow("leading slash") << QStringLiteral("/etc") << QStringLiteral("uid-1234");
		QTest::newRow("dot") << QStringLiteral(".") << QStringLiteral("uid-1234");
		QTest::newRow("dot dot") << QStringLiteral("..") << QStringLiteral("uid-1234");
		QTest::newRow("dot dot inside") << QStringLiteral("a..b") << QStringLiteral("uid-1234");
		QTest::newRow("newline") << QStringLiteral("a\nb") << QStringLiteral("uid-1234");
		QTest::newRow("tab") << QStringLiteral("a\tb") << QStringLiteral("uid-1234");
		QTest::newRow("nul") << QString(QStringLiteral("a") + QChar(0) + QStringLiteral("b")) << QStringLiteral("uid-1234");
		QTest::newRow("delete") << QString(QStringLiteral("a") + QChar(0x7f)) << QStringLiteral("uid-1234");
		QTest::newRow("c1 control") << QString(QStringLiteral("a") + QChar(0x9b)) << QStringLiteral("uid-1234");
		QTest::newRow("too long") << QString(129, QLatin1Char('a')) << QStringLiteral("uid-1234");
		QTest::newRow("long") << QString(128, QLatin1Char('a')) << QString(128, QLatin1Char('a'));
	}

	void sanitizedLoginName()
	{
		QFETCH(QString, loginName);
		QFETCH(QString, expected);

		QCOMPARE(LinuxLogFileDirectory::sanitizedLoginName(loginName, SomeUserId), expected);
	}

	void loginName()
	{
		QCOMPARE(LinuxLogFileDirectory::loginName(0), QStringLiteral("root"));
		// no user database entry
		QCOMPARE(LinuxLogFileDirectory::loginName(uid_t(-2)), QString());
	}

	void paths()
	{
		QCOMPARE(LinuxLogFileDirectory::systemDirectory(), QStringLiteral("/var/log/veyon"));
		QCOMPARE(LinuxLogFileDirectory::userDirectory(QStringLiteral("/tmp//logs/"), QStringLiteral("alice"), SomeUserId),
				 QStringLiteral("/tmp/logs/veyon-alice"));
		QCOMPARE(LinuxLogFileDirectory::userDirectory(QStringLiteral("/"), QStringLiteral("alice"), SomeUserId),
				 QStringLiteral("/veyon-alice"));
		QCOMPARE(LinuxLogFileDirectory::userDirectory(QStringLiteral("/tmp"), QStringLiteral("../etc"), SomeUserId),
				 QStringLiteral("/tmp/veyon-uid-1234"));

		LinuxFilesystemFunctions functions;
		QCOMPARE(functions.systemLogFileDirectory(), QStringLiteral("/var/log/veyon"));
		QCOMPARE(functions.userLogFileDirectory(QStringLiteral("/tmp"), QStringLiteral("*")),
				 QStringLiteral("/tmp/veyon-*"));
		QCOMPARE(functions.userLogFileDirectory(QStringLiteral("/tmp"), QString()),
				 QStringLiteral("/tmp/veyon-uid-%1").arg(::geteuid()));
	}

	void privilegeFollowsEffectiveUid()
	{
		QCOMPARE(LinuxFilesystemFunctions().usesSystemLogFileDirectory(), isRoot());
	}

	void prepareLogFileDirectoryOfUnprivilegedProcess()
	{
		if (isRoot())
		{
			// would use the real /var/log/veyon, covered by LoggerTest in a private mount namespace
			QSKIP("running as root");
		}

		const auto base = directory().filePath(QStringLiteral("base"));
		const auto result = LinuxFilesystemFunctions().prepareLogFileDirectory(base);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(result.path, base + QStringLiteral("/veyon-") + ownLoginName());
	}

	void createsMissingBaseAndUserDirectory()
	{
		const auto base = directory().filePath(QStringLiteral("base"));
		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());

		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(result.path, base + QStringLiteral("/veyon-alice"));
		QCOMPARE(modeOf(base), mode_t(0700));
		QCOMPARE(ownerOf(base), ::geteuid());
		QCOMPARE(modeOf(result.path), mode_t(0700));
		QCOMPARE(ownerOf(result.path), ::geteuid());

		// idempotent
		const auto secondResult = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY2(secondResult.error.isEmpty(), qPrintable(secondResult.error));
		QCOMPARE(secondResult.path, result.path);
	}

	void existingBaseDirectoryKeepsMode()
	{
		const auto base = directory().filePath(QStringLiteral("shared"));
		QVERIFY(makeDirectory(base, 01777));

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(modeOf(base), mode_t(01777));
		QCOMPARE(modeOf(result.path), mode_t(0700));
	}

	void baseDirectoryWithMissingParentRefused()
	{
		const auto base = directory().filePath(QStringLiteral("missing/base"));
		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());

		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("could not create log directory")), qPrintable(result.error));
		QVERIFY(QFileInfo::exists(base) == false);
	}

	void invalidBaseDirectoryRefused_data()
	{
		QTest::addColumn<QString>("base");
		QTest::newRow("empty") << QString();
		QTest::newRow("relative") << QStringLiteral("logs");
	}

	void invalidBaseDirectoryRefused()
	{
		QFETCH(QString, base);

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("not an absolute path")), qPrintable(result.error));

		const auto systemResult = LinuxLogFileDirectory::prepareSystemDirectory(base);
		QVERIFY(systemResult.path.isEmpty());
		QVERIFY2(systemResult.error.contains(QStringLiteral("not an absolute path")), qPrintable(systemResult.error));
	}

	void baseDirectoryIsFileRefused()
	{
		const auto base = directory().filePath(QStringLiteral("file"));
		QFile file(base);
		QVERIFY(file.open(QFile::WriteOnly));
		file.close();

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is not a directory")), qPrintable(result.error));
	}

	void unsafeBaseDirectoryRefused_data()
	{
		QTest::addColumn<int>("mode");
		QTest::addColumn<bool>("accepted");

		QTest::newRow("0700") << 0700 << true;
		QTest::newRow("0755") << 0755 << true;
		QTest::newRow("sticky world writable") << 01777 << true;
		QTest::newRow("sticky group writable") << 01770 << true;
		QTest::newRow("world writable") << 0777 << false;
		QTest::newRow("other writable") << 0757 << false;
		QTest::newRow("group writable") << 0775 << false;
	}

	void unsafeBaseDirectoryRefused()
	{
		QFETCH(int, mode);
		QFETCH(bool, accepted);

		// e.g. a custom log directory created by Veyon 4.11 with mode 0777
		const auto base = directory().filePath(QStringLiteral("base"));
		QVERIFY(makeDirectory(base, mode_t(mode)));

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QCOMPARE(result.path.isEmpty(), accepted == false);
		if (accepted == false)
		{
			QVERIFY2(result.error.contains(QStringLiteral("without the sticky bit")), qPrintable(result.error));
			QVERIFY(QFileInfo::exists(base + QStringLiteral("/veyon-alice")) == false);
		}
		// not changed
		QCOMPARE(modeOf(base), mode_t(mode));
	}

	void baseDirectoryOfOtherUserRefused()
	{
		const auto base = directory().filePath(QStringLiteral("base"));
		QVERIFY(makeDirectory(base, 01777));

		if (isRoot())
		{
			// neither root nor the user
			QVERIFY(changeOwner(base, ForeignUserId));
			const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), SomeUserId);
			QVERIFY(result.path.isEmpty());
			QVERIFY2(result.error.contains(QStringLiteral("is owned by uid %1 instead of root or %2")
											   .arg(ForeignUserId)
											   .arg(SomeUserId)),
					 qPrintable(result.error));

			// owned by root: accepted
			QVERIFY(changeOwner(base, 0));
			QVERIFY(LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), 0).error.isEmpty());
			return;
		}

		// owned by this process, requested for another user
		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid() + 1);
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is owned by uid %1 instead of root or %2")
										   .arg(::geteuid())
										   .arg(::geteuid() + 1)),
				 qPrintable(result.error));
		QVERIFY(QFileInfo::exists(base + QStringLiteral("/veyon-alice")) == false);
	}

	void existingUserDirectoryGetsPrivateMode()
	{
		const auto base = directory().path();
		const QString userDirectory = base + QStringLiteral("/veyon-alice");
		QVERIFY(makeDirectory(userDirectory, 0755));

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(modeOf(userDirectory), mode_t(0700));
	}

	void userDirectoryOfOtherUserRefused()
	{
		const auto base = directory().path();
		const QString userDirectory = base + QStringLiteral("/veyon-alice");
		QVERIFY(makeDirectory(userDirectory, 0700));

		// owned by this process but not by the requested user (not root: the base directory is refused
		// for the same reason already)
		auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid() + 1);
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is owned by uid %1 instead of").arg(::geteuid())),
				 qPrintable(result.error));

		if (isRoot() == false)
		{
			return;
		}

		// owned by somebody else (pre-created to make the user's processes log into it)
		QVERIFY(changeOwner(userDirectory, ForeignUserId));
		result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is owned by uid %1").arg(ForeignUserId)), qPrintable(result.error));
		// not touched
		QCOMPARE(modeOf(userDirectory), mode_t(0700));
		QCOMPARE(ownerOf(userDirectory), ForeignUserId);
	}

	void symlinkedUserDirectoryRefused()
	{
		const auto base = directory().path();
		const QString target = base + QStringLiteral("/target");
		QVERIFY(makeDirectory(target, 0700));
		QVERIFY(QFile::link(target, base + QStringLiteral("/veyon-alice")));

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is a symlink")), qPrintable(result.error));
	}

	void userDirectoryIsFileRefused()
	{
		const auto base = directory().path();
		QFile file(base + QStringLiteral("/veyon-alice"));
		QVERIFY(file.open(QFile::WriteOnly));
		file.close();

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is not a directory")), qPrintable(result.error));
	}

	void symlinkedBaseDirectoryRefused()
	{
		const auto target = directory().filePath(QStringLiteral("target"));
		const auto base = directory().filePath(QStringLiteral("base"));
		QVERIFY(makeDirectory(target, 0700));
		QVERIFY(QFile::link(target, base));

		const auto result = LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is a symlink")), qPrintable(result.error));
		QVERIFY(QFileInfo::exists(target + QStringLiteral("/veyon-alice")) == false);
	}

	void symlinkedParentDirectoryRefused()
	{
		const auto target = directory().filePath(QStringLiteral("target"));
		const auto link = directory().filePath(QStringLiteral("link"));
		QVERIFY(makeDirectory(target, 0700));
		QVERIFY(makeDirectory(target + QStringLiteral("/base"), 0700));
		QVERIFY(QFile::link(target, link));

		const auto result =
			LinuxLogFileDirectory::prepareUserDirectory(link + QStringLiteral("/base"), QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("one of its parent directories is a symlink")), qPrintable(result.error));

		const auto systemResult = LinuxLogFileDirectory::prepareSystemDirectory(link + QStringLiteral("/veyon"));
		QVERIFY(systemResult.path.isEmpty());
		QVERIFY(systemResult.error.isEmpty() == false);
	}

	void inaccessibleBaseDirectoryRefused()
	{
		if (isRoot())
		{
			QSKIP("root bypasses permission checks");
		}

		// not readable: the directory can't be opened
		const auto unreadableBase = directory().filePath(QStringLiteral("unreadable"));
		QVERIFY(makeDirectory(unreadableBase, 0300));
		auto result = LinuxLogFileDirectory::prepareUserDirectory(unreadableBase, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("could not open log directory")), qPrintable(result.error));

		// not writable: the per-user directory can't be created
		const auto readOnlyBase = directory().filePath(QStringLiteral("readonly"));
		QVERIFY(makeDirectory(readOnlyBase, 0500));
		result = LinuxLogFileDirectory::prepareUserDirectory(readOnlyBase, QStringLiteral("alice"), ::geteuid());
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("could not create log directory")), qPrintable(result.error));

		QVERIFY(::chmod(QFile::encodeName(unreadableBase).constData(), 0700) == 0);
		QVERIFY(::chmod(QFile::encodeName(readOnlyBase).constData(), 0700) == 0);
	}

	void createsSystemDirectory()
	{
		const auto path = directory().filePath(QStringLiteral("veyon"));
		const auto result = LinuxLogFileDirectory::prepareSystemDirectory(path);

		if (isRoot() == false)
		{
			// created, but not owned by root
			QVERIFY(result.path.isEmpty());
			QVERIFY2(result.error.contains(QStringLiteral("instead of root")), qPrintable(result.error));
			return;
		}

		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(result.path, path);
		QCOMPARE(modeOf(path), mode_t(0750));
		QCOMPARE(ownerOf(path), uid_t(0));
	}

	void existingSystemDirectoryValidated_data()
	{
		QTest::addColumn<int>("mode");
		QTest::addColumn<bool>("accepted");

		QTest::newRow("0750") << 0750 << true;
		QTest::newRow("0755") << 0755 << true;
		QTest::newRow("0700") << 0700 << true;
		QTest::newRow("group writable") << 0770 << false;
		QTest::newRow("world writable") << 0757 << false;
		QTest::newRow("0777") << 0777 << false;
		QTest::newRow("sticky world writable") << 01777 << false;
	}

	void existingSystemDirectoryValidated()
	{
		if (isRoot() == false)
		{
			QSKIP("requires root");
		}

		QFETCH(int, mode);
		QFETCH(bool, accepted);

		const auto path = directory().filePath(QStringLiteral("veyon"));
		QVERIFY(makeDirectory(path, mode_t(mode)));

		const auto result = LinuxLogFileDirectory::prepareSystemDirectory(path);
		QCOMPARE(result.path.isEmpty(), accepted == false);
		if (accepted == false)
		{
			QVERIFY2(result.error.contains(QStringLiteral("writable by group or others")), qPrintable(result.error));
		}
		// an existing directory is validated, not changed
		QCOMPARE(modeOf(path), mode_t(mode));
	}

	void systemDirectoryOfOtherUserRefused()
	{
		if (isRoot() == false)
		{
			QSKIP("requires root");
		}

		const auto path = directory().filePath(QStringLiteral("veyon"));
		QVERIFY(makeDirectory(path, 0750));
		QVERIFY(changeOwner(path, ForeignUserId));

		const auto result = LinuxLogFileDirectory::prepareSystemDirectory(path);
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is owned by uid %1 instead of root").arg(ForeignUserId)),
				 qPrintable(result.error));
	}

	void symlinkedSystemDirectoryRefused()
	{
		const auto target = directory().filePath(QStringLiteral("target"));
		const auto path = directory().filePath(QStringLiteral("veyon"));
		QVERIFY(makeDirectory(target, 0750));
		QVERIFY(QFile::link(target, path));

		const auto result = LinuxLogFileDirectory::prepareSystemDirectory(path);
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is a symlink")), qPrintable(result.error));
	}

	void nothingCreatedWorldWritable()
	{
		// umask is 0 (init()), so every mode comes from the code
		const auto base = directory().filePath(QStringLiteral("base"));
		QVERIFY(LinuxLogFileDirectory::prepareUserDirectory(base, QStringLiteral("alice"), ::geteuid()).error.isEmpty());
		(void)LinuxLogFileDirectory::prepareSystemDirectory(directory().filePath(QStringLiteral("system")));

		QDirIterator iterator(directory().path(), QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
							  QDirIterator::Subdirectories);
		int count = 0;
		while (iterator.hasNext())
		{
			const auto path = iterator.next();
			QVERIFY2((modeOf(path) & (S_IWGRP | S_IWOTH)) == 0, qPrintable(path));
			QVERIFY2((modeOf(path) & S_IRWXO) == 0, qPrintable(path));
			++count;
		}
		QCOMPARE(count, 3);
	}

	void unsafeLogFileRefused()
	{
		const auto userDirectory =
			LinuxLogFileDirectory::prepareUserDirectory(directory().path(), QStringLiteral("alice"), ::geteuid()).path;
		QVERIFY(userDirectory.isEmpty() == false);

		LinuxFilesystemFunctions functions;
		const auto openMode = QFile::WriteOnly | QFile::Append;
		const auto permissions = QFile::ReadOwner | QFile::WriteOwner;

		// own file: accepted
		const QString ownPath = userDirectory + QStringLiteral("/VeyonOwn.log");
		QFile ownFile(ownPath);
		QVERIFY(functions.openFileSafely(&ownFile, openMode, permissions));
		ownFile.close();
		QCOMPARE(modeOf(ownPath), mode_t(0600));

		// symlink to an own file: refused
		const QString linkPath = userDirectory + QStringLiteral("/VeyonLink.log");
		QVERIFY(QFile::link(ownPath, linkPath));
		QFile linkFile(linkPath);
		QVERIFY(functions.openFileSafely(&linkFile, openMode, permissions) == false);

		if (isRoot() == false)
		{
			return;
		}

		// file of another user: refused, not changed
		const QString foreignPath = userDirectory + QStringLiteral("/VeyonForeign.log");
		QFile foreignFile(foreignPath);
		QVERIFY(foreignFile.open(QFile::WriteOnly));
		foreignFile.close();
		QVERIFY(changeOwner(foreignPath, ForeignUserId));

		QVERIFY(functions.openFileSafely(&foreignFile, openMode, permissions) == false);
		QCOMPARE(ownerOf(foreignPath), ForeignUserId);
		QCOMPARE(QFileInfo(foreignPath).size(), 0);
	}

private:
	QTemporaryDir& directory() const
	{
		return *m_temporaryDirectory;
	}

	std::unique_ptr<QTemporaryDir> m_temporaryDirectory;
	mode_t m_previousUmask = 022;
};


QTEST_GUILESS_MAIN(LinuxLogFileDirectoryTest)
#include "LinuxLogFileDirectoryTest.moc"
