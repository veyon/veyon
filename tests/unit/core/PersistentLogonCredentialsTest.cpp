// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSettings>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

#include "PersistentLogonCredentials.h"

using Veyon::Crypto::SecureBuffer;
using Password = PersistentLogonCredentials::Password;

// 05-tls.md §7: the service data blob holds the plain logon password in SecureBuffers only (the
// zeroisation of SecureBuffer itself is tested in VeyonCryptoSecureBufferTest)
class PersistentLogonCredentialsTest : public QObject
{
	Q_OBJECT
private:
	static QByteArray field(const QByteArray& data)
	{
		const auto size = qToBigEndian<quint32>(quint32(data.size()));
		return QByteArray(reinterpret_cast<const char*>(&size), sizeof(size)) + data;
	}

	static SecureBuffer blob(const QByteArray& data)
	{
		return SecureBuffer::fromByteArray(data);
	}

	static bool decodes(const QByteArray& data)
	{
		QString username = QStringLiteral("unchanged");
		Password password(QByteArray("unchanged"));
		const auto result = PersistentLogonCredentials::decode(blob(data), &username, &password);
		if (result == false)
		{
			// outputs are left alone on failure
			if (username != QLatin1String("unchanged") || password.toByteArray() != "unchanged")
			{
				qWarning() << "outputs modified";
				return true;
			}
		}
		return result;
	}

	QTemporaryDir m_configurationDirectory;
	VeyonCore* m_core = nullptr;

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_configurationDirectory.isValid());
		// never read or write a real configuration
		QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, m_configurationDirectory.filePath(QStringLiteral("system")));
		QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_configurationDirectory.filePath(QStringLiteral("user")));

		m_core = new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::Server, QStringLiteral("LogonCredentialsTest"),
							   // QCA (PersistentLogonCredentials::Password) needs the legacy crypto
							   VeyonCore::InitializationFlag::LegacyCrypto);
	}

	void cleanupTestCase()
	{
		delete m_core;
		m_core = nullptr;
	}

	// without the ServiceIpc capability there is no service connection
	void readWriteWithoutService()
	{
		QVERIFY(VeyonCore::serviceIpcClient() == nullptr);

		QString username;
		Password password;
		QCOMPARE(PersistentLogonCredentials::read(&username, &password), false);
		QCOMPARE(PersistentLogonCredentials::read(nullptr, &password), false);

		QCOMPARE(PersistentLogonCredentials::write(QStringLiteral("alice"), Password(QByteArray("secret"))), false);
		QCOMPARE(PersistentLogonCredentials::write(QStringLiteral("alice"), Password()), false);
	}

	void roundTrip_data()
	{
		QTest::addColumn<QString>("username");
		QTest::addColumn<QByteArray>("password");

		QTest::newRow("ascii") << QStringLiteral("alice") << QByteArray("secret");
		QTest::newRow("domain user") << QStringLiteral("EXAMPLE\\alice") << QByteArray("p@ss w0rd");
		QTest::newRow("unicode") << QStringLiteral("jürgen.müller") << QByteArray("pässwörd€");
		QTest::newRow("binary password") << QStringLiteral("bob") << QByteArray("\x00\x01\xff\x00", 4);
		QTest::newRow("maximum sizes") << QString(PersistentLogonCredentials::MaximumUsernameSize, QLatin1Char('u'))
									   << QByteArray(PersistentLogonCredentials::MaximumPasswordSize, 'p');
	}

	void roundTrip()
	{
		QFETCH(QString, username);
		QFETCH(QByteArray, password);

		const auto data = PersistentLogonCredentials::encode(username, Password(password));
		QVERIFY(data.has_value());

		// plain password in the blob (no default-pkey.pem encryption any more)
		QCOMPARE(data->toUnprotectedByteArray(), field(username.toUtf8()) + field(password));

		QString decodedUsername;
		Password decodedPassword;
		QVERIFY(PersistentLogonCredentials::decode(*data, &decodedUsername, &decodedPassword));
		QCOMPARE(decodedUsername, username);
		QCOMPARE(decodedPassword.toByteArray(), password);
	}

	void encodeRejectsInvalidCredentials_data()
	{
		QTest::addColumn<QString>("username");
		QTest::addColumn<QByteArray>("password");

		QTest::newRow("empty username") << QString() << QByteArray("secret");
		QTest::newRow("empty password") << QStringLiteral("alice") << QByteArray();
		QTest::newRow("username too long") << QString(PersistentLogonCredentials::MaximumUsernameSize + 1, QLatin1Char('u'))
										   << QByteArray("secret");
		// UTF-8 size counts
		QTest::newRow("username too long in UTF-8") << QString(PersistentLogonCredentials::MaximumUsernameSize / 2 + 1, QChar(0xfc))
													<< QByteArray("secret");
		QTest::newRow("password too long") << QStringLiteral("alice")
										   << QByteArray(PersistentLogonCredentials::MaximumPasswordSize + 1, 'p');
	}

	void encodeRejectsInvalidCredentials()
	{
		QFETCH(QString, username);
		QFETCH(QByteArray, password);

		QVERIFY(PersistentLogonCredentials::encode(username, password.isEmpty() ? Password() : Password(password)).has_value() == false);
	}

	void decodeRejectsMalformedData_data()
	{
		QTest::addColumn<QByteArray>("data");

		const auto username = field("alice");
		const auto password = field("secret");

		QTest::newRow("empty") << QByteArray();
		QTest::newRow("truncated username size") << username.left(3);
		QTest::newRow("truncated username") << username.chopped(1);
		QTest::newRow("no password") << username;
		QTest::newRow("truncated password size") << username + password.left(3);
		QTest::newRow("truncated password") << username + password.chopped(1);
		QTest::newRow("trailing data") << username + password + "x";
		QTest::newRow("empty username") << field({}) + password;
		QTest::newRow("empty password") << username + field({});
		QTest::newRow("username size beyond data") << QByteArray("\x00\x00\x00\x10", 4) + "alice" + password;
		QTest::newRow("huge username size") << QByteArray("\xff\xff\xff\xff", 4) + "alice" + password;
		QTest::newRow("huge password size") << username + QByteArray("\xff\xff\xff\xff", 4) + "secret";
		QTest::newRow("username too long") << field(QByteArray(PersistentLogonCredentials::MaximumUsernameSize + 1, 'u')) + password;
		QTest::newRow("password too long") << username + field(QByteArray(PersistentLogonCredentials::MaximumPasswordSize + 1, 'p'));
		// 4.11 format (VariantStream with a default-pkey.pem ciphertext) is not accepted
		QTest::newRow("legacy format") << QByteArray::fromHex("0000000a0000000a0061006c0069006300650000000c00000000046162636400");
	}

	void decodeRejectsMalformedData()
	{
		QFETCH(QByteArray, data);

		QCOMPARE(decodes(data), false);
	}

	void decodeChecksOutputPointers()
	{
		const auto data = PersistentLogonCredentials::encode(QStringLiteral("alice"), Password(QByteArray("secret")));
		QVERIFY(data.has_value());

		QString username;
		Password password;
		QCOMPARE(PersistentLogonCredentials::decode(*data, nullptr, &password), false);
		QCOMPARE(PersistentLogonCredentials::decode(*data, &username, nullptr), false);
	}
};


QTEST_GUILESS_MAIN(PersistentLogonCredentialsTest)
#include "PersistentLogonCredentialsTest.moc"
