// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "WebApiLogRedaction.h"

// debug output of the WebAPI (WebApiHttpServer) never contains the session token or credentials (05-tls.md §7a)
class WebApiLogRedactionTest : public QObject
{
	Q_OBJECT
private:
	static QByteArray marker()
	{
		return QByteArrayLiteral("0f1e2d3c-4b5a-6978-8796-a5b4c3d2e1f0");
	}

private Q_SLOTS:
	void sensitiveHeaders_data()
	{
		QTest::addColumn<QByteArray>("name");
		QTest::addColumn<bool>("sensitive");

		QTest::newRow("Connection-Uid") << WebApiController::connectionUidHeaderFieldName() << true;
		QTest::newRow("connection-uid") << QByteArray("connection-uid") << true;
		QTest::newRow("CONNECTION-UID") << QByteArray("CONNECTION-UID") << true;
		QTest::newRow("Authorization") << QByteArray("Authorization") << true;
		QTest::newRow("Proxy-Authorization") << QByteArray("Proxy-Authorization") << true;
		QTest::newRow("Cookie") << QByteArray("Cookie") << true;
		QTest::newRow("Set-Cookie") << QByteArray("Set-Cookie") << true;
		QTest::newRow("Host") << QByteArray("Host") << false;
		QTest::newRow("Content-Type") << QByteArray("Content-Type") << false;
	}

	void sensitiveHeaders()
	{
		QFETCH(QByteArray, name);
		QFETCH(bool, sensitive);

		QCOMPARE(WebApiLogRedaction::isSensitiveHeader(name), sensitive);

		const QList<QPair<QByteArray, QByteArray>> headerList{{QByteArrayLiteral("Host"), QByteArrayLiteral("example")},
															  {name, marker()}};
		const auto listOutput = WebApiLogRedaction::redactedHeaders(headerList);
		QCOMPARE(listOutput.contains(marker()), sensitive == false);
		QVERIFY(listOutput.contains("example"));
		QVERIFY(listOutput.contains(name));

		const QVariantMap headerMap{{QStringLiteral("host"), QStringLiteral("example")},
									{QString::fromUtf8(name), QString::fromUtf8(marker())}};
		const auto mapOutput = WebApiLogRedaction::redactedHeaders(headerMap);
		QCOMPARE(mapOutput.contains(marker()), sensitive == false);
		QVERIFY(mapOutput.contains("example"));
	}

	void redactedHeadersFormat()
	{
		const QList<QPair<QByteArray, QByteArray>> headers{{QByteArrayLiteral("Host"), QByteArrayLiteral("example")},
														   {QByteArrayLiteral("Connection-Uid"), marker()}};
		QCOMPARE(WebApiLogRedaction::redactedHeaders(headers),
				 QByteArray(R"([["Host","example"],["Connection-Uid","<redacted>"]])"));
	}

	void authenticationResponse()
	{
		// WebApiController::performAuthentication()
		const QVariantMap response{{QString::fromUtf8(WebApiController::connectionUidHeaderFieldName().toLower()),
									QString::fromUtf8(marker())},
								   {QStringLiteral("validUntil"), 1234}};
		const auto output = WebApiLogRedaction::redactedResponseData(response);
		QVERIFY(output.contains(QString::fromUtf8(marker())) == false);
		QCOMPARE(output, QStringLiteral("{connection-uid=<redacted>, validUntil=1234}"));
	}

	void redactedFormat_data()
	{
		QTest::addColumn<QVariantMap>("data");
		QTest::addColumn<QStringList>("sensitiveKeys");
		QTest::addColumn<QString>("expected");

		QTest::newRow("empty") << QVariantMap{} << QStringList{} << QStringLiteral("{}");
		QTest::newRow("values") << QVariantMap{{QStringLiteral("a"), QStringLiteral("x")}, {QStringLiteral("b"), 42}}
								<< QStringList{} << QStringLiteral(R"({a="x", b=42})");
		QTest::newRow("case-insensitive") << QVariantMap{{QStringLiteral("Credentials"), QStringLiteral("secret")}}
										  << QStringList{QStringLiteral("credentials")} << QStringLiteral("{Credentials=<redacted>}");
		// keys come from the request body of a client: no forged log lines, no unbounded length
		QTest::newRow("newline in key") << QVariantMap{{QStringLiteral("a\n2026-11-20T00:00:00.000: [INFO] forged line"), 1}}
										<< QStringList{} << QStringLiteral(R"({a\n2026-11-20T00:00:00.000: [INFO] forged line=1})");
		QTest::newRow("newline in sensitive key") << QVariantMap{{QStringLiteral("a\r\nb\"c"), QStringLiteral("secret")}}
												  << QStringList{QStringLiteral("a\r\nb\"c")} << QStringLiteral(R"({a\r\nb\"c=<redacted>})");
		QTest::newRow("long key truncated") << QVariantMap{{QString(100, QLatin1Char('k')), 1}}
											<< QStringList{} << QLatin1Char('{') + QString(64, QLatin1Char('k')) + QStringLiteral("...=1}");
	}

	void redactedFormat()
	{
		QFETCH(QVariantMap, data);
		QFETCH(QStringList, sensitiveKeys);
		QFETCH(QString, expected);

		const auto output = WebApiLogRedaction::redacted(data, sensitiveKeys);
		QCOMPARE(output, expected);
		QVERIFY(output.contains(QLatin1Char('\n')) == false);
		QVERIFY(output.contains(QLatin1Char('\r')) == false);
	}

	void requestData()
	{
		const QVariantMap request{{QStringLiteral("method"), QStringLiteral("0c69b301-81b4-42d6-8fae-128cdd113314")},
								  {QStringLiteral("credentials"), QVariantMap{{QStringLiteral("password"), QString::fromUtf8(marker())}}},
								  {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("password"), QString::fromUtf8(marker())}}}};
		const auto output = WebApiLogRedaction::redactedRequestData(request);
		QVERIFY(output.contains(QString::fromUtf8(marker())) == false);
		QVERIFY(output.contains(QStringLiteral("credentials=<redacted>")));
		QVERIFY(output.contains(QStringLiteral("arguments=<redacted>")));
		QVERIFY(output.contains(QStringLiteral("0c69b301")));
	}
};

QTEST_GUILESS_MAIN(WebApiLogRedactionTest)
#include "WebApiLogRedactionTest.moc"
