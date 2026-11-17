// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTemporaryDir>
#include <QtTest>

#include "SecretInputOutput.h"

#include <cstdio>
#ifndef Q_OS_WIN
#include <sys/stat.h>
#include <unistd.h>
#endif

// veyon-cli input of secrets and writing of exports (WP4 review M2, L1)
class SecretInputOutputTest : public QObject
{
	Q_OBJECT
private:
	static SecretInputOutput::LineResult readLine(const QByteArray& input, QByteArray* line = nullptr)
	{
		auto file = tmpfile();
		fwrite(input.constData(), 1, size_t(input.size()), file);
		rewind(file);
		Veyon::Crypto::SecureBuffer buffer;
		const auto result = SecretInputOutput::readLine(fileno(file), buffer);
		fclose(file);
		if (line)
		{
			*line = buffer.toUnprotectedByteArray();
		}
		return result;
	}

private Q_SLOTS:
	void readLine()
	{
		QByteArray line;
		QCOMPARE(readLine("secret\nrest", &line), SecretInputOutput::LineResult::Read);
		QCOMPARE(line, QByteArray("secret"));
		QCOMPARE(readLine("secret\r\n", &line), SecretInputOutput::LineResult::Read);
		QCOMPARE(line, QByteArray("secret"));
		// input without a final newline (printf secret | veyon-cli config set ... -)
		QCOMPARE(readLine("secret", &line), SecretInputOutput::LineResult::Read);
		QCOMPARE(line, QByteArray("secret"));
		QCOMPARE(readLine("\n", &line), SecretInputOutput::LineResult::Read);
		QVERIFY(line.isEmpty());
		// /dev/null and the like
		QCOMPARE(readLine(""), SecretInputOutput::LineResult::NoInput);
		QCOMPARE(readLine(QByteArray("sec\0ret\n", 8)), SecretInputOutput::LineResult::Invalid);
		QCOMPARE(readLine(QByteArray(SecretInputOutput::MaximumLineLength + 1, 'x')), SecretInputOutput::LineResult::Invalid);
		QCOMPARE(readLine(QByteArray(SecretInputOutput::MaximumLineLength, 'x')), SecretInputOutput::LineResult::Read);
	}

	void writeFile()
	{
		QTemporaryDir directory;
		const auto fileName = directory.filePath(QStringLiteral("export.json"));

		QString error;
		QVERIFY(SecretInputOutput::writeFile(fileName, "first", true, &error));
		QVERIFY(SecretInputOutput::writeFile(fileName, "second", true, &error));
		QFile file(fileName);
		QVERIFY(file.open(QFile::ReadOnly));
		QCOMPARE(file.readAll(), QByteArray("second"));
		// no temporary files left
		QCOMPARE(QDir(directory.path()).entryList(QDir::Files | QDir::Hidden).size(), 1);

#ifndef Q_OS_WIN
		struct stat status{};
		QCOMPARE(stat(QFile::encodeName(fileName).constData(), &status), 0);
		QCOMPARE(status.st_mode & 0777, mode_t(0600));

		// an existing world-readable target doesn't make the export world-readable
		QVERIFY(QFile::setPermissions(fileName, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadOther | QFile::WriteOther));
		QVERIFY(SecretInputOutput::writeFile(fileName, "third", true, &error));
		QCOMPARE(stat(QFile::encodeName(fileName).constData(), &status), 0);
		QCOMPARE(status.st_mode & 0777, mode_t(0600));

		// plain exports (no secrets) keep the mode of an existing file, follow symlinks and can go
		// to devices (WP4 re-review N5)
		QVERIFY(SecretInputOutput::writeFile(fileName, "plain", false, &error));
		QCOMPARE(stat(QFile::encodeName(fileName).constData(), &status), 0);
		QCOMPARE(status.st_mode & 0777, mode_t(0600));
		QVERIFY(SecretInputOutput::writeFile(QStringLiteral("/dev/null"), "plain", false, &error));
		const auto plainTarget = directory.filePath(QStringLiteral("plain-target"));
		const auto plainLink = directory.filePath(QStringLiteral("plain-link.json"));
		QCOMPARE(symlink(QFile::encodeName(plainTarget).constData(), QFile::encodeName(plainLink).constData()), 0);
		QVERIFY(SecretInputOutput::writeFile(plainLink, "plain", false, &error));
		QFile plainTargetFile(plainTarget);
		QVERIFY(plainTargetFile.open(QFile::ReadOnly));
		QCOMPARE(plainTargetFile.readAll(), QByteArray("plain"));

		// symlinks are replaced, never followed
		const auto target = directory.filePath(QStringLiteral("target"));
		{
			QFile targetFile(target);
			QVERIFY(targetFile.open(QFile::WriteOnly));
			targetFile.write("untouched");
		}
		const auto link = directory.filePath(QStringLiteral("link.json"));
		QCOMPARE(symlink(QFile::encodeName(target).constData(), QFile::encodeName(link).constData()), 0);
		QVERIFY(SecretInputOutput::writeFile(link, "secret", true, &error) == false);
		QFile targetFile(target);
		QVERIFY(targetFile.open(QFile::ReadOnly));
		QCOMPARE(targetFile.readAll(), QByteArray("untouched"));

		// a file of another user (e.g. pre-created world-writable) is refused
		if (geteuid() == 0)
		{
			QCOMPARE(chown(QFile::encodeName(fileName).constData(), 65534, 65534), 0);
			QVERIFY(SecretInputOutput::writeFile(fileName, "secret", true, &error) == false);
			QVERIFY(error.isEmpty() == false);
		}
#endif

		QVERIFY(SecretInputOutput::writeFile(directory.filePath(QStringLiteral("missing/x.json")), "x", true, &error) == false);
	}
};


QTEST_GUILESS_MAIN(SecretInputOutputTest)
#include "SecretInputOutputTest.moc"
