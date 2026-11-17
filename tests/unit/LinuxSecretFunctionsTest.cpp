// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTemporaryDir>
#include <QtTest>

#include "LinuxSecretFunctions.h"
#include "SecretStore.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using Veyon::Crypto::SecureBuffer;

// Linux SecretStore backend (03-secretstore-config.md §2, §6): permissions, symlink/hard link
// swaps, ownership, atomic replacement
class LinuxSecretFunctionsTest : public QObject
{
	Q_OBJECT
private:
	static constexpr auto Key = "ldap/bindPassword";
	static constexpr auto FileName = "ldap__bindPassword";
	static constexpr auto OldValue = "OldSecret-9f2c";
	static constexpr auto NewValue = "NewSecret-7a1e";

	static QStringList* s_logMessages;

	QStringList m_logMessages;
	std::unique_ptr<QTemporaryDir> m_temporaryDirectory;
	QString m_secretDirectory;

	static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_logMessages->append(message);
	}

	static QString key()
	{
		return QLatin1String(Key);
	}

	QString secretFile() const
	{
		return m_secretDirectory + QLatin1Char('/') + QLatin1String(FileName);
	}

	static mode_t mode(const QString& path)
	{
		struct stat status{};
		if (lstat(QFile::encodeName(path).constData(), &status) != 0)
		{
			return 0;
		}
		return status.st_mode & 07777;
	}

	static QByteArray value(LinuxSecretFunctions& backend)
	{
		const auto secret = backend.readSecret(key());
		return secret.has_value() ? secret->toUnprotectedByteArray() : QByteArray();
	}

	LinuxSecretFunctions backend() const
	{
		return LinuxSecretFunctions(m_secretDirectory, geteuid());
	}

	bool writeOldValue()
	{
		auto secrets = backend();
		return secrets.writeSecret(key(), SecureBuffer::fromByteArray(OldValue));
	}

private Q_SLOTS:
	void init()
	{
		m_temporaryDirectory = std::make_unique<QTemporaryDir>();
		QVERIFY(m_temporaryDirectory->isValid());
		// not created yet: the backend creates it (and its parent) on the first write
		m_secretDirectory = m_temporaryDirectory->filePath(QStringLiteral("veyon/secrets.d"));
		m_logMessages.clear();
		s_logMessages = &m_logMessages;
		qInstallMessageHandler(captureMessage);
	}

	void cleanup()
	{
		qInstallMessageHandler(nullptr);
		for (const auto& message : std::as_const(m_logMessages))
		{
			QVERIFY2(message.contains(QLatin1String(OldValue)) == false, qPrintable(message));
			QVERIFY2(message.contains(QLatin1String(NewValue)) == false, qPrintable(message));
		}
		SecretStore::setTestBackend(nullptr, VeyonCore::InitializationFlags());
		m_temporaryDirectory.reset();
	}

	void roundtrip()
	{
		auto secrets = backend();
		QVERIFY(secrets.isAvailable());
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QVERIFY(secrets.secretKeys().isEmpty());
		QVERIFY(secrets.removeSecret(key()));

		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(OldValue)));
		QCOMPARE(value(secrets), QByteArray(OldValue));
		QCOMPARE(mode(m_secretDirectory), mode_t(0700));
		QCOMPARE(mode(secretFile()), mode_t(0600));

		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue)));
		QCOMPARE(value(secrets), QByteArray(NewValue));
		QVERIFY(secrets.writeSecret(QStringLiteral("tls/hostKey.next"), SecureBuffer()));
		QVERIFY(secrets.readSecret(QStringLiteral("tls/hostKey.next")).has_value());
		QVERIFY(secrets.readSecret(QStringLiteral("tls/hostKey.next"))->isEmpty());

		auto keys = secrets.secretKeys();
		keys.sort();
		QCOMPARE(keys, QStringList({key(), QStringLiteral("tls/hostKey.next")}));

		QVERIFY(secrets.removeSecret(key()));
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QVERIFY(secrets.removeSecret(key()));
		QCOMPARE(secrets.secretKeys(), QStringList{QStringLiteral("tls/hostKey.next")});
	}

	// veyon-service start (also with nothing to write): creates the directory or verifies it
	void prepareStorage()
	{
		qputenv("VEYON_SECRET_DIR", QFile::encodeName(m_secretDirectory));
		LinuxSecretFunctions secrets;
		qunsetenv("VEYON_SECRET_DIR");

		QVERIFY(QFileInfo::exists(m_secretDirectory) == false);
		QVERIFY(secrets.prepareStorage());
		QCOMPARE(mode(m_secretDirectory), mode_t(0700));
		QVERIFY(secrets.secretKeys().isEmpty());
		QVERIFY(secrets.prepareStorage());

		// stale temporary files are removed
		const QString staleFile = m_secretDirectory + QStringLiteral("/.ldap__bindPassword.0011223344556677.tmp");
		{
			QFile file(staleFile);
			QVERIFY(file.open(QFile::WriteOnly));
		}
		const struct timespec times[2] = {{0, UTIME_OMIT}, {time(nullptr) - 3600, 0}};
		QCOMPARE(utimensat(AT_FDCWD, QFile::encodeName(staleFile).constData(), times, AT_SYMLINK_NOFOLLOW), 0);
		QVERIFY(secrets.prepareStorage());
		QVERIFY(QFileInfo::exists(staleFile) == false);

		// a directory with wrong permissions (or a symlink) is refused, not used
		QCOMPARE(chmod(QFile::encodeName(m_secretDirectory).constData(), 0755), 0);
		QVERIFY(secrets.prepareStorage() == false);
		QVERIFY(m_logMessages.filter(m_secretDirectory).isEmpty() == false);
		QCOMPARE(chmod(QFile::encodeName(m_secretDirectory).constData(), 0700), 0);

		const auto otherDirectory = m_temporaryDirectory->filePath(QStringLiteral("other"));
		QVERIFY(QDir().rename(m_secretDirectory, otherDirectory));
		QCOMPARE(symlink(QFile::encodeName(otherDirectory).constData(), QFile::encodeName(m_secretDirectory).constData()), 0);
		QVERIFY(secrets.prepareStorage() == false);
	}

	// production default /etc/veyon/secrets.d is replaced only via VEYON_SECRET_DIR in test builds
	void testDirectoryOverride()
	{
		{
			LinuxSecretFunctions defaultBackend;
			QCOMPARE(defaultBackend.isAvailable(), geteuid() == 0);
		}

		qputenv("VEYON_SECRET_DIR", QFile::encodeName(m_secretDirectory));
		LinuxSecretFunctions secrets;
		qunsetenv("VEYON_SECRET_DIR");

		QVERIFY(secrets.isAvailable());
		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(OldValue)));
		QVERIFY(QFileInfo::exists(secretFile()));

		// SecretStore on top of the backend
		SecretStore::setTestBackend(&secrets, VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::SecretStore);
		QCOMPARE(SecretStore::read(key())->toUnprotectedByteArray(), QByteArray(OldValue));
		QVERIFY(SecretStore::write(QStringLiteral("ldap/../../x"), SecureBuffer::fromByteArray("x")) == false);
		QCOMPARE(SecretStore::keys(), QStringList{key()});
		SecretStore::setTestBackend(&secrets, VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc);
		QVERIFY(SecretStore::read(key()).has_value() == false);
	}

	void refuseGroupOrWorldAccessibleFile_data()
	{
		QTest::addColumn<int>("fileMode");
		QTest::newRow("group readable") << 0640;
		QTest::newRow("world readable") << 0604;
		QTest::newRow("group writable") << 0620;
		QTest::newRow("world writable") << 0602;
	}

	void refuseGroupOrWorldAccessibleFile()
	{
		QFETCH(int, fileMode);
		QVERIFY(writeOldValue());
		QCOMPARE(chmod(QFile::encodeName(secretFile()).constData(), mode_t(fileMode)), 0);

		auto secrets = backend();
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QVERIFY(m_logMessages.filter(QLatin1String(FileName)).isEmpty() == false);
	}

	void refuseGroupOrWorldAccessibleDirectory_data()
	{
		QTest::addColumn<int>("directoryMode");
		QTest::newRow("group readable") << 0750;
		QTest::newRow("world executable") << 0701;
		QTest::newRow("world writable") << 0703;
	}

	void refuseGroupOrWorldAccessibleDirectory()
	{
		QFETCH(int, directoryMode);
		QVERIFY(writeOldValue());
		QCOMPARE(chmod(QFile::encodeName(m_secretDirectory).constData(), mode_t(directoryMode)), 0);

		auto secrets = backend();
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue)) == false);
		QVERIFY(secrets.secretKeys().isEmpty());
		QVERIFY(secrets.removeSecret(key()) == false);
		QVERIFY(m_logMessages.filter(m_secretDirectory).isEmpty() == false);

		// nothing was replaced
		QCOMPARE(chmod(QFile::encodeName(m_secretDirectory).constData(), 0700), 0);
		QCOMPARE(value(secrets), QByteArray(OldValue));
	}

	void refuseSymlinks()
	{
		QVERIFY(writeOldValue());
		auto secrets = backend();

		// file replaced by a symlink to a file with correct owner and mode
		const auto target = m_temporaryDirectory->filePath(QStringLiteral("target"));
		{
			QFile targetFile(target);
			QVERIFY(targetFile.open(QFile::WriteOnly));
			targetFile.write(NewValue);
			QVERIFY(targetFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
		}
		QVERIFY(QFile::remove(secretFile()));
		QCOMPARE(symlink(QFile::encodeName(target).constData(), QFile::encodeName(secretFile()).constData()), 0);
		QVERIFY(secrets.readSecret(key()).has_value() == false);

		// writes replace the link itself, never the link target
		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(OldValue)));
		QCOMPARE(QFileInfo(secretFile()).isSymLink(), false);
		QFile targetFile(target);
		QVERIFY(targetFile.open(QFile::ReadOnly));
		QCOMPARE(targetFile.readAll(), QByteArray(NewValue));

		// directory replaced by a symlink to a directory with correct owner and mode
		const auto otherDirectory = m_temporaryDirectory->filePath(QStringLiteral("other"));
		QVERIFY(QDir().rename(m_secretDirectory, otherDirectory));
		QCOMPARE(symlink(QFile::encodeName(otherDirectory).constData(), QFile::encodeName(m_secretDirectory).constData()), 0);
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue)) == false);
		QVERIFY(secrets.secretKeys().isEmpty());
	}

	void refuseHardLinksAndSpecialFiles()
	{
		QVERIFY(writeOldValue());
		auto secrets = backend();

		const auto link = m_temporaryDirectory->filePath(QStringLiteral("link"));
		QCOMPARE(::link(QFile::encodeName(secretFile()).constData(), QFile::encodeName(link).constData()), 0);
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QCOMPARE(unlink(QFile::encodeName(link).constData()), 0);
		QCOMPARE(value(secrets), QByteArray(OldValue));

		// a FIFO must neither block nor be read
		QVERIFY(QFile::remove(secretFile()));
		QCOMPARE(mkfifo(QFile::encodeName(secretFile()).constData(), 0600), 0);
		QVERIFY(secrets.readSecret(key()).has_value() == false);

		QVERIFY(QFile::remove(secretFile()));
		QVERIFY(QDir().mkdir(secretFile()));
		QCOMPARE(chmod(QFile::encodeName(secretFile()).constData(), 0700), 0);
		QVERIFY(secrets.readSecret(key()).has_value() == false);
	}

	void refuseWrongOwner()
	{
		QVERIFY(writeOldValue());

		if (geteuid() == 0)
		{
			// real ownership checks (expected owner root)
			static constexpr uid_t Nobody = 65534;
			LinuxSecretFunctions secrets(m_secretDirectory, 0);
			QCOMPARE(value(secrets), QByteArray(OldValue));

			QCOMPARE(chown(QFile::encodeName(secretFile()).constData(), Nobody, Nobody), 0);
			QVERIFY(secrets.readSecret(key()).has_value() == false);
			QCOMPARE(chown(QFile::encodeName(secretFile()).constData(), 0, 0), 0);

			QCOMPARE(chown(QFile::encodeName(m_secretDirectory).constData(), Nobody, Nobody), 0);
			QVERIFY(secrets.readSecret(key()).has_value() == false);
			QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue)) == false);
			QCOMPARE(chown(QFile::encodeName(m_secretDirectory).constData(), 0, 0), 0);
			QCOMPARE(value(secrets), QByteArray(OldValue));
		}

		// mock owner: files and directory belong to someone else than the expected owner, which
		// also isn't the effective user (not privileged)
		LinuxSecretFunctions secrets(m_secretDirectory, geteuid() + 1);
		QVERIFY(secrets.isAvailable() == false);
		QVERIFY(secrets.readSecret(key()).has_value() == false);
		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue)) == false);
	}

	// a writer killed between writing the temporary file and the rename leaves the old value
	void atomicReplacement()
	{
		QVERIFY(writeOldValue());

		const auto pid = fork();
		QVERIFY(pid >= 0);
		if (pid == 0)
		{
			auto secrets = backend();
			secrets.setBeforeRenameHook([]() { _exit(0); });
			(void)secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue));
			_exit(1);
		}

		int status = 0;
		QCOMPARE(waitpid(pid, &status, 0), pid);
		QVERIFY(WIFEXITED(status));
		QCOMPARE(WEXITSTATUS(status), 0);

		auto secrets = backend();
		QCOMPARE(value(secrets), QByteArray(OldValue));
		// the left-over temporary file is not a key
		QCOMPARE(QDir(m_secretDirectory).entryList(QDir::Files | QDir::Hidden).size(), 2);
		QCOMPARE(secrets.secretKeys(), QStringList{key()});

		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(NewValue)));
		QCOMPARE(value(secrets), QByteArray(NewValue));

		// recent temporary files (a concurrent writer) are kept, stale ones are removed by writes
		QCOMPARE(QDir(m_secretDirectory).entryList(QDir::Files | QDir::Hidden).size(), 2);
		const auto temporaryFile = QDir(m_secretDirectory).entryList({QStringLiteral(".*.tmp")}, QDir::Files | QDir::Hidden).value(0);
		QVERIFY(temporaryFile.isEmpty() == false);
		const struct timespec times[2] = {{0, UTIME_OMIT}, {time(nullptr) - 3600, 0}};
		QCOMPARE(utimensat(AT_FDCWD, QFile::encodeName(m_secretDirectory + QLatin1Char('/') + temporaryFile).constData(),
						   times, AT_SYMLINK_NOFOLLOW),
				 0);
		QVERIFY(secrets.writeSecret(key(), SecureBuffer::fromByteArray(OldValue)));
		QCOMPARE(QDir(m_secretDirectory).entryList(QDir::Files | QDir::Hidden), QStringList{QLatin1String(FileName)});
	}
};


QStringList* LinuxSecretFunctionsTest::s_logMessages = nullptr;


QTEST_GUILESS_MAIN(LinuxSecretFunctionsTest)
#include "LinuxSecretFunctionsTest.moc"
