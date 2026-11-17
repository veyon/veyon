// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "CaptureHelperCredentials.h"

using VeyonCrypto::SecureBuffer;

// stdin line of veyon-capture-helper: "VEYONCAP1 <hex64> <socketName>\n" with a 32-byte VNC
// password, the name of its Unix domain socket and nothing else (02-ipc.md §1a.5)
class CaptureHelperCredentialsTest : public QObject
{
	Q_OBJECT
private:
	static SecureBuffer buffer(const QByteArray& data)
	{
		return SecureBuffer(data.constData(), data.size());
	}

	static bool parses(const QByteArray& line)
	{
		auto lineBuffer = buffer(line);
		return CaptureHelperCredentials::parse(lineBuffer).has_value();
	}

	static QString validSocketName()
	{
		return QStringLiteral("capture-3-0123abcd.sock");
	}

	static QByteArray validLine()
	{
		return QByteArray("VEYONCAP1 ") + QByteArray(32, '\x5a').toHex() + ' ' + validSocketName().toLatin1();
	}

	static bool creates(const QByteArray& password, const QString& socketName = validSocketName())
	{
		return CaptureHelperCredentials::create(buffer(password), socketName).has_value();
	}

private Q_SLOTS:
	void roundTrip()
	{
		auto password = CaptureHelperCredentials::generatePassword();
		const auto socketName = CaptureHelperCredentials::generateSocketName(7);
		QVERIFY(password.has_value());
		QVERIFY(socketName.has_value());
		QVERIFY(socketName->startsWith(QStringLiteral("capture-7-")));
		const auto expectedPassword = password->toUnprotectedByteArray();

		const auto credentials = CaptureHelperCredentials::create(std::move(*password), *socketName);
		QVERIFY(credentials.has_value());

		const auto line = credentials->toLine().toUnprotectedByteArray();
		QCOMPARE(line, QByteArray("VEYONCAP1 ") + expectedPassword.toHex() + ' ' + socketName->toLatin1() + '\n');
		QVERIFY(line.size() <= int(CaptureHelperCredentials::MaximumLineSize) + 1);

		// what StandardInputReader::readSecretLine() returns: without the newline
		auto lineBuffer = buffer(line.chopped(1));
		const auto parsed = CaptureHelperCredentials::parse(lineBuffer);
		QVERIFY(parsed.has_value());
		QCOMPARE(parsed->password().toUnprotectedByteArray(), expectedPassword);
		QCOMPARE(parsed->socketName(), *socketName);

		// the line is wiped after parsing
		QVERIFY(lineBuffer.isEmpty());
	}

	void generatedPassword()
	{
		const auto password = CaptureHelperCredentials::generatePassword();
		QVERIFY(password.has_value());
		QCOMPARE(password->size(), qsizetype(32));
		for (qsizetype i = 0; i < password->size(); ++i)
		{
			QVERIFY(password->bytes()[i] & 0x80);
		}
		QVERIFY(creates(password->toUnprotectedByteArray()));
	}

	void passwordSize()
	{
		QVERIFY(creates(QByteArray(32, 'x')));
		// e.g. the 8-character fallback password of VncServer must not be accepted
		QVERIFY(creates(QByteArray(8, 'x')) == false);
		QVERIFY(creates(QByteArray(31, 'x')) == false);
		QVERIFY(creates(QByteArray(33, 'x')) == false);
		QVERIFY(creates({}) == false);

		// a NUL byte within the 8-byte DES key would end the key for libvncserver's rfbEncryptBytes()
		auto keyWithNul = QByteArray(32, 'x');
		keyWithNul[7] = '\0';
		QVERIFY(creates(keyWithNul) == false);
		auto nulAfterKey = QByteArray(32, 'x');
		nulAfterKey[8] = '\0';
		QVERIFY(creates(nulAfterKey));
	}

	void socketNames()
	{
		QVERIFY(CaptureHelperCredentials::isValidSocketName(validSocketName()));
		QVERIFY(CaptureHelperCredentials::isValidSocketName(QStringLiteral("capture-123456789-ffffffff.sock")));

		const auto invalidNames = {
			QString(),
			QStringLiteral("capture-1234567890-0123abcd.sock"),
			QStringLiteral("capture--0123abcd.sock"),
			QStringLiteral("capture-3-0123ABCD.sock"),
			QStringLiteral("capture-3-0123abc.sock"),
			QStringLiteral("capture-3-0123abcd"),
			QStringLiteral("capture-3-0123abcd.sock.x"),
			QStringLiteral("../capture-3-0123abcd.sock"),
			QStringLiteral("/tmp/capture-3-0123abcd.sock"),
			QStringLiteral("veyon-ipc-3-0123abcd.sock"),
			QStringLiteral("capture-3-0123abcd.sock\n"),
		};
		for (const auto& name : invalidNames)
		{
			QVERIFY2(CaptureHelperCredentials::isValidSocketName(name) == false, qPrintable(name));
			QVERIFY(creates(QByteArray(32, 'x'), name) == false);
		}

		QVERIFY(CaptureHelperCredentials::generateSocketName(-1).has_value() == false);
	}

	void malformedLines()
	{
		const auto passwordHex = QByteArray(32, '\x5a').toHex();
		const auto socketName = validSocketName().toLatin1();

		QVERIFY(parses(validLine()));
		// upper-case hex is accepted by the strict decoder as well
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex.toUpper() + ' ' + socketName));

		QVERIFY(parses(QByteArray()) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1")) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1 ")) == false);
		// without socket name (the format of the first WP16 version)
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex + ' ') == false);
		// other prefixes, e.g. the IPC credentials line of veyon-server
		QVERIFY(parses(QByteArray("VEYONIPC1 ") + passwordHex + ' ' + socketName) == false);
		QVERIFY(parses(QByteArray("VEYONCAP2 ") + passwordHex + ' ' + socketName) == false);
		QVERIFY(parses(QByteArray("veyoncap1 ") + passwordHex + ' ' + socketName) == false);
		// separators
		QVERIFY(parses(QByteArray("VEYONCAP1\t") + passwordHex + ' ' + socketName) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex + '\t' + socketName) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex + "  " + socketName) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1  ") + passwordHex.chopped(1) + ' ' + socketName) == false);
		// short or long password, additional fields, socket path instead of name
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex.chopped(2) + ' ' + socketName) == false);
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex + "00 " + socketName) == false);
		QVERIFY(parses(validLine() + " x") == false);
		QVERIFY(parses(validLine() + '\r') == false);
		QVERIFY(parses(QByteArray("VEYONCAP1 ") + passwordHex + " /tmp/" + socketName) == false);
		QVERIFY(parses(validLine() + QByteArray(200, 'x')) == false);
		// non-hex digits, NUL byte
		auto line = validLine();
		line[20] = 'g';
		QVERIFY(parses(line) == false);
		line[20] = '\0';
		QVERIFY(parses(line) == false);
		line = validLine();
		line[line.size() - 6] = '\0';
		QVERIFY(parses(line) == false);
	}

	void lineWipedOnFailure()
	{
		auto lineBuffer = buffer(validLine() + '0');
		QVERIFY(CaptureHelperCredentials::parse(lineBuffer).has_value() == false);
		QVERIFY(lineBuffer.isEmpty());
	}
};


QTEST_GUILESS_MAIN(CaptureHelperCredentialsTest)
#include "CaptureHelperCredentialsTest.moc"
