// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "SniffClassifier.h"

Q_DECLARE_METATYPE(SniffClassifier::Result)

// TLS/plain decision on the first bytes of a connection (05-tls.md §1.2, §10)
class SniffClassifierTest : public QObject
{
	Q_OBJECT
private:
	static constexpr qint64 SniffTimeout = 200;

private Q_SLOTS:
	void classify_data()
	{
		QTest::addColumn<QByteArray>("bytes");
		QTest::addColumn<qint64>("elapsed");
		QTest::addColumn<SniffClassifier::Result>("expected");

		using Result = SniffClassifier::Result;

		// TLS ClientHello record headers (legacy record version 3.1 as sent by TLS 1.2/1.3 clients,
		// 3.0, 3.3 and unknown minor versions)
		QTest::newRow("ClientHello TLS 1.0 record") << QByteArray::fromHex("160301") << qint64(0) << Result::Tls;
		QTest::newRow("ClientHello TLS 1.2 record") << QByteArray::fromHex("16030300f4") << qint64(0) << Result::Tls;
		QTest::newRow("ClientHello SSL 3 record") << QByteArray::fromHex("1603000010") << qint64(0) << Result::Tls;
		QTest::newRow("unknown minor version") << QByteArray::fromHex("1603ff") << qint64(0) << Result::Tls;
		QTest::newRow("two bytes only") << QByteArray::fromHex("1603") << qint64(0) << Result::Tls;
		QTest::newRow("two bytes at timeout") << QByteArray::fromHex("1603") << SniffTimeout << Result::Tls;

		// plain RFB clients and junk
		QTest::newRow("RFB") << QByteArrayLiteral("RFB") << qint64(0) << Result::Plain;
		QTest::newRow("RFB version") << QByteArrayLiteral("RFB 003.008\n") << qint64(0) << Result::Plain;
		QTest::newRow("HTTP") << QByteArrayLiteral("GET / HTTP/1.1\r\n") << qint64(0) << Result::Plain;
		QTest::newRow("NUL bytes") << QByteArray(16, '\0') << qint64(0) << Result::Plain;
		QTest::newRow("other TLS record type") << QByteArray::fromHex("170303") << qint64(0) << Result::Plain;
		QTest::newRow("alert record") << QByteArray::fromHex("150303") << qint64(0) << Result::Plain;
		QTest::newRow("handshake type, wrong major") << QByteArray::fromHex("160201") << qint64(0) << Result::Plain;
		QTest::newRow("SSLv2 ClientHello") << QByteArray::fromHex("80310103") << qint64(0) << Result::Plain;
		QTest::newRow("0xff bytes") << QByteArray::fromHex("ffff") << qint64(0) << Result::Plain;

		// waiting for more data within the sniff window
		QTest::newRow("no bytes") << QByteArray() << qint64(0) << Result::NeedMoreData;
		QTest::newRow("no bytes before timeout") << QByteArray() << SniffTimeout - 1 << Result::NeedMoreData;
		QTest::newRow("single TLS byte") << QByteArray::fromHex("16") << qint64(10) << Result::NeedMoreData;
		QTest::newRow("single junk byte") << QByteArrayLiteral("R") << qint64(10) << Result::NeedMoreData;

		// sniff timeout expired: plain (a waiting 4.11 master or a delayed/slowloris ClientHello)
		QTest::newRow("no bytes at timeout") << QByteArray() << SniffTimeout << Result::Plain;
		QTest::newRow("no bytes after timeout") << QByteArray() << SniffTimeout * 10 << Result::Plain;
		QTest::newRow("single TLS byte at timeout") << QByteArray::fromHex("16") << SniffTimeout << Result::Plain;
		QTest::newRow("single junk byte at timeout") << QByteArrayLiteral("R") << SniffTimeout << Result::Plain;
	}

	void classify()
	{
		QFETCH(QByteArray, bytes);
		QFETCH(qint64, elapsed);
		QFETCH(SniffClassifier::Result, expected);

		QCOMPARE(SniffClassifier::classify(bytes, elapsed, SniffTimeout), expected);
	}

	// every input of up to two bytes with any timing yields a decision once the window expired
	void exhaustiveTwoBytes()
	{
		for (int first = 0; first < 256; ++first)
		{
			for (int second = 0; second < 256; ++second)
			{
				QByteArray bytes;
				bytes.append(char(first));
				bytes.append(char(second));
				const auto expected = (first == 0x16 && second == 0x03) ? SniffClassifier::Result::Tls :
																		  SniffClassifier::Result::Plain;
				QCOMPARE(SniffClassifier::classify(bytes, 0, SniffTimeout), expected);
				QCOMPARE(SniffClassifier::classify(bytes + QByteArrayLiteral("trailing"), SniffTimeout, SniffTimeout), expected);
			}

			QCOMPARE(SniffClassifier::classify(QByteArray(1, char(first)), 0, SniffTimeout),
					 SniffClassifier::Result::NeedMoreData);
			QCOMPARE(SniffClassifier::classify(QByteArray(1, char(first)), SniffTimeout, SniffTimeout),
					 SniffClassifier::Result::Plain);
		}
	}
};


QTEST_GUILESS_MAIN(SniffClassifierTest)
#include "SniffClassifierTest.moc"
