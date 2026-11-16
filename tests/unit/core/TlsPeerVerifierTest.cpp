// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSettings>
#include <QSignalSpy>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTemporaryFile>
#include <QTimeZone>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "PkiTestCertificates.h"
#include "VeyonConfiguration.h"
#include "pki/PkiPublicApi.h"

#include <algorithm>
#include <memory>

using namespace VeyonCrypto;
using PkiTestCertificates::Specification;

Q_DECLARE_METATYPE(TlsError)
Q_DECLARE_METATYPE(TlsPeerVerifier::Status)


// TLS server for one connection with the given certificate and key; sends application data as
// soon as its side of the handshake is complete and records what it receives
class TlsTestServer : public QTcpServer
{
public:
	static constexpr auto ServerData = "server-data";

	TlsTestServer(const QSslCertificate& certificate, const QSslKey& privateKey) :
		m_certificate(certificate),
		m_privateKey(privateKey)
	{
	}

	const QByteArray& receivedData() const
	{
		return m_receivedData;
	}

	bool isEncrypted() const
	{
		return m_encrypted;
	}

protected:
	void incomingConnection(qintptr socketDescriptor) override
	{
		m_socket = std::make_unique<QSslSocket>();
		auto configuration = m_socket->sslConfiguration();
		configuration.setProtocol(QSsl::TlsV1_3OrLater);
		configuration.setLocalCertificate(m_certificate);
		configuration.setPrivateKey(m_privateKey);
		configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
		m_socket->setSslConfiguration(configuration);
		connect(m_socket.get(), &QSslSocket::encrypted, this, [this]() {
			m_encrypted = true;
			m_socket->write(ServerData);
		});
		connect(m_socket.get(), &QSslSocket::readyRead, this, [this]() {
			m_receivedData += m_socket->readAll();
		});
		if (m_socket->setSocketDescriptor(socketDescriptor))
		{
			m_socket->startServerEncryption();
		}
	}

private:
	const QSslCertificate m_certificate;
	const QSslKey m_privateKey;
	std::unique_ptr<QSslSocket> m_socket;
	QByteArray m_receivedData;
	bool m_encrypted = false;
};



class TlsPeerVerifierTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();

	void errorCodes();
	void normalizedHost_data();
	void normalizedHost();
	void dnsNameMatches_data();
	void dnsNameMatches();

	void caSignedDnsTarget_data();
	void caSignedDnsTarget();
	void wrongCa();
	void validityAndClockSkew_data();
	void validityAndClockSkew();
	void ipTarget_data();
	void ipTarget();
	void commonNameIsNeverUsed();
	void wildcardCertificate_data();
	void wildcardCertificate();
	void extendedKeyUsage_data();
	void extendedKeyUsage();
	void unsupportedCriticalExtension();
	void caCertificateAsServerCertificate();
	void revokedSerial();
	void intermediateCa();
	void renewedIssuerIsPreferred();
	void sslErrors_data();
	void sslErrors();
	void noPeerCertificate();
	void noTrustAnchor();
	void demoPolicyNeverSkipsNameCheck();

	void pinMatch();
	void pinOfOtherHostIsRejected();
	void pinByDirectoryNameForIpTarget();
	void pinIgnoresValidityAndRevocation();
	void pinMismatchFallsBackToCa();
	void barePinPerConnection();
	void pinRejectsUnsupportedCriticalExtension();

	void parsePinEntry_data();
	void parsePinEntry();
	void normalizedSerialNumber_data();
	void normalizedSerialNumber();
	void invalidRevokedSerialFailsClosed();
	void emptyConfigurationEntriesAreSkipped();
	void policyFromConfiguration();
	void enumValuesFromConfigurationFile();
	void numericEnumValuesFromConfiguration();
	void settingsRoundTrip();
	void enumValueForms_data();
	void enumValueForms();
	void serverModeFailsClosed();
	void invalidEnumValuesFailClosed();
	void policyDefaultsFromConfiguration();
	void bareConfigurationPinIsIgnoredWithWarning();
	void invalidCaConfigurationFailsClosed();
	void invalidPinConfigurationFailsClosed();
	void configureSocket();

	void handshake_data();
	void handshake();

private:
	Certificate leaf(const Specification& specification) const
	{
		return PkiTestCertificates::create(m_leafKey, specification, m_caKey, m_caCertificate.subjectCommonName(), m_now);
	}

	Certificate leaf(const QString& subjectAltNames) const
	{
		Specification specification;
		specification.subjectAltNames = subjectAltNames;
		specification.serialNumber = ++m_serialNumber;
		return leaf(specification);
	}

	TlsClientPolicy caPolicy() const
	{
		TlsClientPolicy policy;
		policy.caCertificates = {m_caCertificate};
		return policy;
	}

	TlsPeerVerifier::Result verify(const TlsClientPolicy& policy, const QList<Certificate>& chain,
								   const TlsPeerVerifier::Target& target, const QList<QSslError>& sslErrors = {}) const
	{
		QList<QSslCertificate> peerCertificateChain;
		for (const auto& certificate : chain)
		{
			peerCertificateChain.append(certificate.toQSslCertificate());
		}
		return TlsPeerVerifier(policy).verify(peerCertificateChain, sslErrors, target, m_now);
	}

	static TlsPeerVerifier::Target target(const QString& host)
	{
		return TlsPeerVerifier::Target{host};
	}

	static TlsPin pin(const QString& host, const Certificate& certificate)
	{
		return TlsPin{host, certificate.spkiSha256()};
	}

	PrivateKey m_caKey;
	PrivateKey m_otherCaKey;
	PrivateKey m_leafKey;
	PrivateKey m_otherLeafKey;
	Certificate m_caCertificate;
	Certificate m_otherCaCertificate;
	QDateTime m_now;
	bool m_openSslBackendAvailable = false;
	mutable long m_serialNumber = 100;
};



void TlsPeerVerifierTest::initTestCase()
{
	// whole seconds: X.509 times have no sub-second precision
	m_now = QDateTime::fromSecsSinceEpoch(QDateTime::currentSecsSinceEpoch(), QTimeZone::utc());

	m_caKey = PkiTestCertificates::generateKey();
	m_otherCaKey = PkiTestCertificates::generateKey();
	m_leafKey = PkiTestCertificates::generateKey();
	m_otherLeafKey = PkiTestCertificates::generateKey();
	QVERIFY(m_caKey.isNull() == false && m_otherCaKey.isNull() == false &&
			m_leafKey.isNull() == false && m_otherLeafKey.isNull() == false);

	m_caCertificate = PkiTestCertificates::createCa(m_caKey, QStringLiteral("Veyon CA Test School"), m_now);
	m_otherCaCertificate = PkiTestCertificates::createCa(m_otherCaKey, QStringLiteral("Veyon CA Test School"), m_now);
	QVERIFY(m_caCertificate.isNull() == false && m_otherCaCertificate.isNull() == false);

	// TLS verification requires Qt's OpenSSL backend (05-tls.md, spike S8)
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
	m_openSslBackendAvailable = QSslSocket::setActiveBackend(QStringLiteral("openssl"));
#else
	m_openSslBackendAvailable = QSslSocket::supportsSsl();
#endif
}



void TlsPeerVerifierTest::errorCodes()
{
	QCOMPARE(tlsErrorCode(TlsError::None), QString());
	QCOMPARE(tlsErrorCode(TlsError::Expired), QStringLiteral("expired"));
	QCOMPARE(tlsErrorCode(TlsError::NotYetValid), QStringLiteral("not-yet-valid"));
	QCOMPARE(tlsErrorCode(TlsError::ClockSkew), QStringLiteral("clock-skew"));
	QCOMPARE(tlsErrorCode(TlsError::NameMismatch), QStringLiteral("name-mismatch"));
	QCOMPARE(tlsErrorCode(TlsError::UntrustedCa), QStringLiteral("untrusted-ca"));
	QCOMPARE(tlsErrorCode(TlsError::Revoked), QStringLiteral("revoked"));
	QCOMPARE(tlsErrorCode(TlsError::PinMismatch), QStringLiteral("pin-mismatch"));
	QCOMPARE(tlsErrorCode(TlsError::HandshakeFailed), QStringLiteral("handshake-failed"));
	QCOMPARE(tlsErrorCode(TlsError::TlsRequiredPeerPlain), QStringLiteral("tls-required-peer-plain"));
	QCOMPARE(tlsErrorCode(TlsError::DowngradeSuspected), QStringLiteral("downgrade-suspected"));
	QCOMPARE(tlsErrorCode(TlsError::CredentialsRefused), QStringLiteral("credentials-refused"));

	QCOMPARE(tlsErrorMessage(TlsError::ClockSkew, 7), QStringLiteral("system clock differs by 7 min from peer"));
}



void TlsPeerVerifierTest::normalizedHost_data()
{
	QTest::addColumn<QString>("host");
	QTest::addColumn<QString>("normalizedHost");

	QTest::newRow("dns") << QStringLiteral("pc01.school.example") << QStringLiteral("pc01.school.example");
	QTest::newRow("upper case") << QStringLiteral("PC01.School.Example") << QStringLiteral("pc01.school.example");
	QTest::newRow("trailing dot") << QStringLiteral("pc01.school.example.") << QStringLiteral("pc01.school.example");
	QTest::newRow("two trailing dots") << QStringLiteral("pc01.school.example..") << QString();
	QTest::newRow("single label") << QStringLiteral("PC01") << QStringLiteral("pc01");
	QTest::newRow("idn") << QStringLiteral("rechner.schüle.example") << QStringLiteral("rechner.xn--schle-mva.example");
	QTest::newRow("ipv4") << QStringLiteral("192.0.2.10") << QStringLiteral("192.0.2.10");
	QTest::newRow("ipv6") << QStringLiteral("2001:DB8:0::10") << QStringLiteral("2001:db8::10");
	QTest::newRow("bracketed ipv6") << QStringLiteral("[2001:db8::10]") << QStringLiteral("2001:db8::10");
	QTest::newRow("ipv6 scope") << QStringLiteral("fe80::1%eth0") << QStringLiteral("fe80::1");
	QTest::newRow("bracketed ipv4") << QStringLiteral("[192.0.2.10]") << QString();
	QTest::newRow("empty") << QString() << QString();
	QTest::newRow("dot") << QStringLiteral(".") << QString();
	QTest::newRow("empty label") << QStringLiteral("pc01..example") << QString();
	QTest::newRow("wildcard") << QStringLiteral("*.school.example") << QString();
	QTest::newRow("space") << QStringLiteral("pc01 .example") << QString();
	QTest::newRow("slash") << QStringLiteral("pc01/x") << QString();
	QTest::newRow("at") << QStringLiteral("user@pc01") << QString();
	QTest::newRow("nul") << QStringLiteral("pc01\0.example") << QString();
	QTest::newRow("long label") << QString(QStringLiteral("a").repeated(64) + QStringLiteral(".example")) << QString();
	QTest::newRow("IPv4 with trailing dot") << QStringLiteral("192.0.2.10.") << QString();
	QTest::newRow("full-width digits") << QStringLiteral("\uff11\uff19\uff12.0.2.10") << QString();
	QTest::newRow("full-width digits, trailing dot") << QStringLiteral("\uff11\uff19\uff12.0.2.10.") << QString();
	QTest::newRow("numeric labels") << QStringLiteral("1.2.3.4.5") << QStringLiteral("1.2.3.4.5");
	QTest::newRow("long name") << QString(QStringLiteral("abcdefghi.").repeated(26)) << QString();
}



void TlsPeerVerifierTest::normalizedHost()
{
	QFETCH(QString, host);
	QFETCH(QString, normalizedHost);

	QCOMPARE(TlsPeerVerifier::normalizedHost(host), normalizedHost);
}



void TlsPeerVerifierTest::dnsNameMatches_data()
{
	QTest::addColumn<QString>("subjectAltName");
	QTest::addColumn<QString>("name");
	QTest::addColumn<bool>("matches");

	QTest::newRow("exact") << QStringLiteral("pc01.school.example") << QStringLiteral("pc01.school.example") << true;
	QTest::newRow("case") << QStringLiteral("PC01.School.Example") << QStringLiteral("pc01.school.example") << true;
	QTest::newRow("trailing dot") << QStringLiteral("pc01.school.example.") << QStringLiteral("pc01.school.example") << true;
	QTest::newRow("other") << QStringLiteral("pc02.school.example") << QStringLiteral("pc01.school.example") << false;
	QTest::newRow("suffix") << QStringLiteral("school.example") << QStringLiteral("pc01.school.example") << false;
	QTest::newRow("wildcard") << QStringLiteral("*.school.example") << QStringLiteral("pc01.school.example") << true;
	QTest::newRow("wildcard case") << QStringLiteral("*.SCHOOL.example") << QStringLiteral("pc01.school.example") << true;
	QTest::newRow("wildcard two labels") << QStringLiteral("*.school.example") << QStringLiteral("a.pc01.school.example") << false;
	QTest::newRow("wildcard no label") << QStringLiteral("*.school.example") << QStringLiteral("school.example") << false;
	QTest::newRow("wildcard empty label") << QStringLiteral("*.school.example") << QStringLiteral(".school.example") << false;
	QTest::newRow("wildcard tld") << QStringLiteral("*.example") << QStringLiteral("school.example") << false;
	QTest::newRow("wildcard only") << QStringLiteral("*") << QStringLiteral("example") << false;
	QTest::newRow("partial wildcard") << QStringLiteral("pc*.school.example") << QStringLiteral("pc01.school.example") << false;
	QTest::newRow("partial wildcard suffix") << QStringLiteral("*01.school.example") << QStringLiteral("pc01.school.example") << false;
	QTest::newRow("inner wildcard") << QStringLiteral("pc01.*.example") << QStringLiteral("pc01.school.example") << false;
	QTest::newRow("double wildcard") << QStringLiteral("*.*.example") << QStringLiteral("pc01.school.example") << false;
	QTest::newRow("wildcard empty suffix label") << QStringLiteral("*.school..example") << QStringLiteral("pc01.school..example") << false;
	QTest::newRow("empty") << QString() << QStringLiteral("pc01.school.example") << false;
}



void TlsPeerVerifierTest::dnsNameMatches()
{
	QFETCH(QString, subjectAltName);
	QFETCH(QString, name);
	QFETCH(bool, matches);

	QCOMPARE(TlsPeerVerifier::dnsNameMatches(subjectAltName, name), matches);
}



void TlsPeerVerifierTest::caSignedDnsTarget_data()
{
	QTest::addColumn<QString>("host");
	QTest::addColumn<TlsPeerVerifier::Status>("status");
	QTest::addColumn<TlsError>("error");

	QTest::newRow("exact") << QStringLiteral("pc01.school.example") << TlsPeerVerifier::Status::Verified << TlsError::None;
	QTest::newRow("case and trailing dot") << QStringLiteral("PC01.School.Example.") << TlsPeerVerifier::Status::Verified << TlsError::None;
	QTest::newRow("single label SAN") << QStringLiteral("pc01") << TlsPeerVerifier::Status::Verified << TlsError::None;
	QTest::newRow("wrong DNS SAN") << QStringLiteral("pc02.school.example") << TlsPeerVerifier::Status::Rejected << TlsError::NameMismatch;
	QTest::newRow("IP target without IP SAN") << QStringLiteral("192.0.2.10") << TlsPeerVerifier::Status::Rejected << TlsError::NameMismatch;
	QTest::newRow("invalid target") << QStringLiteral("pc01 school") << TlsPeerVerifier::Status::Rejected << TlsError::NameMismatch;
}



void TlsPeerVerifierTest::caSignedDnsTarget()
{
	QFETCH(QString, host);
	QFETCH(TlsPeerVerifier::Status, status);
	QFETCH(TlsError, error);

	const auto certificate = leaf(QStringLiteral("DNS:pc01.school.example,DNS:pc01"));
	QVERIFY(certificate.isNull() == false);

	const auto result = verify(caPolicy(), {certificate}, target(host));
	QVERIFY2(result.status == status, qPrintable(result.reason));
	QCOMPARE(result.error, error);
}



void TlsPeerVerifierTest::wrongCa()
{
	Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	// same issuer name, different CA key
	const auto foreignCertificate = PkiTestCertificates::create(m_leafKey, specification, m_otherCaKey,
																m_otherCaCertificate.subjectCommonName(), m_now);
	QVERIFY(foreignCertificate.isNull() == false);

	auto result = verify(caPolicy(), {foreignCertificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::UntrustedCa);

	// also when the peer sends its own CA certificate along
	result = verify(caPolicy(), {foreignCertificate, m_otherCaCertificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.error, TlsError::UntrustedCa);

	// self-signed certificate
	specification.serialNumber = 2;
	const auto selfSigned = PkiTestCertificates::create(m_leafKey, specification, m_leafKey, {}, m_now);
	result = verify(caPolicy(), {selfSigned}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.error, TlsError::UntrustedCa);
}



void TlsPeerVerifierTest::validityAndClockSkew_data()
{
	QTest::addColumn<qint64>("notBeforeOffset");
	QTest::addColumn<qint64>("notAfterOffset");
	QTest::addColumn<TlsError>("error");
	QTest::addColumn<int>("clockSkewMinutes");

	constexpr qint64 Minute = 60;
	constexpr qint64 Day = 24 * 60 * Minute;

	QTest::newRow("valid") << -Minute << Day << TlsError::None << 0;
	QTest::newRow("not yet valid +4 min") << 4 * Minute << Day << TlsError::None << 0;
	QTest::newRow("not yet valid +5 min") << 5 * Minute << Day << TlsError::None << 0;
	QTest::newRow("not yet valid +6 min") << 6 * Minute << Day << TlsError::ClockSkew << 6;
	QTest::newRow("expired -4 min") << -Day << -4 * Minute << TlsError::None << 0;
	QTest::newRow("expired -6 min") << -Day << -6 * Minute << TlsError::ClockSkew << 6;
	QTest::newRow("expired -60 min") << -Day << -60 * Minute << TlsError::ClockSkew << 60;
	QTest::newRow("expired -61 min") << -Day << -61 * Minute << TlsError::Expired << 0;
	QTest::newRow("expired -90 min") << -Day << -90 * Minute << TlsError::Expired << 0;
	QTest::newRow("not yet valid +60 min") << 60 * Minute << Day << TlsError::ClockSkew << 60;
	QTest::newRow("not yet valid +61 min") << 61 * Minute << Day << TlsError::NotYetValid << 0;
	QTest::newRow("not yet valid") << 2 * Day << 3 * Day << TlsError::NotYetValid << 0;
	QTest::newRow("expired") << -3 * Day << -2 * Day << TlsError::Expired << 0;
}



void TlsPeerVerifierTest::validityAndClockSkew()
{
	QFETCH(qint64, notBeforeOffset);
	QFETCH(qint64, notAfterOffset);
	QFETCH(TlsError, error);
	QFETCH(int, clockSkewMinutes);

	Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	specification.notBeforeOffsetSeconds = notBeforeOffset;
	specification.notAfterOffsetSeconds = notAfterOffset;
	const auto certificate = leaf(specification);
	QVERIFY(certificate.isNull() == false);

	const auto result = verify(caPolicy(), {certificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.error, error);
	QCOMPARE(result.status, error == TlsError::None ? TlsPeerVerifier::Status::Verified : TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.clockSkewMinutes, clockSkewMinutes);
	if (error == TlsError::ClockSkew)
	{
		QCOMPARE(result.message(), QStringLiteral("system clock differs by %1 min from peer").arg(clockSkewMinutes));
	}
}



void TlsPeerVerifierTest::ipTarget_data()
{
	QTest::addColumn<QString>("subjectAltNames");
	QTest::addColumn<VeyonCore::TlsIpTargetPolicy>("ipTargetPolicy");
	QTest::addColumn<QString>("host");
	QTest::addColumn<QString>("directoryHostName");
	QTest::addColumn<QString>("reverseDnsHostName");
	QTest::addColumn<TlsError>("error");

	const auto requireIpSan = VeyonCore::TlsIpTargetPolicy::RequireIpSan;
	const auto acceptHostname = VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithHostname;
	const auto acceptReverseDns = VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithReverseDnsHostname;
	const auto dnsOnly = QStringLiteral("DNS:pc01.school.example");
	const auto withIp = QStringLiteral("DNS:pc01.school.example,IP:192.0.2.10,IP:2001:db8::10");

	QTest::newRow("IPv4 SAN") << withIp << requireIpSan << QStringLiteral("192.0.2.10") << QString() << QString() << TlsError::None;
	QTest::newRow("IPv4 with trailing dot") << QStringLiteral("DNS:192.0.2.10,IP:192.0.2.10") << requireIpSan << QStringLiteral("192.0.2.10.")
											<< QString() << QString() << TlsError::NameMismatch;
	QTest::newRow("wrong IPv4 SAN") << withIp << requireIpSan << QStringLiteral("192.0.2.11") << QString() << QString() << TlsError::NameMismatch;
	QTest::newRow("IPv6 SAN") << withIp << requireIpSan << QStringLiteral("2001:DB8::10") << QString() << QString() << TlsError::None;
	QTest::newRow("bracketed IPv6 SAN") << withIp << requireIpSan << QStringLiteral("[2001:db8::10]") << QString() << QString() << TlsError::None;
	QTest::newRow("wrong IPv6 SAN") << withIp << requireIpSan << QStringLiteral("2001:db8::11") << QString() << QString() << TlsError::NameMismatch;
	QTest::newRow("IPv4-mapped IPv6") << withIp << requireIpSan << QStringLiteral("::ffff:192.0.2.10") << QString() << QString() << TlsError::NameMismatch;
	QTest::newRow("wrong IP SAN but directory name") << withIp << acceptHostname << QStringLiteral("192.0.2.11")
													 << QStringLiteral("pc01.school.example") << QString() << TlsError::None;
	QTest::newRow("directory name, RequireIpSan") << dnsOnly << requireIpSan << QStringLiteral("192.0.2.50")
												  << QStringLiteral("pc01.school.example") << QString() << TlsError::NameMismatch;
	QTest::newRow("directory name") << dnsOnly << acceptHostname << QStringLiteral("192.0.2.50")
									<< QStringLiteral("PC01.School.Example.") << QString() << TlsError::None;
	QTest::newRow("other directory name") << dnsOnly << acceptHostname << QStringLiteral("192.0.2.50")
										  << QStringLiteral("pc02.school.example") << QString() << TlsError::NameMismatch;
	QTest::newRow("no directory name") << dnsOnly << acceptHostname << QStringLiteral("192.0.2.50")
									   << QString() << QString() << TlsError::NameMismatch;
	QTest::newRow("directory name is IP literal") << QStringLiteral("DNS:192.0.2.50") << acceptHostname << QStringLiteral("192.0.2.50")
												  << QStringLiteral("192.0.2.50") << QString() << TlsError::NameMismatch;
	QTest::newRow("directory name against wildcard SAN") << QStringLiteral("DNS:*.school.example") << acceptHostname << QStringLiteral("192.0.2.50")
														 << QStringLiteral("pc01.school.example") << QString() << TlsError::NameMismatch;
	QTest::newRow("reverse name without reverse policy") << dnsOnly << acceptHostname << QStringLiteral("192.0.2.50")
														 << QString() << QStringLiteral("pc01.school.example") << TlsError::NameMismatch;
	QTest::newRow("reverse name") << dnsOnly << acceptReverseDns << QStringLiteral("192.0.2.50")
								  << QString() << QStringLiteral("pc01.school.example") << TlsError::None;
	QTest::newRow("directory name with reverse policy") << dnsOnly << acceptReverseDns << QStringLiteral("192.0.2.50")
														<< QStringLiteral("pc01.school.example") << QString() << TlsError::None;
	QTest::newRow("other reverse name") << dnsOnly << acceptReverseDns << QStringLiteral("192.0.2.50")
										<< QString() << QStringLiteral("pc02.school.example") << TlsError::NameMismatch;
	QTest::newRow("directory name for DNS target is ignored") << dnsOnly << acceptHostname << QStringLiteral("pc02.school.example")
															  << QStringLiteral("pc01.school.example") << QString() << TlsError::NameMismatch;
}



void TlsPeerVerifierTest::ipTarget()
{
	QFETCH(QString, subjectAltNames);
	QFETCH(VeyonCore::TlsIpTargetPolicy, ipTargetPolicy);
	QFETCH(QString, host);
	QFETCH(QString, directoryHostName);
	QFETCH(QString, reverseDnsHostName);
	QFETCH(TlsError, error);

	const auto certificate = leaf(subjectAltNames);
	QVERIFY(certificate.isNull() == false);

	auto policy = caPolicy();
	policy.ipTargetPolicy = ipTargetPolicy;
	const auto result = verify(policy, {certificate}, {host, directoryHostName, reverseDnsHostName});
	QVERIFY2(result.error == error, qPrintable(result.reason));
	QCOMPARE(result.isVerified(), error == TlsError::None);
}



void TlsPeerVerifierTest::commonNameIsNeverUsed()
{
	Specification specification;
	specification.commonName = QStringLiteral("pc01.school.example");
	const auto withoutSan = leaf(specification);
	QVERIFY(withoutSan.isNull() == false);
	QCOMPARE(verify(caPolicy(), {withoutSan}, target(QStringLiteral("pc01.school.example"))).error, TlsError::NameMismatch);

	specification.commonName = QStringLiteral("192.0.2.10");
	specification.serialNumber = 2;
	const auto ipCommonName = leaf(specification);
	QCOMPARE(verify(caPolicy(), {ipCommonName}, target(QStringLiteral("192.0.2.10"))).error, TlsError::NameMismatch);
}



void TlsPeerVerifierTest::wildcardCertificate_data()
{
	QTest::addColumn<QString>("host");
	QTest::addColumn<TlsError>("error");

	QTest::newRow("one label") << QStringLiteral("pc01.school.example") << TlsError::None;
	QTest::newRow("two labels") << QStringLiteral("a.pc01.school.example") << TlsError::NameMismatch;
	QTest::newRow("base name") << QStringLiteral("school.example") << TlsError::NameMismatch;
	QTest::newRow("IP target") << QStringLiteral("192.0.2.10") << TlsError::NameMismatch;
}



void TlsPeerVerifierTest::wildcardCertificate()
{
	QFETCH(QString, host);
	QFETCH(TlsError, error);

	const auto certificate = leaf(QStringLiteral("DNS:*.school.example"));
	QVERIFY(certificate.isNull() == false);
	QCOMPARE(verify(caPolicy(), {certificate}, target(host)).error, error);
}



void TlsPeerVerifierTest::extendedKeyUsage_data()
{
	QTest::addColumn<QString>("extendedKeyUsage");
	QTest::addColumn<TlsError>("error");

	QTest::newRow("serverAuth") << QStringLiteral("serverAuth") << TlsError::None;
	QTest::newRow("serverAuth and clientAuth") << QStringLiteral("serverAuth,clientAuth") << TlsError::None;
	QTest::newRow("clientAuth") << QStringLiteral("clientAuth") << TlsError::UntrustedCa;
	QTest::newRow("no extension") << QString() << TlsError::UntrustedCa;
}



void TlsPeerVerifierTest::extendedKeyUsage()
{
	QFETCH(QString, extendedKeyUsage);
	QFETCH(TlsError, error);

	Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	specification.extendedKeyUsage = extendedKeyUsage;
	const auto certificate = leaf(specification);
	QVERIFY(certificate.isNull() == false);
	QCOMPARE(verify(caPolicy(), {certificate}, target(QStringLiteral("pc01.school.example"))).error, error);
}



void TlsPeerVerifierTest::unsupportedCriticalExtension()
{
	Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	specification.unsupportedCriticalExtension = true;
	const auto certificate = leaf(specification);
	QVERIFY(certificate.isNull() == false);
	QVERIFY(certificate.hasUnsupportedCriticalExtension());

	QCOMPARE(verify(caPolicy(), {certificate}, target(QStringLiteral("pc01.school.example"))).error, TlsError::UntrustedCa);
}



void TlsPeerVerifierTest::caCertificateAsServerCertificate()
{
	// CA-signed CA certificate with serverAuth and matching SAN
	Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	specification.isCa = true;
	specification.extendedKeyUsage = QStringLiteral("serverAuth");
	specification.serialNumber = 60;
	const auto caLeaf = leaf(specification);
	QVERIFY(caLeaf.isNull() == false && caLeaf.isCa());

	TlsClientPolicy policy;
	policy.caCertificates = {PkiTestCertificates::createCa(m_caKey, m_caCertificate.subjectCommonName(), m_now, -1)};
	QCOMPARE(verify(policy, {caLeaf}, target(QStringLiteral("pc01.school.example"))).error, TlsError::UntrustedCa);

	// the configured CA itself presented as server certificate
	QCOMPARE(verify(caPolicy(), {m_caCertificate}, target(QStringLiteral("pc01.school.example"))).error, TlsError::UntrustedCa);

	// a pinned CA certificate never matches
	TlsClientPolicy pinPolicy;
	pinPolicy.pins = {pin(QStringLiteral("pc01.school.example"), caLeaf)};
	const auto result = verify(pinPolicy, {caLeaf}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.error, TlsError::PinMismatch);
	QVERIFY2(result.reason.contains(QStringLiteral("CA certificate")), qPrintable(result.reason));
}



void TlsPeerVerifierTest::revokedSerial()
{
	Specification specification;
	specification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	specification.serialNumber = 0x0a1b2c;
	const auto certificate = leaf(specification);
	QVERIFY(certificate.isNull() == false);
	QCOMPARE(certificate.serialNumberHex(), QStringLiteral("0a1b2c"));

	auto policy = caPolicy();
	policy.revokedSerials = QStringList{QStringLiteral("123456")};
	QVERIFY(verify(policy, {certificate}, target(QStringLiteral("pc01.school.example"))).isVerified());

	policy.revokedSerials.append(TlsClientPolicy::normalizedSerialNumber(QStringLiteral("000A1B2C")));
	const auto result = verify(policy, {certificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::Revoked);
}



void TlsPeerVerifierTest::intermediateCa()
{
	// root (no path length constraint) -> intermediate CA -> leaf
	const auto rootKey = PkiTestCertificates::generateKey();
	const auto intermediateKey = PkiTestCertificates::generateKey();
	const auto root = PkiTestCertificates::createCa(rootKey, QStringLiteral("Organisation Root CA"), m_now, -1);

	Specification intermediateSpecification;
	intermediateSpecification.commonName = QStringLiteral("Organisation Issuing CA");
	intermediateSpecification.isCa = true;
	intermediateSpecification.pathLength = 0;
	intermediateSpecification.extendedKeyUsage.clear();
	intermediateSpecification.serialNumber = 10;
	const auto intermediate = PkiTestCertificates::create(intermediateKey, intermediateSpecification, rootKey,
														  root.subjectCommonName(), m_now);

	Specification leafSpecification;
	leafSpecification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	leafSpecification.serialNumber = 11;
	const auto certificate = PkiTestCertificates::create(m_leafKey, leafSpecification, intermediateKey,
														 intermediate.subjectCommonName(), m_now);
	QVERIFY(root.isNull() == false && intermediate.isNull() == false && certificate.isNull() == false);

	TlsClientPolicy policy;
	policy.caCertificates = {root};
	const auto host = target(QStringLiteral("pc01.school.example"));

	QVERIFY(verify(policy, {certificate, intermediate}, host).isVerified());
	// intermediate missing
	QCOMPARE(verify(policy, {certificate}, host).error, TlsError::UntrustedCa);
	// revoked intermediate
	policy.revokedSerials = QStringList{QStringLiteral("0a")};
	QCOMPARE(verify(policy, {certificate, intermediate}, host).error, TlsError::Revoked);
	policy.revokedSerials.clear();

	// path length constraint of the trust anchor
	const auto constrainedRoot = PkiTestCertificates::createCa(rootKey, QStringLiteral("Organisation Root CA"), m_now, 0);
	policy.caCertificates = {constrainedRoot};
	QCOMPARE(verify(policy, {certificate, intermediate}, host).error, TlsError::UntrustedCa);

	// an end-entity certificate must not act as issuer
	Specification fakeIssuerSpecification;
	fakeIssuerSpecification.commonName = QStringLiteral("Organisation Issuing CA");
	fakeIssuerSpecification.serialNumber = 12;
	const auto fakeIssuer = PkiTestCertificates::create(intermediateKey, fakeIssuerSpecification, rootKey,
														root.subjectCommonName(), m_now);
	policy.caCertificates = {root};
	QCOMPARE(verify(policy, {certificate, fakeIssuer}, host).error, TlsError::UntrustedCa);
}



void TlsPeerVerifierTest::renewedIssuerIsPreferred()
{
	// renewed CA certificate with the same subject and key, the expired one listed first
	Specification expiredCaSpecification;
	expiredCaSpecification.commonName = m_caCertificate.subjectCommonName();
	expiredCaSpecification.isCa = true;
	expiredCaSpecification.pathLength = 0;
	expiredCaSpecification.extendedKeyUsage.clear();
	expiredCaSpecification.notBeforeOffsetSeconds = -400LL * 24 * 60 * 60;
	expiredCaSpecification.notAfterOffsetSeconds = -2LL * 24 * 60 * 60;
	expiredCaSpecification.serialNumber = 50;
	const auto expiredCa = PkiTestCertificates::create(m_caKey, expiredCaSpecification, m_caKey, {}, m_now);
	QVERIFY(expiredCa.isNull() == false);

	const auto certificate = leaf(QStringLiteral("DNS:pc01.school.example"));
	TlsClientPolicy policy;
	policy.caCertificates = {expiredCa, m_caCertificate};
	const auto result = verify(policy, {certificate}, target(QStringLiteral("pc01.school.example")));
	QVERIFY2(result.isVerified(), qPrintable(result.reason));

	// only the expired one: the expiry is reported
	policy.caCertificates = {expiredCa};
	QCOMPARE(verify(policy, {certificate}, target(QStringLiteral("pc01.school.example"))).error, TlsError::Expired);

	// same for intermediates sent by the peer
	const auto rootKey = PkiTestCertificates::generateKey();
	const auto intermediateKey = PkiTestCertificates::generateKey();
	const auto root = PkiTestCertificates::createCa(rootKey, QStringLiteral("Organisation Root CA"), m_now, -1);
	Specification intermediateSpecification = expiredCaSpecification;
	intermediateSpecification.commonName = QStringLiteral("Organisation Issuing CA");
	intermediateSpecification.serialNumber = 51;
	const auto expiredIntermediate = PkiTestCertificates::create(intermediateKey, intermediateSpecification, rootKey,
																 root.subjectCommonName(), m_now);
	intermediateSpecification.notBeforeOffsetSeconds = -24 * 60 * 60;
	intermediateSpecification.notAfterOffsetSeconds = 400LL * 24 * 60 * 60;
	intermediateSpecification.serialNumber = 52;
	const auto renewedIntermediate = PkiTestCertificates::create(intermediateKey, intermediateSpecification, rootKey,
																 root.subjectCommonName(), m_now);
	Specification leafSpecification;
	leafSpecification.subjectAltNames = QStringLiteral("DNS:pc01.school.example");
	leafSpecification.serialNumber = 53;
	const auto organisationLeaf = PkiTestCertificates::create(m_leafKey, leafSpecification, intermediateKey,
															  renewedIntermediate.subjectCommonName(), m_now);
	policy.caCertificates = {root};
	QVERIFY(verify(policy, {organisationLeaf, expiredIntermediate, renewedIntermediate}, target(QStringLiteral("pc01.school.example")))
				.isVerified());
}



void TlsPeerVerifierTest::sslErrors_data()
{
	QTest::addColumn<int>("sslError");
	QTest::addColumn<TlsError>("error");

	QTest::newRow("host name mismatch") << int(QSslError::HostNameMismatch) << TlsError::None;
	QTest::newRow("expired (re-evaluated)") << int(QSslError::CertificateExpired) << TlsError::None;
	QTest::newRow("not yet valid (re-evaluated)") << int(QSslError::CertificateNotYetValid) << TlsError::None;
	QTest::newRow("self-signed in chain") << int(QSslError::SelfSignedCertificateInChain) << TlsError::UntrustedCa;
	QTest::newRow("unable to get issuer") << int(QSslError::UnableToGetLocalIssuerCertificate) << TlsError::UntrustedCa;
	QTest::newRow("signature failure") << int(QSslError::CertificateSignatureFailed) << TlsError::UntrustedCa;
	QTest::newRow("invalid purpose") << int(QSslError::InvalidPurpose) << TlsError::UntrustedCa;
	QTest::newRow("revoked") << int(QSslError::CertificateRevoked) << TlsError::Revoked;
	QTest::newRow("unspecified") << int(QSslError::UnspecifiedError) << TlsError::UntrustedCa;
}



void TlsPeerVerifierTest::sslErrors()
{
	QFETCH(int, sslError);
	QFETCH(TlsError, error);

	// a certificate that passes the verifier's own checks is still rejected when Qt/OpenSSL
	// reported a chain error
	const auto certificate = leaf(QStringLiteral("DNS:pc01.school.example"));
	const auto result = verify(caPolicy(), {certificate}, target(QStringLiteral("pc01.school.example")),
							   {QSslError(QSslError::SslError(sslError), certificate.toQSslCertificate())});
	QCOMPARE(result.error, error);
	QCOMPARE(result.isVerified(), error == TlsError::None);
}



void TlsPeerVerifierTest::noPeerCertificate()
{
	const auto result = TlsPeerVerifier(caPolicy()).verify({}, {}, target(QStringLiteral("pc01.school.example")), m_now);
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::HandshakeFailed);

	// also without trust anchors: nothing to encrypt against
	QCOMPARE(TlsPeerVerifier(TlsClientPolicy{}).verify({QSslCertificate{}}, {}, target(QStringLiteral("pc01")), m_now).error,
			 TlsError::HandshakeFailed);

	// unparsable peer certificate
	const QSslCertificate garbage(QByteArrayLiteral("garbage"), QSsl::Der);
	QCOMPARE(TlsPeerVerifier(caPolicy()).verify({garbage}, {}, target(QStringLiteral("pc01")), m_now).status,
			 TlsPeerVerifier::Status::Rejected);
}



void TlsPeerVerifierTest::noTrustAnchor()
{
	const auto certificate = leaf(QStringLiteral("DNS:pc99.school.example"));
	const auto result = verify(TlsClientPolicy{}, {certificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Unverified);
	QCOMPARE(result.error, TlsError::None);
	QVERIFY(TlsClientPolicy{}.hasTrustAnchor() == false);
}



void TlsPeerVerifierTest::demoPolicyNeverSkipsNameCheck()
{
	// a per-connection bare pin together with CA certificates (e.g. a demo policy built from the
	// configured policy) must not accept a CA-signed certificate of another host
	const auto otherHost = leaf(QStringLiteral("DNS:teacher.school.example"));
	auto policy = caPolicy();
	policy.pins = {TlsPin{{}, QByteArray(32, 'x')}};

	const auto result = verify(policy, {otherHost}, target(QStringLiteral("demo-host.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::NameMismatch);
}



void TlsPeerVerifierTest::pinMatch()
{
	// self-signed, no SAN: pins do not depend on chain or names
	const auto certificate = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);
	QVERIFY(certificate.isNull() == false);

	TlsClientPolicy policy;
	policy.pins = {pin(QStringLiteral("pc01.school.example"), certificate)};
	QVERIFY(policy.hasTrustAnchor());

	auto result = verify(policy, {certificate}, target(QStringLiteral("PC01.school.example.")));
	QVERIFY2(result.isVerified(), qPrintable(result.reason));

	// wrong key for this host
	const auto otherCertificate = PkiTestCertificates::create(m_otherLeafKey, Specification{}, m_otherLeafKey, {}, m_now);
	result = verify(policy, {otherCertificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::PinMismatch);

	// IP-bound pin
	policy.pins = {pin(QStringLiteral("2001:db8::10"), certificate)};
	QVERIFY(verify(policy, {certificate}, target(QStringLiteral("[2001:DB8::10]"))).isVerified());
}



void TlsPeerVerifierTest::pinOfOtherHostIsRejected()
{
	// T-NET-14: the pinned key of computer B never authenticates target A
	const auto certificateOfB = PkiTestCertificates::create(m_otherLeafKey, Specification{}, m_otherLeafKey, {}, m_now);
	const auto certificateOfA = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);

	TlsClientPolicy policy;
	policy.pins = {pin(QStringLiteral("pc01.school.example"), certificateOfA),
				   pin(QStringLiteral("pc02.school.example"), certificateOfB)};

	auto result = verify(policy, {certificateOfB}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::PinMismatch);

	// target without any pin while other pins exist
	result = verify(policy, {certificateOfB}, target(QStringLiteral("pc03.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::PinMismatch);

	// B's pin does not apply to A's IP address via B's name either
	result = verify(policy, {certificateOfB}, {QStringLiteral("192.0.2.10"), QStringLiteral("pc01.school.example")});
	QCOMPARE(result.error, TlsError::PinMismatch);
}



void TlsPeerVerifierTest::pinByDirectoryNameForIpTarget()
{
	const auto certificate = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);

	TlsClientPolicy policy;
	policy.pins = {pin(QStringLiteral("pc01.school.example"), certificate)};

	QVERIFY(verify(policy, {certificate}, {QStringLiteral("192.0.2.10"), QStringLiteral("pc01.school.example")}).isVerified());
	QCOMPARE(verify(policy, {certificate}, {QStringLiteral("192.0.2.10"), QStringLiteral("pc02.school.example")}).error,
			 TlsError::PinMismatch);
	QCOMPARE(verify(policy, {certificate}, target(QStringLiteral("192.0.2.10"))).error, TlsError::PinMismatch);
	// the reverse DNS name never selects pins
	QCOMPARE(verify(policy, {certificate}, {QStringLiteral("192.0.2.10"), QString(), QStringLiteral("pc01.school.example")}).error,
			 TlsError::PinMismatch);
}



void TlsPeerVerifierTest::pinIgnoresValidityAndRevocation()
{
	// documented (05-tls.md §5.1): a pin is revoked only by removing its entry
	Specification specification;
	specification.notBeforeOffsetSeconds = -30LL * 24 * 60 * 60;
	specification.notAfterOffsetSeconds = -20LL * 24 * 60 * 60;
	specification.serialNumber = 0x42;
	specification.extendedKeyUsage.clear();
	const auto expired = leaf(specification);
	QVERIFY(expired.isNull() == false);

	auto policy = caPolicy();
	policy.revokedSerials = QStringList{QStringLiteral("42")};
	QCOMPARE(verify(policy, {expired}, target(QStringLiteral("pc01.school.example"))).error, TlsError::Revoked);

	policy.pins = {pin(QStringLiteral("pc01.school.example"), expired)};
	QVERIFY(verify(policy, {expired}, target(QStringLiteral("pc01.school.example"))).isVerified());
}



void TlsPeerVerifierTest::pinMismatchFallsBackToCa()
{
	const auto pinned = PkiTestCertificates::create(m_otherLeafKey, Specification{}, m_otherLeafKey, {}, m_now);
	const auto caSigned = leaf(QStringLiteral("DNS:pc01.school.example"));

	auto policy = caPolicy();
	policy.pins = {pin(QStringLiteral("pc01.school.example"), pinned)};

	// a CA-signed certificate still verifies (CA path), with all CA path checks
	QVERIFY(verify(policy, {caSigned}, target(QStringLiteral("pc01.school.example"))).isVerified());
	const auto wrongName = leaf(QStringLiteral("DNS:pc02.school.example"));
	QCOMPARE(verify(policy, {wrongName}, target(QStringLiteral("pc01.school.example"))).error, TlsError::NameMismatch);
	const auto selfSigned = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);
	QCOMPARE(verify(policy, {selfSigned}, target(QStringLiteral("pc01.school.example"))).error, TlsError::UntrustedCa);

	// a target without applicable pin uses the CA path as well
	QCOMPARE(verify(policy, {selfSigned}, target(QStringLiteral("pc03.school.example"))).error, TlsError::UntrustedCa);
}



void TlsPeerVerifierTest::barePinPerConnection()
{
	const auto certificate = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);
	const auto otherCertificate = PkiTestCertificates::create(m_otherLeafKey, Specification{}, m_otherLeafKey, {}, m_now);

	TlsClientPolicy policy;
	policy.mode = VeyonCore::TlsClientMode::Required;
	policy.pins = {pin({}, certificate)};

	QVERIFY(verify(policy, {certificate}, target(QStringLiteral("127.0.0.1"))).isVerified());
	QVERIFY(verify(policy, {certificate}, target(QStringLiteral("demo-host"))).isVerified());
	QCOMPARE(verify(policy, {otherCertificate}, target(QStringLiteral("127.0.0.1"))).error, TlsError::PinMismatch);
}



void TlsPeerVerifierTest::pinRejectsUnsupportedCriticalExtension()
{
	Specification specification;
	specification.unsupportedCriticalExtension = true;
	const auto certificate = PkiTestCertificates::create(m_leafKey, specification, m_leafKey, {}, m_now);
	QVERIFY(certificate.isNull() == false);

	TlsClientPolicy policy;
	policy.pins = {pin(QStringLiteral("pc01.school.example"), certificate)};
	const auto result = verify(policy, {certificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.error, TlsError::PinMismatch);
	// the log reason names the actual cause, not a key mismatch
	QVERIFY2(result.reason.contains(QStringLiteral("unsupported critical extension")), qPrintable(result.reason));
	QVERIFY(result.reason.contains(QStringLiteral("does not match")) == false);
}



void TlsPeerVerifierTest::parsePinEntry_data()
{
	QTest::addColumn<QString>("entry");
	QTest::addColumn<QString>("host");
	QTest::addColumn<bool>("valid");

	const auto hash = QStringLiteral("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
	const QString fingerprint = QStringLiteral("sha256:") + hash;

	QTest::newRow("dns") << QString(QStringLiteral("PC01.school.example=") + fingerprint) << QStringLiteral("pc01.school.example") << true;
	QTest::newRow("ipv4") << QString(QStringLiteral("192.0.2.10=") + fingerprint) << QStringLiteral("192.0.2.10") << true;
	QTest::newRow("ipv6") << QString(QStringLiteral("2001:db8::10=") + fingerprint) << QStringLiteral("2001:db8::10") << true;
	QTest::newRow("upper case hex") << QString(QStringLiteral("pc01=sha256:") + hash.toUpper()) << QStringLiteral("pc01") << true;
	QTest::newRow("surrounding space") << QString(QStringLiteral(" pc01=") + fingerprint + QStringLiteral(" ")) << QStringLiteral("pc01") << true;
	QTest::newRow("bare") << fingerprint << QString() << false;
	QTest::newRow("empty host") << QString(QStringLiteral("=") + fingerprint) << QString() << false;
	QTest::newRow("wildcard host") << QString(QStringLiteral("*.school.example=") + fingerprint) << QString() << false;
	QTest::newRow("two separators") << QString(QStringLiteral("pc01=pc02=") + fingerprint) << QString() << false;
	QTest::newRow("no prefix") << QString(QStringLiteral("pc01=") + hash) << QString() << false;
	QTest::newRow("upper case prefix") << QString(QStringLiteral("pc01=SHA256:") + hash) << QString() << false;
	QTest::newRow("short") << QString(QStringLiteral("pc01=") + fingerprint.left(fingerprint.size() - 1)) << QString() << false;
	QTest::newRow("long") << QString(QStringLiteral("pc01=") + fingerprint + QStringLiteral("0")) << QString() << false;
	QTest::newRow("non-hex") << QString(QStringLiteral("pc01=sha256:") + hash.left(63) + QStringLiteral("g")) << QString() << false;
	QTest::newRow("space inside") << QString(QStringLiteral("pc01 = ") + fingerprint) << QString() << false;
	QTest::newRow("empty") << QString() << QString() << false;
}



void TlsPeerVerifierTest::parsePinEntry()
{
	QFETCH(QString, entry);
	QFETCH(QString, host);
	QFETCH(bool, valid);

	const auto pinEntry = TlsClientPolicy::parsePinEntry(entry);
	QCOMPARE(pinEntry.has_value(), valid);
	if (valid)
	{
		QCOMPARE(pinEntry->host, host);
		QCOMPARE(pinEntry->spkiSha256.toHex(), QByteArrayLiteral("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"));
	}
}



void TlsPeerVerifierTest::normalizedSerialNumber_data()
{
	QTest::addColumn<QString>("serial");
	QTest::addColumn<QString>("normalizedSerial");

	QTest::newRow("plain") << QStringLiteral("0a1b2c") << QStringLiteral("a1b2c");
	QTest::newRow("upper case") << QStringLiteral("0A1B2C") << QStringLiteral("a1b2c");
	QTest::newRow("leading zeros") << QStringLiteral("0000a1b2c") << QStringLiteral("a1b2c");
	QTest::newRow("zero") << QStringLiteral("00") << QStringLiteral("0");
	QTest::newRow("space") << QStringLiteral(" a1 ") << QStringLiteral("a1");
	QTest::newRow("colons") << QStringLiteral("0A:1B:2C") << QStringLiteral("a1b2c");
	QTest::newRow("spaces") << QStringLiteral("0a 1b 2c") << QStringLiteral("a1b2c");
	QTest::newRow("0x prefix") << QStringLiteral("0x0A1B2C") << QStringLiteral("a1b2c");
	QTest::newRow("upper case 0x prefix") << QStringLiteral("0X0a1b2c") << QStringLiteral("a1b2c");
	QTest::newRow("only prefix") << QStringLiteral("0x") << QString();
	QTest::newRow("prefix inside") << QStringLiteral("0a0x1b") << QString();
	QTest::newRow("non-hex") << QStringLiteral("xyz") << QString();
	QTest::newRow("empty") << QString() << QString();
}



void TlsPeerVerifierTest::normalizedSerialNumber()
{
	QFETCH(QString, serial);
	QFETCH(QString, normalizedSerial);

	QCOMPARE(TlsClientPolicy::normalizedSerialNumber(serial), normalizedSerial);
}



void TlsPeerVerifierTest::invalidRevokedSerialFailsClosed()
{
	const auto certificate = leaf(QStringLiteral("DNS:pc01.school.example"));
	const auto pinned = PkiTestCertificates::create(m_otherLeafKey, Specification{}, m_otherLeafKey, {}, m_now);

	QTemporaryFile file;
	QVERIFY(file.open());
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	configuration.setTlsCaCertificate(QString::fromLatin1(m_caCertificate.toPem()));
	configuration.setTlsPinnedFingerprints({QString(QStringLiteral("pc02.school.example=") + pinned.spkiFingerprint())});
	// a decimal/garbled entry must not silently drop the revocation of that certificate
	configuration.setTlsRevokedSerials({QStringLiteral("0a:1b"), QStringLiteral("serial 12")});

	QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("invalid TLS/RevokedSerials entry 1")));
	const auto policy = TlsClientPolicy::fromConfiguration(configuration);
	QCOMPARE(policy.revokedSerials, QStringList{QStringLiteral("a1b")});
	QVERIFY(policy.hasInvalidRevokedSerials);

	const auto result = verify(policy, {certificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::UntrustedCa);
	QVERIFY(result.reason.contains(QStringLiteral("TLS/RevokedSerials")));

	// pins ignore TLS/RevokedSerials (05-tls.md §5.1)
	QVERIFY(verify(policy, {pinned}, target(QStringLiteral("pc02.school.example"))).isVerified());

	// per-connection policies are checked the same way
	auto codePolicy = caPolicy();
	codePolicy.revokedSerials = QStringList{QStringLiteral("xyz")};
	QCOMPARE(verify(codePolicy, {certificate}, target(QStringLiteral("pc01.school.example"))).error, TlsError::UntrustedCa);
}



void TlsPeerVerifierTest::emptyConfigurationEntriesAreSkipped()
{
	// e.g. `veyon-cli config set TLS/PinnedFingerprints ""` stores [""]
	QTemporaryFile file;
	QVERIFY(file.open());
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	configuration.setTlsCaCertificate(QStringLiteral(" \n"));
	configuration.setTlsPinnedFingerprints({QString(), QStringLiteral("  ")});
	configuration.setTlsRevokedSerials({QString(), QStringLiteral(" ")});

	const auto policy = TlsClientPolicy::fromConfiguration(configuration);
	QVERIFY(policy.hasIgnoredPinEntries == false);
	QVERIFY(policy.hasInvalidRevokedSerials == false);
	QVERIFY(policy.hasInvalidCaCertificates == false);
	QVERIFY(policy.hasTrustAnchor() == false);
	QVERIFY(policy.revokedSerials.isEmpty());

	const auto certificate = leaf(QStringLiteral("DNS:pc01.school.example"));
	QCOMPARE(verify(policy, {certificate}, target(QStringLiteral("pc01.school.example"))).status, TlsPeerVerifier::Status::Unverified);
}



void TlsPeerVerifierTest::policyFromConfiguration()
{
	const auto pinnedCertificate = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);

	QTemporaryFile file;
	QVERIFY(file.open());
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	configuration.setTlsClientMode(VeyonCore::TlsClientMode::Required);
	configuration.setTlsIpTargetPolicy(VeyonCore::TlsIpTargetPolicy::RequireIpSan);
	configuration.setTlsCaCertificate(QString::fromLatin1(QByteArray(m_caCertificate.toPem() + m_otherCaCertificate.toPem())));
	configuration.setTlsPinnedFingerprints({QString(QStringLiteral("PC01.school.example=") + pinnedCertificate.spkiFingerprint())});
	configuration.setTlsRevokedSerials({QStringLiteral("00AB"), QStringLiteral("cd")});

	const auto policy = TlsClientPolicy::fromConfiguration(configuration);
	QCOMPARE(policy.mode, VeyonCore::TlsClientMode::Required);
	QCOMPARE(policy.ipTargetPolicy, VeyonCore::TlsIpTargetPolicy::RequireIpSan);
	QCOMPARE(policy.caCertificates.size(), 2);
	QCOMPARE(policy.caCertificates.at(0).spkiSha256(), m_caCertificate.spkiSha256());
	QCOMPARE(policy.pins.size(), 1);
	QCOMPARE(policy.pins.first().host, QStringLiteral("pc01.school.example"));
	QCOMPARE(policy.pins.first().spkiSha256, pinnedCertificate.spkiSha256());
	QCOMPARE(policy.revokedSerials, QStringList({QStringLiteral("ab"), QStringLiteral("cd")}));
	QVERIFY(policy.hasInvalidCaCertificates == false && policy.hasIgnoredPinEntries == false);
	QVERIFY(policy.hasTrustAnchor());

	QVERIFY(verify(policy, {pinnedCertificate}, target(QStringLiteral("pc01.school.example"))).isVerified());
}



void TlsPeerVerifierTest::enumValuesFromConfigurationFile()
{
	// configuration files, GPOs and veyon-cli store enumerator names
	QTemporaryFile file;
	QVERIFY(file.open());
	file.write(R"({"TLS":{"ClientMode":"Required","IpTargetPolicy":"RequireIpSan"}})");
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	auto policy = TlsClientPolicy::fromConfiguration(configuration);
	QCOMPARE(policy.mode, VeyonCore::TlsClientMode::Required);
	QCOMPARE(policy.ipTargetPolicy, VeyonCore::TlsIpTargetPolicy::RequireIpSan);

	configuration.setTlsClientMode(VeyonCore::TlsClientMode::Disabled);
	configuration.setTlsIpTargetPolicy(VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithReverseDnsHostname);
	policy = TlsClientPolicy::fromConfiguration(configuration);
	QCOMPARE(policy.mode, VeyonCore::TlsClientMode::Disabled);
	QCOMPARE(policy.ipTargetPolicy, VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithReverseDnsHostname);
}



void TlsPeerVerifierTest::numericEnumValuesFromConfiguration()
{
	// LocalStore and JsonStore persist enums as numbers; QSettings INI files return them as numeric
	// strings, `veyon-cli config set TLS/ClientMode 1` stores "1"
	const QList<QByteArray> files{
		R"({"TLS":{"ClientMode":1,"IpTargetPolicy":2}})",
		R"({"TLS":{"ClientMode":"1","IpTargetPolicy":"2"}})",
		R"({"TLS":{"ClientMode":1.0,"IpTargetPolicy":2.0}})",
	};

	for (const auto& content : files)
	{
		QTemporaryFile file;
		QVERIFY(file.open());
		file.write(content);
		file.close();

		VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
		const auto policy = TlsClientPolicy::fromConfiguration(configuration);
		QCOMPARE(policy.mode, VeyonCore::TlsClientMode::Prefer);
		QCOMPARE(policy.ipTargetPolicy, VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithReverseDnsHostname);
		QCOMPARE(TlsConfiguration::clientMode(configuration), configuration.tlsClientMode());
		QCOMPARE(TlsConfiguration::ipTargetPolicy(configuration), configuration.tlsIpTargetPolicy());
	}
}



void TlsPeerVerifierTest::settingsRoundTrip()
{
	// what LocalStore reads back from INI files: enums persisted as numbers come back as strings
	QTemporaryFile file;
	QVERIFY(file.open());
	file.write("[TLS]\nClientMode=0\nServerMode=Required\nIpTargetPolicy=1\n");
	file.close();

	QSettings settings(file.fileName(), QSettings::IniFormat);
	const auto clientMode = settings.value(QStringLiteral("TLS/ClientMode"));
	QCOMPARE(clientMode.userType(), int(QMetaType::QString));
	QCOMPARE(TlsConfiguration::enumValue<VeyonCore::TlsClientMode>(clientMode),
			 std::optional<VeyonCore::TlsClientMode>(VeyonCore::TlsClientMode::Disabled));
	QCOMPARE(TlsConfiguration::enumValue<VeyonCore::TlsServerMode>(settings.value(QStringLiteral("TLS/ServerMode"))),
			 std::optional<VeyonCore::TlsServerMode>(VeyonCore::TlsServerMode::Required));
	QCOMPARE(TlsConfiguration::enumValue<VeyonCore::TlsIpTargetPolicy>(settings.value(QStringLiteral("TLS/IpTargetPolicy"))),
			 std::optional<VeyonCore::TlsIpTargetPolicy>(VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithHostname));
}



void TlsPeerVerifierTest::enumValueForms_data()
{
	QTest::addColumn<QVariant>("value");
	QTest::addColumn<int>("expected"); // -1: invalid

	QTest::newRow("name") << QVariant(QStringLiteral("Required")) << int(VeyonCore::TlsClientMode::Required);
	QTest::newRow("numeric string") << QVariant(QStringLiteral("1")) << int(VeyonCore::TlsClientMode::Prefer);
	QTest::newRow("int") << QVariant(2) << int(VeyonCore::TlsClientMode::Required);
	QTest::newRow("integral double") << QVariant(0.0) << int(VeyonCore::TlsClientMode::Disabled);
	QTest::newRow("enum") << QVariant::fromValue(VeyonCore::TlsClientMode::Prefer) << int(VeyonCore::TlsClientMode::Prefer);
	QTest::newRow("lower case name") << QVariant(QStringLiteral("required")) << -1;
	QTest::newRow("qualified name") << QVariant(QStringLiteral("TlsClientMode::Required")) << -1;
	QTest::newRow("out of range") << QVariant(3) << -1;
	QTest::newRow("negative") << QVariant(-1) << -1;
	QTest::newRow("out of range string") << QVariant(QStringLiteral("7")) << -1;
	QTest::newRow("fraction") << QVariant(1.5) << -1;
	QTest::newRow("huge double") << QVariant(1e300) << -1;
	QTest::newRow("bool") << QVariant(true) << -1;
	QTest::newRow("empty string") << QVariant(QString()) << -1;
	QTest::newRow("invalid") << QVariant() << -1;
}



void TlsPeerVerifierTest::enumValueForms()
{
	QFETCH(QVariant, value);
	QFETCH(int, expected);

	const auto result = TlsConfiguration::enumValue<VeyonCore::TlsClientMode>(value);
	QCOMPARE(result.has_value(), expected >= 0);
	if (expected >= 0)
	{
		QCOMPARE(int(*result), expected);
	}
}



void TlsPeerVerifierTest::serverModeFailsClosed()
{
	const QList<QPair<QByteArray, int>> files{
		{R"({"TLS":{"ServerMode":"Disabled"}})", int(VeyonCore::TlsServerMode::Disabled)},
		{R"({"TLS":{"ServerMode":"1"}})", int(VeyonCore::TlsServerMode::Optional)},
		{R"({"TLS":{"ServerMode":2}})", int(VeyonCore::TlsServerMode::Required)},
		{R"({"TLS":{}})", int(VeyonCore::TlsServerMode::Optional)},
		{R"({"TLS":{"ServerMode":"Requierd"}})", -1},
		{R"({"TLS":{"ServerMode":"required"}})", -1},
		{R"({"TLS":{"ServerMode":5}})", -1},
	};

	for (const auto& file : files)
	{
		QTemporaryFile configurationFile;
		QVERIFY(configurationFile.open());
		configurationFile.write(file.first);
		configurationFile.close();

		VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, configurationFile.fileName()));
		if (file.second < 0)
		{
			// the generic accessor would return Disabled or an undefined value
			QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("invalid value of .TLS/ServerMode. - using Required")));
			QCOMPARE(TlsConfiguration::serverMode(configuration), VeyonCore::TlsServerMode::Required);
		}
		else
		{
			QCOMPARE(int(TlsConfiguration::serverMode(configuration)), file.second);
		}
	}
}



void TlsPeerVerifierTest::invalidEnumValuesFailClosed()
{
	// an invalid value would otherwise be read as the first enumerator, TlsClientMode::Disabled
	const QList<QByteArray> files{
		R"({"TLS":{"ClientMode":"Requierd","IpTargetPolicy":"AcceptAnything"}})",
		R"({"TLS":{"ClientMode":42,"IpTargetPolicy":-1}})",
		R"({"TLS":{"ClientMode":"","IpTargetPolicy":"acceptCaSignedWithHostname"}})",
		R"({"TLS":{"ClientMode":true,"IpTargetPolicy":1.5}})",
		R"({"TLS":{"ClientMode":"Prefer ","IpTargetPolicy":"3"}})",
	};

	for (const auto& content : files)
	{
		QTemporaryFile file;
		QVERIFY(file.open());
		file.write(content);
		file.close();

		VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
		QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("invalid value of .TLS/ClientMode. - using Required")));
		QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral("invalid value of .TLS/IpTargetPolicy. - using RequireIpSan")));
		const auto policy = TlsClientPolicy::fromConfiguration(configuration);
		QCOMPARE(policy.mode, VeyonCore::TlsClientMode::Required);
		QCOMPARE(policy.ipTargetPolicy, VeyonCore::TlsIpTargetPolicy::RequireIpSan);
	}
}



void TlsPeerVerifierTest::policyDefaultsFromConfiguration()
{
	QTemporaryFile file;
	QVERIFY(file.open());
	file.write("{}");
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	QCOMPARE(configuration.tlsServerMode(), VeyonCore::TlsServerMode::Optional);
	QCOMPARE(configuration.tlsClientMode(), VeyonCore::TlsClientMode::Prefer);
	QCOMPARE(configuration.tlsIpTargetPolicy(), VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithHostname);
	QCOMPARE(configuration.tlsCaCertificate(), QString());
	QCOMPARE(configuration.tlsPinnedFingerprints(), QStringList());
	QCOMPARE(configuration.tlsRevokedSerials(), QStringList());
	QCOMPARE(configuration.tlsServerModeProperty().key(), QStringLiteral("ServerMode"));
	QCOMPARE(configuration.tlsServerModeProperty().parentKey(), QStringLiteral("TLS"));
	QCOMPARE(configuration.tlsClientModeProperty().key(), QStringLiteral("ClientMode"));
	QCOMPARE(configuration.tlsCaCertificateProperty().key(), QStringLiteral("CaCertificate"));
	QCOMPARE(configuration.tlsPinnedFingerprintsProperty().key(), QStringLiteral("PinnedFingerprints"));
	QCOMPARE(configuration.tlsRevokedSerialsProperty().key(), QStringLiteral("RevokedSerials"));
	QCOMPARE(configuration.tlsIpTargetPolicyProperty().key(), QStringLiteral("IpTargetPolicy"));

	const auto policy = TlsClientPolicy::fromConfiguration(configuration);
	QCOMPARE(policy.mode, VeyonCore::TlsClientMode::Prefer);
	QVERIFY(policy.hasTrustAnchor() == false);
}



void TlsPeerVerifierTest::bareConfigurationPinIsIgnoredWithWarning()
{
	const auto certificate = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);

	QTemporaryFile file;
	QVERIFY(file.open());
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	configuration.setTlsPinnedFingerprints({certificate.spkiFingerprint()});

	QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("ignoring TLS/PinnedFingerprints entry 0")));
	const auto policy = TlsClientPolicy::fromConfiguration(configuration);
	QVERIFY(policy.pins.isEmpty());
	QVERIFY(policy.hasIgnoredPinEntries);
	QVERIFY(policy.hasTrustAnchor());

	// the bare entry authenticates nothing, and its presence still blocks the unverified result
	const auto result = verify(policy, {certificate}, target(QStringLiteral("pc01.school.example")));
	QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
	QCOMPARE(result.error, TlsError::PinMismatch);
}



void TlsPeerVerifierTest::invalidCaConfigurationFailsClosed()
{
	QTemporaryFile file;
	QVERIFY(file.open());
	file.close();

	const auto certificate = leaf(QStringLiteral("DNS:pc01.school.example"));
	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));

	// garbage, an end-entity certificate, a valid CA together with an invalid block
	const QStringList invalidValues{
		QStringLiteral("-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n"),
		QString::fromLatin1(certificate.toPem()),
		QString::fromLatin1(QByteArray(m_caCertificate.toPem() + "-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n")),
		QStringLiteral("no PEM at all"),
	};

	for (const auto& value : invalidValues)
	{
		configuration.setTlsCaCertificate(value);
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("TLS/CaCertificate is invalid")));
		const auto policy = TlsClientPolicy::fromConfiguration(configuration);
		QVERIFY(policy.caCertificates.isEmpty());
		QVERIFY(policy.hasInvalidCaCertificates);
		QVERIFY(policy.hasTrustAnchor());

		const auto result = verify(policy, {certificate}, target(QStringLiteral("pc01.school.example")));
		QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
		QCOMPARE(result.error, TlsError::UntrustedCa);
	}
}



void TlsPeerVerifierTest::invalidPinConfigurationFailsClosed()
{
	QTemporaryFile file;
	QVERIFY(file.open());
	file.close();

	VeyonConfiguration configuration(new Configuration::JsonStore(Configuration::Store::Scope::System, file.fileName()));
	configuration.setTlsPinnedFingerprints({QStringLiteral("pc01.school.example=sha256:1234")});

	QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("ignoring TLS/PinnedFingerprints entry 0")));
	const auto policy = TlsClientPolicy::fromConfiguration(configuration);
	const auto certificate = PkiTestCertificates::create(m_leafKey, Specification{}, m_leafKey, {}, m_now);
	QCOMPARE(verify(policy, {certificate}, target(QStringLiteral("pc01.school.example"))).error, TlsError::PinMismatch);
}



void TlsPeerVerifierTest::configureSocket()
{
	QSslConfiguration configuration;
	TlsPeerVerifier(caPolicy()).configure(configuration);
	QCOMPARE(configuration.peerVerifyMode(), QSslSocket::VerifyPeer);
	QCOMPARE(configuration.caCertificates(), QList<QSslCertificate>{m_caCertificate.toQSslCertificate()});

	TlsClientPolicy pinPolicy;
	pinPolicy.pins = {pin(QStringLiteral("pc01"), m_caCertificate)};
	configuration = QSslConfiguration::defaultConfiguration();
	TlsPeerVerifier(pinPolicy).configure(configuration);
	QCOMPARE(configuration.peerVerifyMode(), QSslSocket::VerifyNone);
	// never the system CA store
	QVERIFY(configuration.caCertificates().isEmpty());

	// a configured but invalid CA keeps the socket verifying (against no CA at all)
	TlsClientPolicy invalidCaPolicy;
	invalidCaPolicy.hasInvalidCaCertificates = true;
	TlsPeerVerifier(invalidCaPolicy).configure(configuration);
	QCOMPARE(configuration.peerVerifyMode(), QSslSocket::VerifyPeer);
	QVERIFY(configuration.caCertificates().isEmpty());
}



void TlsPeerVerifierTest::handshake_data()
{
	QTest::addColumn<QString>("subjectAltNames");
	QTest::addColumn<bool>("signedByConfiguredCa");
	QTest::addColumn<qint64>("notBeforeOffset");
	QTest::addColumn<qint64>("notAfterOffset");
	QTest::addColumn<QString>("host");
	QTest::addColumn<QString>("directoryHostName");
	QTest::addColumn<bool>("pinned");
	QTest::addColumn<bool>("withCa");
	QTest::addColumn<TlsError>("error");

	constexpr qint64 Minute = 60;
	constexpr qint64 Day = 24 * 60 * Minute;
	const auto ip = QStringLiteral("127.0.0.1");
	const auto ipSan = QStringLiteral("IP:127.0.0.1");

	QTest::newRow("CA, IP SAN") << ipSan << true << -Minute << Day << ip << QString() << false << true << TlsError::None;
	QTest::newRow("CA, wrong IP SAN") << QStringLiteral("IP:127.0.0.2") << true << -Minute << Day << ip << QString() << false << true
									  << TlsError::NameMismatch;
	QTest::newRow("CA, directory host name") << QStringLiteral("DNS:pc01.school.example") << true << -Minute << Day << ip
											 << QStringLiteral("pc01.school.example") << false << true << TlsError::None;
	QTest::newRow("CA, expired 4 min ago") << ipSan << true << -Day << -4 * Minute << ip << QString() << false << true << TlsError::None;
	QTest::newRow("CA, valid in 4 min") << ipSan << true << 4 * Minute << Day << ip << QString() << false << true << TlsError::None;
	QTest::newRow("CA, expired") << ipSan << true << -3 * Day << -2 * Day << ip << QString() << false << true << TlsError::Expired;
	QTest::newRow("foreign CA") << ipSan << false << -Minute << Day << ip << QString() << false << true << TlsError::UntrustedCa;
	QTest::newRow("pin") << QString() << false << -2 * Day << -Day << ip << QString() << true << false << TlsError::None;
	QTest::newRow("pin on pin+CA policy") << QString() << false << -Minute << Day << ip << QString() << true << true << TlsError::None;
	QTest::newRow("pin mismatch") << QString() << false << -Minute << Day << QStringLiteral("localhost") << QString() << true << false
								  << TlsError::PinMismatch;
}



void TlsPeerVerifierTest::handshake()
{
	QFETCH(QString, subjectAltNames);
	QFETCH(bool, signedByConfiguredCa);
	QFETCH(qint64, notBeforeOffset);
	QFETCH(qint64, notAfterOffset);
	QFETCH(QString, host);
	QFETCH(QString, directoryHostName);
	QFETCH(bool, pinned);
	QFETCH(bool, withCa);
	QFETCH(TlsError, error);

	if (m_openSslBackendAvailable == false)
	{
		QSKIP("Qt OpenSSL TLS backend not available");
	}

	const auto now = QDateTime::fromSecsSinceEpoch(QDateTime::currentSecsSinceEpoch(), QTimeZone::utc());
	Specification specification;
	specification.subjectAltNames = subjectAltNames;
	specification.notBeforeOffsetSeconds = notBeforeOffset;
	specification.notAfterOffsetSeconds = notAfterOffset;
	specification.serialNumber = ++m_serialNumber;
	const auto& issuerKey = signedByConfiguredCa ? m_caKey : m_otherCaKey;
	const auto& issuer = signedByConfiguredCa ? m_caCertificate : m_otherCaCertificate;
	const auto certificate = PkiTestCertificates::create(m_leafKey, specification, issuerKey, issuer.subjectCommonName(), now);
	QVERIFY(certificate.isNull() == false);
	const auto privateKeyDer = m_leafKey.toPkcs8Der();
	QVERIFY(privateKeyDer.has_value());
	const QSslKey privateKey(privateKeyDer->toUnprotectedByteArray(), QSsl::Ec, QSsl::Der);
	QVERIFY(privateKey.isNull() == false);

	TlsTestServer server(certificate.toQSslCertificate(), privateKey);
	QVERIFY(server.listen(QHostAddress::LocalHost));

	TlsClientPolicy policy;
	if (pinned)
	{
		policy.pins = {pin(QStringLiteral("127.0.0.1"), certificate)};
	}
	if (withCa)
	{
		policy.caCertificates = {m_caCertificate};
	}

	QSslSocket client;
	auto configuration = client.sslConfiguration();
	configuration.setProtocol(QSsl::TlsV1_3OrLater);
	client.setSslConfiguration(configuration);

	int resultCount = 0;
	TlsPeerVerifier::Result result;
	QByteArray clientReceivedData;
	{
		// the verifier does not need to outlive attach()
		const TlsPeerVerifier verifier(policy);
		verifier.attach(&client, {host, directoryHostName}, [&](const TlsPeerVerifier::Result& verificationResult) {
			++resultCount;
			result = verificationResult;
			if (verificationResult.status != TlsPeerVerifier::Status::Rejected)
			{
				client.write("client-data");
			}
		});
	}
	connect(&client, &QSslSocket::readyRead, this, [&]() {
		clientReceivedData += client.readAll();
	});

	client.connectToHostEncrypted(server.serverAddress().toString(), server.serverPort(), host);

	if (error == TlsError::None)
	{
		QTRY_COMPARE_WITH_TIMEOUT(server.receivedData(), QByteArrayLiteral("client-data"), 10000);
		QTRY_COMPARE_WITH_TIMEOUT(clientReceivedData, QByteArray(TlsTestServer::ServerData), 10000);
		QVERIFY2(result.isVerified(), qPrintable(result.reason));
	}
	else
	{
		QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::UnconnectedState, 10000);
		// give a misbehaving implementation the chance to exchange data
		QTest::qWait(200);
		QVERIFY2(result.error == error, qPrintable(result.reason));
		QCOMPARE(result.status, TlsPeerVerifier::Status::Rejected);
		QVERIFY(server.receivedData().isEmpty());
		QVERIFY(clientReceivedData.isEmpty());
	}
	QCOMPARE(resultCount, 1);
}



QTEST_GUILESS_MAIN(TlsPeerVerifierTest)
#include "TlsPeerVerifierTest.moc"
