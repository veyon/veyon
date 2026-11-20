// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

// FileCollect: grouping values used as file name prefix or subdirectory, handling of output files
// opened via openFileSafely(), files that could not be collected

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "FileCollectController.h"
#include "FileCollection.h"
#include "PlatformFilesystemFunctions.h"
#include "VeyonCore.h"

#include <memory>


class FileCollectTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_home.isValid());
		qputenv("HOME", m_home.path().toUtf8());
		qputenv("XDG_CONFIG_HOME", m_home.filePath(QStringLiteral(".config")).toUtf8());

		m_core = std::make_unique<VeyonCore>(QCoreApplication::instance(), VeyonCore::Component::CLI,
											 QStringLiteral("FileCollectTest"));
	}

	void cleanupTestCase()
	{
		m_core.reset();
	}

	void safeGroupingValue_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<QString>("expected");

		QTest::newRow("plain") << QStringLiteral("Jane Doe") << QStringLiteral("Jane Doe");
		QTest::newRow("backslash") << QStringLiteral("a\\b") << QStringLiteral("a_b");
		QTest::newRow("domain-backslash") << QStringLiteral("SCHOOL\\jdoe") << QStringLiteral("SCHOOL_jdoe");
		QTest::newRow("slash") << QStringLiteral("a/b") << QStringLiteral("a_b");
		QTest::newRow("reserved-characters") << QStringLiteral("a<b>c:d\"e|f?g*h") << QStringLiteral("a_b_c_d_e_f_g_h");
		QTest::newRow("control-character") << QStringLiteral("a\tb") << QStringLiteral("a_b");
		QTest::newRow("trailing-dot-space") << QStringLiteral("name. ") << QStringLiteral("name");
		QTest::newRow("parent") << QStringLiteral("..") << QStringLiteral("_");
		QTest::newRow("empty") << QString{} << QStringLiteral("_");
	}

	void safeGroupingValue()
	{
		QFETCH(QString, value);
		QFETCH(QString, expected);

		QCOMPARE(FileCollectController::safeGroupingValue(value), expected);
	}

	void failedFilesAreRecordedByIndex()
	{
		FileCollection collection;
		collection.files = {QStringLiteral("a.txt"), QStringLiteral("Task 1: draft.odt"), QStringLiteral("c.txt")};

		collection.processedFilesCount = 1;
		collection.markCurrentFileFailed(QStringLiteral("not supported"));
		collection.processedFilesCount = 3;

		QVERIFY(collection.isFailedFile(0) == false);
		QVERIFY(collection.isFailedFile(1));
		QVERIFY(collection.isFailedFile(2) == false);
		QCOMPARE(collection.failedFiles.size(), 1);
		QCOMPARE(collection.failedFiles.value(1), QStringLiteral("not supported"));
		// a failed file is processed, so the collection still completes
		QCOMPARE(collection.progress(), 100);
	}

	void discardRemovesFileOpenedSafely()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const auto filePath = dir.filePath(QStringLiteral("partial.txt"));

		FileCollection collection;
		QVERIFY(openOutputFile(collection, filePath));
		QCOMPARE(collection.currentOutputFile->write("partial"), 7);
		QVERIFY(QFile::exists(filePath));

		collection.discardCurrentOutputFile();

		QVERIFY(collection.currentOutputFile == nullptr);
		QVERIFY(collection.currentOutputFilePath.isEmpty());
		QCOMPARE(collection.currentFileSize, 0);
		QVERIFY(collection.currentTransferId.isNull());
		QVERIFY(QFile::exists(filePath) == false);
	}

	void closeKeepsFile()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const auto filePath = dir.filePath(QStringLiteral("complete.txt"));

		FileCollection collection;
		QVERIFY(openOutputFile(collection, filePath));
		QCOMPARE(collection.currentOutputFile->write("complete"), 8);

		collection.closeCurrentOutputFile();

		QVERIFY(collection.currentOutputFile == nullptr);
		QVERIFY(collection.currentOutputFilePath.isEmpty());
		QFile file(filePath);
		QVERIFY(file.open(QFile::ReadOnly));
		QCOMPARE(file.readAll(), QByteArray("complete"));
	}

	void discardWithoutOpenedFileRemovesNothing()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const auto filePath = dir.filePath(QStringLiteral("existing.txt"));
		QFile existingFile(filePath);
		QVERIFY(existingFile.open(QFile::WriteOnly));
		existingFile.close();

		// e.g. openFileSafely() refused the file: the path is not recorded, so the file is not removed
		FileCollection collection;
		collection.currentOutputFile = new QFile(filePath);
		collection.discardCurrentOutputFile();

		QVERIFY(collection.currentOutputFile == nullptr);
		QVERIFY(QFile::exists(filePath));
	}

private:
	static bool openOutputFile(FileCollection& collection, const QString& filePath)
	{
		collection.currentOutputFile = new QFile(filePath);
		if (VeyonCore::platform().filesystemFunctions().openFileSafely(
				collection.currentOutputFile, QFile::WriteOnly | QFile::Truncate,
				QFile::ReadOwner | QFile::WriteOwner) == false)
		{
			return false;
		}
		collection.currentOutputFilePath = filePath;
		collection.currentTransferId = QUuid::createUuid();
		collection.currentFileSize = 100;
		return true;
	}

	QTemporaryDir m_home;
	std::unique_ptr<VeyonCore> m_core;
};



int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();
	QCoreApplication application(argc, argv);
	FileCollectTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "FileCollectTest.moc"
