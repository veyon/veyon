/*
 * FilesystemSafeJoinTest.cpp - tests for safe filesystem path joining
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include "Filesystem.h"

class FilesystemSafeJoinTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void safeJoin_data();
	void safeJoin();
	void safeJoinRejectsSymlinkEscape();
	void safePathComponent_data();
	void safePathComponent();
};

void FilesystemSafeJoinTest::safeJoin_data()
{
	QTest::addColumn<QString>("relativePath");
	QTest::addColumn<bool>("accepted");
	QTest::addColumn<QString>("cleanedPath");

	QTest::newRow("file") << QStringLiteral("a.txt") << true << QStringLiteral("a.txt");
	QTest::newRow("directory") << QStringLiteral("dir/a.txt") << true << QStringLiteral("dir/a.txt");
	QTest::newRow("backslash-directory") << QStringLiteral("dir\\a.txt") << true << QStringLiteral("dir/a.txt");
	QTest::newRow("unicode") << QString::fromUtf8("ü ä.txt") << true << QString::fromUtf8("ü ä.txt");
	QTest::newRow("embedded-dots") << QStringLiteral("a..b.txt") << true << QStringLiteral("a..b.txt");
	QTest::newRow("hidden") << QStringLiteral(".hidden") << true << QStringLiteral(".hidden");
	QTest::newRow("current-directory") << QStringLiteral("a/./b") << true << QStringLiteral("a/b");
	QTest::newRow("encoded-separator") << QStringLiteral("..%2f") << true << QStringLiteral("..%2f");

	QTest::newRow("parent") << QStringLiteral("../x") << false << QString{};
	QTest::newRow("nested-parent") << QStringLiteral("a/../../x") << false << QString{};
	QTest::newRow("backslash-parent") << QStringLiteral("..\\x") << false << QString{};
	QTest::newRow("absolute") << QStringLiteral("/etc/passwd") << false << QString{};
	QTest::newRow("backslash-absolute") << QStringLiteral("\\x") << false << QString{};
	QTest::newRow("drive-absolute") << QStringLiteral("C:\\x") << false << QString{};
	QTest::newRow("drive-relative") << QStringLiteral("C:x") << false << QString{};
	QTest::newRow("unc") << QStringLiteral("\\\\server\\share\\x") << false << QString{};
	QTest::newRow("device-path") << QStringLiteral("\\\\?\\C:\\x") << false << QString{};
	QTest::newRow("alternate-data-stream") << QStringLiteral("a:b") << false << QString{};
	QTest::newRow("nul") << (QStringLiteral("a") + QChar::Null + QStringLiteral("b")) << false << QString{};
	QTest::newRow("trailing-separator") << QStringLiteral("a/") << false << QString{};
	QTest::newRow("empty") << QString{} << false << QString{};
	QTest::newRow("dot") << QStringLiteral(".") << false << QString{};
	QTest::newRow("dot-dot") << QStringLiteral("..") << false << QString{};
	QTest::newRow("reserved") << QStringLiteral("CON") << false << QString{};
	QTest::newRow("reserved-with-extension") << QStringLiteral("nul.txt") << false << QString{};
	QTest::newRow("trailing-dot") << QStringLiteral("a.") << false << QString{};
	QTest::newRow("trailing-space") << QStringLiteral("a ") << false << QString{};
	QTest::newRow("too-long") << QString(4097, QLatin1Char('a')) << false << QString{};
}

void FilesystemSafeJoinTest::safeJoin()
{
	QFETCH(QString, relativePath);
	QFETCH(bool, accepted);
	QFETCH(QString, cleanedPath);

	QTemporaryDir temporaryDir;
	QVERIFY(temporaryDir.isValid());

	const auto result = Filesystem::safeJoin(temporaryDir.path(), relativePath);
	if (accepted)
	{
		QCOMPARE(result, QDir::cleanPath(temporaryDir.path() + QLatin1Char('/') + cleanedPath));
	}
	else
	{
		QVERIFY(result.isEmpty());
	}
}

void FilesystemSafeJoinTest::safeJoinRejectsSymlinkEscape()
{
#ifdef Q_OS_WIN
	QSKIP("Creating directory symlinks requires elevated privileges on Windows");
#else
	QTemporaryDir temporaryDir;
	QVERIFY(temporaryDir.isValid());

	const auto baseDir = temporaryDir.path() + QStringLiteral("/base");
	const auto outsideDir = temporaryDir.path() + QStringLiteral("/outside");
	QVERIFY(QDir().mkpath(baseDir));
	QVERIFY(QDir().mkpath(outsideDir));
	QVERIFY(QFile::link(outsideDir, baseDir + QStringLiteral("/link")));

	QVERIFY(Filesystem::safeJoin(baseDir, QStringLiteral("link/x")).isEmpty());
#endif
}

void FilesystemSafeJoinTest::safePathComponent_data()
{
	QTest::addColumn<QString>("component");
	QTest::addColumn<bool>("accepted");

	QTest::newRow("normal") << QStringLiteral("name") << true;
	QTest::newRow("parent") << QStringLiteral("..") << false;
	QTest::newRow("current") << QStringLiteral(".") << false;
	QTest::newRow("separator") << QStringLiteral("a/b") << false;
	QTest::newRow("reserved") << QStringLiteral("con") << false;
	QTest::newRow("trailing-dot") << QStringLiteral("x.") << false;
}

void FilesystemSafeJoinTest::safePathComponent()
{
	QFETCH(QString, component);
	QFETCH(bool, accepted);

	QCOMPARE(Filesystem::isSafePathComponent(component), accepted);
}

QTEST_MAIN(FilesystemSafeJoinTest)

#include "FilesystemSafeJoinTest.moc"
