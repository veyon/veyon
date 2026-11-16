// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QtTest>

#include "PkiTestCertificates.h"
#include "pki/PkiPublicApi.h"

using namespace VeyonCrypto;


// collects all log messages to check that the enrollment token never appears in them
static QStringList s_logMessages;

static void collectLogMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	Q_UNUSED(type)
	Q_UNUSED(context)
	s_logMessages.append(message);
}



class TrustBundleTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void init();
	void cleanup();

	void parseCompleteBundle();
	void parseMinimalBundle();
	void roundTrip();
	void unknownFieldsAreIgnored();
	void invalidBundles_data();
	void invalidBundles();
	void sizeLimit();
	void caCertificatesPemContainsCertificatesOnly();
	void caKeyOutsideKeyPolicyIsRefused();
	void createRefusesInvalidBundle();
	void enrollmentUrl_data();
	void enrollmentUrl();
	void enrollmentToken_data();
	void enrollmentToken();
	void tokenIsNeverLogged();

private:
	QJsonObject completeBundle() const;

	static QByteArray toJson(const QJsonObject& object)
	{
		return QJsonDocument(object).toJson(QJsonDocument::Compact);
	}

	static QByteArray marker()
	{
		return QByteArrayLiteral("eyJhbGciOiJFZERTQSJ9.eyJtYXJrZXIiOiJTRUNSRVQifQ.c2lnbmF0dXJlTUFSS0VS");
	}

	Certificate m_caCertificate;
	Certificate m_secondCaCertificate;
	Certificate m_endEntityCertificate;
	QtMessageHandler m_previousMessageHandler = nullptr;
};



void TrustBundleTest::initTestCase()
{
	const auto now = QDateTime::fromSecsSinceEpoch(QDateTime::currentSecsSinceEpoch(), QTimeZone::utc());
	const auto caKey = PkiTestCertificates::generateKey();
	const auto secondCaKey = PkiTestCertificates::generateKey();
	const auto hostKey = PkiTestCertificates::generateKey();

	m_caCertificate = PkiTestCertificates::createCa(caKey, QStringLiteral("Veyon CA Test School"), now);
	m_secondCaCertificate = PkiTestCertificates::createCa(secondCaKey, QStringLiteral("Veyon CA Test School 2"), now);
	PkiTestCertificates::Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	m_endEntityCertificate = PkiTestCertificates::create(hostKey, specification, caKey, m_caCertificate.subjectCommonName(), now);

	QVERIFY(m_caCertificate.isNull() == false && m_secondCaCertificate.isNull() == false &&
			m_endEntityCertificate.isNull() == false);
}



void TrustBundleTest::init()
{
	s_logMessages.clear();
	m_previousMessageHandler = qInstallMessageHandler(collectLogMessage);
}



void TrustBundleTest::cleanup()
{
	qInstallMessageHandler(m_previousMessageHandler);
}



QJsonObject TrustBundleTest::completeBundle() const
{
	return QJsonObject{
		{QStringLiteral("v"), 1},
		{QStringLiteral("caCertificates"), QString::fromLatin1(QByteArray(m_caCertificate.toPem() + m_secondCaCertificate.toPem()))},
		{QStringLiteral("enrollment"), QJsonObject{
										   {QStringLiteral("url"), QStringLiteral("https://pki-host.example.org:11010")},
										   {QStringLiteral("token"), QString::fromLatin1(marker())},
									   }},
		{QStringLiteral("policy"), QJsonObject{
									   {QStringLiteral("certValidityDays"), 730},
									   {QStringLiteral("renewBeforeDays"), 180},
								   }},
		{QStringLiteral("modes"), QJsonObject{
									  {QStringLiteral("server"), QStringLiteral("Optional")},
									  {QStringLiteral("client"), QStringLiteral("Prefer")},
								  }},
	};
}



void TrustBundleTest::parseCompleteBundle()
{
	const auto bundle = TrustBundle::fromJson(toJson(completeBundle()));
	QVERIFY(bundle.has_value());

	QCOMPARE(bundle->caCertificates.size(), 2);
	QCOMPARE(bundle->caCertificates.at(0).spkiSha256(), m_caCertificate.spkiSha256());
	QCOMPARE(bundle->caCertificates.at(1).spkiSha256(), m_secondCaCertificate.spkiSha256());
	QCOMPARE(bundle->enrollmentUrl, QStringLiteral("https://pki-host.example.org:11010"));
	QCOMPARE(bundle->enrollmentToken.toUnprotectedByteArray(), marker());
	QCOMPARE(bundle->certValidityDays, std::optional<int>(730));
	QCOMPARE(bundle->renewBeforeDays, std::optional<int>(180));
	QVERIFY(bundle->serverMode == VeyonCore::TlsServerMode::Optional);
	QVERIFY(bundle->clientMode == VeyonCore::TlsClientMode::Prefer);
	QVERIFY(bundle->isValid());
}



void TrustBundleTest::parseMinimalBundle()
{
	// CA certificates only, e.g. for masters or an organisation CA without enrollment
	const auto bundle = TrustBundle::fromJson(toJson(QJsonObject{
		{QStringLiteral("v"), 1},
		{QStringLiteral("caCertificates"), QString::fromLatin1(m_caCertificate.toPem())},
	}));
	QVERIFY(bundle.has_value());
	QCOMPARE(bundle->caCertificates.size(), 1);
	QVERIFY(bundle->enrollmentUrl.isEmpty());
	QVERIFY(bundle->enrollmentToken.isEmpty());
	QVERIFY(bundle->certValidityDays.has_value() == false);
	QVERIFY(bundle->renewBeforeDays.has_value() == false);
	// modes are applied only if present
	QVERIFY(bundle->serverMode.has_value() == false);
	QVERIFY(bundle->clientMode.has_value() == false);

	// enrollment URL without token (window closed)
	const auto withoutToken = TrustBundle::fromJson(toJson(QJsonObject{
		{QStringLiteral("v"), 1},
		{QStringLiteral("caCertificates"), QString::fromLatin1(m_caCertificate.toPem())},
		{QStringLiteral("enrollment"), QJsonObject{{QStringLiteral("url"), QStringLiteral("https://192.0.2.1:11010/")}}},
		{QStringLiteral("modes"), QJsonObject{{QStringLiteral("server"), QStringLiteral("Required")}}},
	}));
	QVERIFY(withoutToken.has_value());
	QVERIFY(withoutToken->enrollmentToken.isEmpty());
	QVERIFY(withoutToken->serverMode == VeyonCore::TlsServerMode::Required);
	QVERIFY(withoutToken->clientMode.has_value() == false);

	// a UTF-8 BOM (e.g. from Windows editors) is accepted
	const auto withBom = QByteArray(QByteArrayLiteral("\xef\xbb\xbf") + toJson(completeBundle()));
	QVERIFY(TrustBundle::fromJson(withBom).has_value());
}



void TrustBundleTest::roundTrip()
{
	TrustBundle bundle;
	bundle.caCertificates = {m_caCertificate, m_secondCaCertificate};
	bundle.enrollmentUrl = QStringLiteral("https://pki-host.example.org:11010");
	bundle.enrollmentToken = SecureBuffer::fromByteArray(marker());
	bundle.certValidityDays = 365;
	bundle.renewBeforeDays = 30;
	bundle.serverMode = VeyonCore::TlsServerMode::Required;
	bundle.clientMode = VeyonCore::TlsClientMode::Disabled;

	const auto json = bundle.toJson();
	QVERIFY(json.has_value());
	QVERIFY(json->size() <= qsizetype(TrustBundle::MaximumSize));

	const auto parsed = TrustBundle::fromJson(*json);
	QVERIFY(parsed.has_value());
	QCOMPARE(parsed->caCertificates.size(), 2);
	QCOMPARE(parsed->caCertificates.at(0).toDer(), m_caCertificate.toDer());
	QCOMPARE(parsed->caCertificates.at(1).toDer(), m_secondCaCertificate.toDer());
	QCOMPARE(parsed->enrollmentUrl, bundle.enrollmentUrl);
	QCOMPARE(parsed->enrollmentToken.toUnprotectedByteArray(), marker());
	QCOMPARE(parsed->certValidityDays, bundle.certValidityDays);
	QCOMPARE(parsed->renewBeforeDays, bundle.renewBeforeDays);
	QVERIFY(parsed->serverMode == bundle.serverMode);
	QVERIFY(parsed->clientMode == bundle.clientMode);

	// field names and values as specified (06-pki.md §4)
	const auto object = QJsonDocument::fromJson(json->toUnprotectedByteArray()).object();
	QCOMPARE(object.value(QStringLiteral("v")).toInt(), 1);
	QCOMPARE(object.value(QStringLiteral("modes")).toObject().value(QStringLiteral("server")).toString(), QStringLiteral("Required"));
	QCOMPARE(object.value(QStringLiteral("modes")).toObject().value(QStringLiteral("client")).toString(), QStringLiteral("Disabled"));
	QCOMPARE(object.value(QStringLiteral("policy")).toObject().value(QStringLiteral("renewBeforeDays")).toInt(), 30);

	// a minimal bundle omits the optional sections
	TrustBundle minimalBundle;
	minimalBundle.caCertificates = {m_caCertificate};
	const auto minimalJson = minimalBundle.toJson();
	QVERIFY(minimalJson.has_value());
	const auto minimalObject = QJsonDocument::fromJson(minimalJson->toUnprotectedByteArray()).object();
	QCOMPARE(minimalObject.keys(), QStringList({QStringLiteral("caCertificates"), QStringLiteral("v")}));
}



void TrustBundleTest::unknownFieldsAreIgnored()
{
	auto object = completeBundle();
	object[QStringLiteral("comment")] = QStringLiteral("created by the wizard");
	object[QStringLiteral("signature")] = QJsonObject{{QStringLiteral("alg"), QStringLiteral("none")}};
	auto enrollment = object.value(QStringLiteral("enrollment")).toObject();
	enrollment[QStringLiteral("extra")] = 42;
	object[QStringLiteral("enrollment")] = enrollment;
	auto modes = object.value(QStringLiteral("modes")).toObject();
	modes[QStringLiteral("demo")] = QStringLiteral("Required");
	object[QStringLiteral("modes")] = modes;

	const auto bundle = TrustBundle::fromJson(toJson(object));
	QVERIFY(bundle.has_value());
	QCOMPARE(bundle->caCertificates.size(), 2);
}



void TrustBundleTest::invalidBundles_data()
{
	QTest::addColumn<QByteArray>("json");

	const auto valid = completeBundle();
	const auto with = [&valid](const QString& key, const QJsonValue& value) {
		auto object = valid;
		object[key] = value;
		return toJson(object);
	};
	const auto without = [&valid](const QString& key) {
		auto object = valid;
		object.remove(key);
		return toJson(object);
	};
	const auto withSection = [&valid](const QString& section, const QString& key, const QJsonValue& value) {
		auto object = valid;
		auto sectionObject = object.value(section).toObject();
		if (value.isUndefined())
		{
			sectionObject.remove(key);
		}
		else
		{
			sectionObject[key] = value;
		}
		object[section] = sectionObject;
		return toJson(object);
	};
	const auto caPem = QString::fromLatin1(m_caCertificate.toPem());

	QTest::newRow("empty") << QByteArray();
	QTest::newRow("not JSON") << QByteArrayLiteral("veyon");
	QTest::newRow("array") << QByteArrayLiteral("[1]");
	QTest::newRow("truncated") << toJson(valid).chopped(1);
	QTest::newRow("trailing garbage") << QByteArray(toJson(valid) + "x");
	QTest::newRow("version missing") << without(QStringLiteral("v"));
	QTest::newRow("version 2") << with(QStringLiteral("v"), 2);
	QTest::newRow("version 0") << with(QStringLiteral("v"), 0);
	QTest::newRow("version 1.5") << with(QStringLiteral("v"), 1.5);
	QTest::newRow("version string") << with(QStringLiteral("v"), QStringLiteral("1"));
	QTest::newRow("version true") << with(QStringLiteral("v"), true);
	QTest::newRow("CA missing") << without(QStringLiteral("caCertificates"));
	QTest::newRow("CA empty") << with(QStringLiteral("caCertificates"), QString());
	QTest::newRow("CA not a string") << with(QStringLiteral("caCertificates"), QJsonArray{caPem});
	QTest::newRow("CA garbage") << with(QStringLiteral("caCertificates"), QStringLiteral("-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n"));
	QTest::newRow("CA no PEM") << with(QStringLiteral("caCertificates"), QStringLiteral("hello"));
	QTest::newRow("CA end-entity") << with(QStringLiteral("caCertificates"), QString::fromLatin1(m_endEntityCertificate.toPem()));
	QTest::newRow("CA with end-entity") << with(QStringLiteral("caCertificates"), QString(caPem + QString::fromLatin1(m_endEntityCertificate.toPem())));
	QTest::newRow("CA with invalid block") << with(QStringLiteral("caCertificates"), QString(caPem + QStringLiteral("-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n")));
	QTest::newRow("CA too many") << with(QStringLiteral("caCertificates"), caPem.repeated(17));
	QTest::newRow("enrollment not an object") << with(QStringLiteral("enrollment"), QStringLiteral("https://pki-host.example.org"));
	QTest::newRow("URL not a string") << withSection(QStringLiteral("enrollment"), QStringLiteral("url"), 11010);
	QTest::newRow("URL http") << withSection(QStringLiteral("enrollment"), QStringLiteral("url"), QStringLiteral("http://pki-host.example.org:11010"));
	QTest::newRow("URL empty") << withSection(QStringLiteral("enrollment"), QStringLiteral("url"), QString());
	QTest::newRow("token without URL") << withSection(QStringLiteral("enrollment"), QStringLiteral("url"), QJsonValue(QJsonValue::Undefined));
	QTest::newRow("token not a string") << withSection(QStringLiteral("enrollment"), QStringLiteral("token"), 1);
	QTest::newRow("token empty") << withSection(QStringLiteral("enrollment"), QStringLiteral("token"), QString());
	QTest::newRow("token invalid") << withSection(QStringLiteral("enrollment"), QStringLiteral("token"), QStringLiteral("a.b"));
	QTest::newRow("token too large") << withSection(QStringLiteral("enrollment"), QStringLiteral("token"),
													QString(QStringLiteral("a.b.") + QStringLiteral("c").repeated(4 * 1024)));
	QTest::newRow("policy not an object") << with(QStringLiteral("policy"), 730);
	QTest::newRow("validity too short") << withSection(QStringLiteral("policy"), QStringLiteral("certValidityDays"), 29);
	QTest::newRow("validity too long") << withSection(QStringLiteral("policy"), QStringLiteral("certValidityDays"), 1096);
	QTest::newRow("validity fraction") << withSection(QStringLiteral("policy"), QStringLiteral("certValidityDays"), 730.5);
	QTest::newRow("validity string") << withSection(QStringLiteral("policy"), QStringLiteral("certValidityDays"), QStringLiteral("730"));
	QTest::newRow("renewal too short") << withSection(QStringLiteral("policy"), QStringLiteral("renewBeforeDays"), 6);
	QTest::newRow("renewal too long") << withSection(QStringLiteral("policy"), QStringLiteral("renewBeforeDays"), 366);
	QTest::newRow("renewal null") << withSection(QStringLiteral("policy"), QStringLiteral("renewBeforeDays"), QJsonValue::Null);
	QTest::newRow("modes not an object") << with(QStringLiteral("modes"), QStringLiteral("Required"));
	QTest::newRow("server mode unknown") << withSection(QStringLiteral("modes"), QStringLiteral("server"), QStringLiteral("Prefer"));
	QTest::newRow("server mode lower case") << withSection(QStringLiteral("modes"), QStringLiteral("server"), QStringLiteral("required"));
	QTest::newRow("server mode number") << withSection(QStringLiteral("modes"), QStringLiteral("server"), 2);
	QTest::newRow("server mode numeric string") << withSection(QStringLiteral("modes"), QStringLiteral("server"), QStringLiteral("2"));
	QTest::newRow("server mode qualified") << withSection(QStringLiteral("modes"), QStringLiteral("server"), QStringLiteral("TlsServerMode::Required"));
	QTest::newRow("client mode unknown") << withSection(QStringLiteral("modes"), QStringLiteral("client"), QStringLiteral("Optional"));
	QTest::newRow("client mode empty") << withSection(QStringLiteral("modes"), QStringLiteral("client"), QString());
}



void TrustBundleTest::invalidBundles()
{
	QFETCH(QByteArray, json);

	QVERIFY(TrustBundle::fromJson(json).has_value() == false);
}



void TrustBundleTest::sizeLimit()
{
	// unknown fields count towards the limit as well
	auto object = completeBundle();
	object[QStringLiteral("padding")] = QString();
	const auto baseSize = toJson(object).size();

	object[QStringLiteral("padding")] = QString(QStringLiteral("x").repeated(int(qsizetype(TrustBundle::MaximumSize) - baseSize)));
	const auto atLimit = toJson(object);
	QVERIFY(atLimit.size() == qsizetype(TrustBundle::MaximumSize));
	QVERIFY(TrustBundle::fromJson(atLimit).has_value());

	const auto aboveLimit = QByteArray(atLimit + " ");
	QVERIFY(TrustBundle::fromJson(aboveLimit).has_value() == false);
}



void TrustBundleTest::caCertificatesPemContainsCertificatesOnly()
{
	// a private key (e.g. the CA key exported by mistake) refuses the whole bundle
	auto object = completeBundle();
	object[QStringLiteral("caCertificates")] = QString(QStringLiteral("-----BEGIN PRIVATE KEY-----\nTUFSS0VS\n-----END PRIVATE KEY-----\n") +
													   QString::fromLatin1(m_caCertificate.toPem()));
	QVERIFY(TrustBundle::fromJson(toJson(object)).has_value() == false);

	// text around the certificates is not imported, TLS/CaCertificate gets re-encoded certificates
	object[QStringLiteral("caCertificates")] = QString(QStringLiteral("Veyon CA Test School\n") +
													   QString::fromLatin1(m_caCertificate.toPem()) +
													   QStringLiteral("\ncomment\n"));
	const auto bundle = TrustBundle::fromJson(toJson(object));
	QVERIFY(bundle.has_value());
	QCOMPARE(bundle->caCertificatesPem(), QString::fromLatin1(m_caCertificate.toPem()));

	const auto caCertificates = TlsClientPolicy::parseCaCertificates(m_caCertificate.toPem());
	QVERIFY(caCertificates.has_value());
	QCOMPARE(caCertificates->size(), 1);
}



void TrustBundleTest::caKeyOutsideKeyPolicyIsRefused()
{
	const auto weakKey = PkiTestCertificates::generateWeakRsaKey();
	QVERIFY(weakKey != nullptr);
	PkiTestCertificates::Specification specification;
	specification.commonName = QStringLiteral("Weak CA");
	specification.isCa = true;
	specification.extendedKeyUsage.clear();
	const auto weakCa = PkiTestCertificates::createWithNativeKeys(weakKey.get(), specification, weakKey.get(), {},
																  QDateTime::currentDateTimeUtc());
	QVERIFY(weakCa.isNull() == false);
	QVERIFY(weakCa.isCa());
	QVERIFY(weakCa.publicKey().isNull());

	QVERIFY(TlsClientPolicy::isAcceptableCaCertificate(weakCa) == false);
	QVERIFY(TlsClientPolicy::parseCaCertificates(QByteArray(m_caCertificate.toPem() + weakCa.toPem())).has_value() == false);

	auto object = completeBundle();
	object[QStringLiteral("caCertificates")] = QString::fromLatin1(weakCa.toPem());
	QVERIFY(TrustBundle::fromJson(toJson(object)).has_value() == false);
}



void TrustBundleTest::createRefusesInvalidBundle()
{
	TrustBundle bundle;
	QVERIFY(bundle.toJson().has_value() == false);

	bundle.caCertificates = {m_endEntityCertificate};
	QVERIFY(bundle.toJson().has_value() == false);

	bundle.caCertificates = {m_caCertificate};
	QVERIFY(bundle.toJson().has_value());

	bundle.enrollmentToken = SecureBuffer::fromByteArray(marker());
	QVERIFY(bundle.toJson().has_value() == false);

	bundle.enrollmentUrl = QStringLiteral("http://pki-host.example.org");
	QVERIFY(bundle.toJson().has_value() == false);

	bundle.enrollmentUrl = QStringLiteral("https://pki-host.example.org");
	QVERIFY(bundle.toJson().has_value());

	bundle.renewBeforeDays = 400;
	QVERIFY(bundle.toJson().has_value() == false);
	bundle.renewBeforeDays.reset();

	bundle.serverMode = VeyonCore::TlsServerMode(42);
	QVERIFY(bundle.toJson().has_value() == false);
}



void TrustBundleTest::enrollmentUrl_data()
{
	QTest::addColumn<QString>("url");
	QTest::addColumn<QString>("normalizedUrl"); // empty: invalid

	const auto host = QStringLiteral("https://pki-host.example.org");
	const auto hostAndPort = QStringLiteral("https://pki-host.example.org:11010");

	QTest::newRow("host and port") << hostAndPort << hostAndPort;
	QTest::newRow("host") << host << host;
	QTest::newRow("root path") << QStringLiteral("https://pki-host.example.org:11010/") << hostAndPort;
	QTest::newRow("IPv4") << QStringLiteral("https://192.0.2.1:11010") << QStringLiteral("https://192.0.2.1:11010");
	QTest::newRow("IPv6") << QStringLiteral("https://[2001:DB8:0::1]:11010") << QStringLiteral("https://[2001:db8::1]:11010");
	QTest::newRow("upper case") << QStringLiteral("HTTPS://PKI-Host.Example.ORG:11010") << hostAndPort;
	QTest::newRow("IDN") << QStringLiteral("https://b\u00fccher.example") << QStringLiteral("https://xn--bcher-kva.example");
	QTest::newRow("http") << QStringLiteral("http://pki-host.example.org:11010") << QString();
	QTest::newRow("no scheme") << QStringLiteral("pki-host.example.org:11010") << QString();
	QTest::newRow("no authority") << QStringLiteral("https:pki-host.example.org") << QString();
	QTest::newRow("file") << QStringLiteral("file:///etc/passwd") << QString();
	QTest::newRow("user info") << QStringLiteral("https://user:password@pki-host.example.org") << QString();
	QTest::newRow("empty user info") << QStringLiteral("https://@pki-host.example.org") << QString();
	QTest::newRow("backslash") << QStringLiteral("https://pki-host.example.org\\@evil.example") << QString();
	QTest::newRow("path") << QStringLiteral("https://pki-host.example.org/api") << QString();
	QTest::newRow("double slash") << QStringLiteral("https://pki-host.example.org//") << QString();
	QTest::newRow("query") << QStringLiteral("https://pki-host.example.org/?a=b") << QString();
	QTest::newRow("empty query") << QStringLiteral("https://pki-host.example.org?") << QString();
	QTest::newRow("fragment") << QStringLiteral("https://pki-host.example.org/#a") << QString();
	QTest::newRow("no host") << QStringLiteral("https://:11010") << QString();
	QTest::newRow("empty port") << QStringLiteral("https://pki-host.example.org:") << QString();
	QTest::newRow("port 0") << QStringLiteral("https://pki-host.example.org:0") << QString();
	QTest::newRow("port with leading zero") << QStringLiteral("https://pki-host.example.org:011010") << QString();
	QTest::newRow("invalid port") << QStringLiteral("https://pki-host.example.org:99999") << QString();
	QTest::newRow("trailing dot") << QStringLiteral("https://pki-host.example.org.") << QString();
	QTest::newRow("IPv4 trailing dot") << QStringLiteral("https://192.0.2.1.") << QString();
	QTest::newRow("hex IPv4") << QStringLiteral("https://0x7f.1") << QString();
	QTest::newRow("short IPv4") << QStringLiteral("https://127.1") << QString();
	QTest::newRow("octal IPv4") << QStringLiteral("https://010.0.0.1") << QString();
	QTest::newRow("IPv6 zone") << QStringLiteral("https://[fe80::1%25eth0]:11010") << QString();
	QTest::newRow("percent-encoded host") << QStringLiteral("https://%70ki-host.example.org") << QString();
	QTest::newRow("space") << QStringLiteral("https://pki host.example.org") << QString();
	QTest::newRow("leading space") << QStringLiteral(" https://pki-host.example.org") << QString();
	QTest::newRow("newline") << QStringLiteral("https://pki-host.example.org\n") << QString();
	QTest::newRow("wildcard host") << QStringLiteral("https://*.example.org") << QString();
	QTest::newRow("too long") << QString(QStringLiteral("https://") + QStringLiteral("a.").repeated(1024) + QStringLiteral("org")) << QString();
	QTest::newRow("empty") << QString() << QString();
}



void TrustBundleTest::enrollmentUrl()
{
	QFETCH(QString, url);
	QFETCH(QString, normalizedUrl);

	QCOMPARE(TrustBundle::normalizedEnrollmentUrl(url), normalizedUrl);
	QCOMPARE(TrustBundle::isValidEnrollmentUrl(url), normalizedUrl.isEmpty() == false);

	if (normalizedUrl.isEmpty() == false)
	{
		// the bundle stores the rebuilt URL
		auto object = completeBundle();
		object[QStringLiteral("enrollment")] = QJsonObject{{QStringLiteral("url"), url}};
		const auto bundle = TrustBundle::fromJson(toJson(object));
		QVERIFY(bundle.has_value());
		QCOMPARE(bundle->enrollmentUrl, normalizedUrl);
	}
}



void TrustBundleTest::enrollmentToken_data()
{
	QTest::addColumn<QByteArray>("token");
	QTest::addColumn<bool>("valid");

	QTest::newRow("JWS") << marker() << true;
	QTest::newRow("base64url characters") << QByteArrayLiteral("aA0-_.bB1-_.cC2-_") << true;
	QTest::newRow("maximum size") << QByteArray(QByteArrayLiteral("a.b.") + QByteArray(4 * 1024 - 4, 'c')) << true;
	QTest::newRow("too large") << QByteArray(QByteArrayLiteral("a.b.") + QByteArray(4 * 1024 - 3, 'c')) << false;
	QTest::newRow("two parts") << QByteArrayLiteral("a.b") << false;
	QTest::newRow("four parts") << QByteArrayLiteral("a.b.c.d") << false;
	QTest::newRow("empty part") << QByteArrayLiteral("a..c") << false;
	QTest::newRow("empty signature") << QByteArrayLiteral("a.b.") << false;
	QTest::newRow("padding") << QByteArrayLiteral("a.b.c=") << false;
	QTest::newRow("base64") << QByteArrayLiteral("a.b+.c/") << false;
	QTest::newRow("space") << QByteArrayLiteral("a.b.c ") << false;
	QTest::newRow("newline") << QByteArrayLiteral("a.b.c\n") << false;
	QTest::newRow("NUL") << QByteArray("a.b\0.c", 6) << false;
	QTest::newRow("empty") << QByteArray() << false;
}



void TrustBundleTest::enrollmentToken()
{
	QFETCH(QByteArray, token);
	QFETCH(bool, valid);

	QCOMPARE(TrustBundle::isValidEnrollmentToken(token), valid);
}



void TrustBundleTest::tokenIsNeverLogged()
{
	// DoD 3: refusals are logged with a reason, never with the token
	const auto valid = completeBundle();
	const QList<QPair<QString, QJsonValue>> invalidFields{
		{QStringLiteral("v"), 2},
		{QStringLiteral("caCertificates"), QStringLiteral("x")},
		{QStringLiteral("modes"), 1},
		{QStringLiteral("policy"), 1},
	};
	for (const auto& field : invalidFields)
	{
		auto object = valid;
		object[field.first] = field.second;
		QVERIFY(TrustBundle::fromJson(toJson(object)).has_value() == false);
	}

	auto object = valid;
	auto enrollment = object.value(QStringLiteral("enrollment")).toObject();
	enrollment[QStringLiteral("token")] = QString::fromLatin1(QByteArray(marker() + " "));
	object[QStringLiteral("enrollment")] = enrollment;
	QVERIFY(TrustBundle::fromJson(toJson(object)).has_value() == false);

	enrollment[QStringLiteral("token")] = QString::fromLatin1(marker());
	enrollment.remove(QStringLiteral("url"));
	object[QStringLiteral("enrollment")] = enrollment;
	QVERIFY(TrustBundle::fromJson(toJson(object)).has_value() == false);

	TrustBundle bundle;
	bundle.caCertificates = {m_caCertificate};
	bundle.enrollmentToken = SecureBuffer::fromByteArray(marker());
	QVERIFY(bundle.toJson().has_value() == false);

	QVERIFY(s_logMessages.isEmpty() == false);
	for (const auto& message : std::as_const(s_logMessages))
	{
		QVERIFY2(message.contains(QStringLiteral("refusing trust bundle")) ||
					 message.contains(QStringLiteral("not creating invalid trust bundle")),
				 qPrintable(message));
		QVERIFY(message.contains(QString::fromLatin1(marker().left(20))) == false);
		QVERIFY(message.contains(QString::fromLatin1(marker().right(16))) == false);
	}
}


QTEST_GUILESS_MAIN(TrustBundleTest)
#include "TrustBundleTest.moc"
