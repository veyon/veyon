/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "SessionFileAccess.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <sys/stat.h>
#include <unistd.h>

class SessionFileAccessTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void sessionOwnedAuthorityIsAccepted();
	void rootOwnedAuthorityIsAcceptedOnlyForGreeter();
	void unsafeAuthorityIsRejected();
};

void SessionFileAccessTest::sessionOwnedAuthorityIsAccepted()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("Xauthority"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.close();
	QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
	QVERIFY(SessionFileAccess::isXAuthoritySafe(path, uint(::getuid()), false));
	QVERIFY(SessionFileAccess::isXAuthoritySafe(path, uint(::getuid()), true));
	// A regular user's file is never accepted for another user just because
	// the server happens to be running in a greeter.
	if (::getuid() != 0)
	{
		QVERIFY(!SessionFileAccess::isXAuthoritySafe(path, uint(::getuid()) + 1, true));
	}
}

void SessionFileAccessTest::rootOwnedAuthorityIsAcceptedOnlyForGreeter()
{
	// This test checks metadata only; it never reads the system file's contents.
	const auto path = QStringLiteral("/etc/passwd");
	struct stat info{};
	if (::stat(QFile::encodeName(path).constData(), &info) != 0 || info.st_uid != 0 ||
		(info.st_mode & (S_IWGRP | S_IWOTH)))
	{
		QSKIP("No suitable root-owned file is available for the ownership check");
	}
	QVERIFY(!SessionFileAccess::isXAuthoritySafe(path, 12345, false));
	QVERIFY(SessionFileAccess::isXAuthoritySafe(path, 12345, true));
}

void SessionFileAccessTest::unsafeAuthorityIsRejected()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("Xauthority"));
	QVERIFY(!SessionFileAccess::isXAuthoritySafe(path, uint(::getuid()), true));
	QVERIFY(!SessionFileAccess::isXAuthoritySafe(directory.path(), uint(::getuid()), true));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.close();
	QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::WriteGroup));
	QVERIFY(!SessionFileAccess::isXAuthoritySafe(path, uint(::getuid()), true));
	QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::WriteOther));
	QVERIFY(!SessionFileAccess::isXAuthoritySafe(path, uint(::getuid()), true));
}

QTEST_GUILESS_MAIN(SessionFileAccessTest)
#include "SessionFileAccessTest.moc"
