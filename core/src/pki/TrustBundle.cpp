// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QUrl>

#include "TlsClientPolicy.h"
#include "TlsPeerVerifier.h"
#include "TrustBundle.h"

#include <algorithm>
#include <cmath>


namespace TrustBundleKeys {

inline constexpr auto Version = QLatin1String("v");
inline constexpr auto CaCertificates = QLatin1String("caCertificates");
inline constexpr auto Enrollment = QLatin1String("enrollment");
inline constexpr auto EnrollmentUrl = QLatin1String("url");
inline constexpr auto EnrollmentToken = QLatin1String("token");
inline constexpr auto Policy = QLatin1String("policy");
inline constexpr auto CertValidityDays = QLatin1String("certValidityDays");
inline constexpr auto RenewBeforeDays = QLatin1String("renewBeforeDays");
inline constexpr auto Modes = QLatin1String("modes");
inline constexpr auto ServerMode = QLatin1String("server");
inline constexpr auto ClientMode = QLatin1String("client");

} // namespace TrustBundleKeys



// exact enumerator name, e.g. "Required" (no numbers, no scope prefixes)
template<typename T>
static std::optional<T> trustBundleEnumValue(const QJsonValue& value)
{
	if (value.isString() == false)
	{
		return std::nullopt;
	}

	const auto metaEnum = QMetaEnum::fromType<T>();
	const auto name = value.toString();
	for (int index = 0; index < metaEnum.keyCount(); ++index)
	{
		if (name == QLatin1String(metaEnum.key(index)))
		{
			return T(metaEnum.value(index));
		}
	}

	return std::nullopt;
}



template<typename T>
static QString trustBundleEnumName(T value)
{
	return QLatin1String(QMetaEnum::fromType<T>().valueToKey(int(value)));
}



static std::optional<int> trustBundleInteger(const QJsonValue& value, int minimum, int maximum)
{
	if (value.isDouble() == false)
	{
		return std::nullopt;
	}

	const auto number = value.toDouble();
	if (std::isfinite(number) == false || std::floor(number) != number || number < minimum || number > maximum)
	{
		return std::nullopt;
	}

	return int(number);
}



std::optional<TrustBundle> TrustBundle::fromJson(VeyonCrypto::ByteView json)
{
	if (json.isEmpty() || json.size() > MaximumSize)
	{
		vWarning() << "refusing trust bundle of" << json.size() << "bytes (limit" << qsizetype(MaximumSize) << ")";
		return std::nullopt;
	}

	const auto refuse = [](const char* reason) -> std::optional<TrustBundle> {
		vWarning() << "refusing trust bundle:" << reason;
		return std::nullopt;
	};

	QJsonParseError parseError{};
	const auto document = QJsonDocument::fromJson(QByteArray(json.data(), json.size()), &parseError);
	if (parseError.error != QJsonParseError::NoError || document.isObject() == false)
	{
		return refuse("no valid JSON object");
	}
	const auto object = document.object();

	const auto version = trustBundleInteger(object.value(TrustBundleKeys::Version), Version, Version);
	if (version.has_value() == false)
	{
		return refuse("unsupported version");
	}

	TrustBundle bundle;

	const auto caCertificatesValue = object.value(TrustBundleKeys::CaCertificates);
	const auto caCertificatesPem = caCertificatesValue.toString().toUtf8();
	// other PEM blocks are skipped by the parser, but a private key (e.g. the CA key) in a
	// distributed bundle is an incident the admin has to notice
	if (caCertificatesPem.contains("PRIVATE KEY-----"))
	{
		return refuse("CA certificates contain a private key");
	}
	const auto caCertificates = TlsClientPolicy::parseCaCertificates(caCertificatesPem);
	if (caCertificatesValue.isString() == false || caCertificates.has_value() == false)
	{
		return refuse("missing or invalid CA certificates");
	}
	bundle.caCertificates = *caCertificates;

	if (object.contains(TrustBundleKeys::Enrollment))
	{
		const auto enrollmentValue = object.value(TrustBundleKeys::Enrollment);
		if (enrollmentValue.isObject() == false)
		{
			return refuse("invalid enrollment section");
		}
		const auto enrollment = enrollmentValue.toObject();

		if (enrollment.contains(TrustBundleKeys::EnrollmentUrl))
		{
			const auto url = enrollment.value(TrustBundleKeys::EnrollmentUrl);
			bundle.enrollmentUrl = url.isString() ? normalizedEnrollmentUrl(url.toString()) : QString{};
			if (bundle.enrollmentUrl.isEmpty())
			{
				return refuse("invalid enrollment URL");
			}
		}

		if (enrollment.contains(TrustBundleKeys::EnrollmentToken))
		{
			const auto token = enrollment.value(TrustBundleKeys::EnrollmentToken);
			if (token.isString() == false)
			{
				return refuse("invalid enrollment token");
			}
			bundle.enrollmentToken = VeyonCrypto::SecureBuffer::fromByteArray(token.toString().toUtf8());
			if (isValidEnrollmentToken(bundle.enrollmentToken) == false)
			{
				return refuse("invalid enrollment token");
			}
			if (bundle.enrollmentUrl.isEmpty())
			{
				return refuse("enrollment token without enrollment URL");
			}
		}
	}

	if (object.contains(TrustBundleKeys::Policy))
	{
		const auto policyValue = object.value(TrustBundleKeys::Policy);
		if (policyValue.isObject() == false)
		{
			return refuse("invalid policy section");
		}
		const auto policy = policyValue.toObject();

		if (policy.contains(TrustBundleKeys::CertValidityDays))
		{
			bundle.certValidityDays = trustBundleInteger(policy.value(TrustBundleKeys::CertValidityDays),
														 MinimumCertValidityDays, MaximumCertValidityDays);
			if (bundle.certValidityDays.has_value() == false)
			{
				return refuse("invalid certificate validity");
			}
		}

		if (policy.contains(TrustBundleKeys::RenewBeforeDays))
		{
			bundle.renewBeforeDays = trustBundleInteger(policy.value(TrustBundleKeys::RenewBeforeDays),
														MinimumRenewBeforeDays, MaximumRenewBeforeDays);
			if (bundle.renewBeforeDays.has_value() == false)
			{
				return refuse("invalid renewal lead time");
			}
		}
	}

	if (object.contains(TrustBundleKeys::Modes))
	{
		const auto modesValue = object.value(TrustBundleKeys::Modes);
		if (modesValue.isObject() == false)
		{
			return refuse("invalid modes section");
		}
		const auto modes = modesValue.toObject();

		if (modes.contains(TrustBundleKeys::ServerMode))
		{
			bundle.serverMode = trustBundleEnumValue<VeyonCore::TlsServerMode>(modes.value(TrustBundleKeys::ServerMode));
			if (bundle.serverMode.has_value() == false)
			{
				return refuse("invalid server mode");
			}
		}

		if (modes.contains(TrustBundleKeys::ClientMode))
		{
			bundle.clientMode = trustBundleEnumValue<VeyonCore::TlsClientMode>(modes.value(TrustBundleKeys::ClientMode));
			if (bundle.clientMode.has_value() == false)
			{
				return refuse("invalid client mode");
			}
		}
	}

	return bundle;
}



std::optional<VeyonCrypto::SecureBuffer> TrustBundle::toJson() const
{
	if (isValid() == false)
	{
		vWarning() << "not creating invalid trust bundle";
		return std::nullopt;
	}

	QJsonObject object{
		{TrustBundleKeys::Version, Version},
		{TrustBundleKeys::CaCertificates, caCertificatesPem()},
	};

	if (enrollmentUrl.isEmpty() == false)
	{
		QJsonObject enrollment{{TrustBundleKeys::EnrollmentUrl, normalizedEnrollmentUrl(enrollmentUrl)}};
		if (enrollmentToken.isEmpty() == false)
		{
			enrollment[TrustBundleKeys::EnrollmentToken] = QString::fromLatin1(enrollmentToken.constData(), enrollmentToken.size());
		}
		object[TrustBundleKeys::Enrollment] = enrollment;
	}

	QJsonObject policy;
	if (certValidityDays.has_value())
	{
		policy[TrustBundleKeys::CertValidityDays] = *certValidityDays;
	}
	if (renewBeforeDays.has_value())
	{
		policy[TrustBundleKeys::RenewBeforeDays] = *renewBeforeDays;
	}
	if (policy.isEmpty() == false)
	{
		object[TrustBundleKeys::Policy] = policy;
	}

	QJsonObject modes;
	if (serverMode.has_value())
	{
		modes[TrustBundleKeys::ServerMode] = trustBundleEnumName(*serverMode);
	}
	if (clientMode.has_value())
	{
		modes[TrustBundleKeys::ClientMode] = trustBundleEnumName(*clientMode);
	}
	if (modes.isEmpty() == false)
	{
		object[TrustBundleKeys::Modes] = modes;
	}

	auto json = VeyonCrypto::SecureBuffer::fromByteArray(QJsonDocument(object).toJson(QJsonDocument::Indented));
	if (json.size() > MaximumSize)
	{
		vWarning() << "trust bundle exceeds" << qsizetype(MaximumSize) << "bytes";
		return std::nullopt;
	}

	return json;
}



QString TrustBundle::caCertificatesPem() const
{
	QByteArray pem;
	for (const auto& certificate : caCertificates)
	{
		pem.append(certificate.toPem());
	}

	return QString::fromLatin1(pem);
}



bool TrustBundle::isValid() const
{
	const auto isValidEnum = [](auto value) {
		return QMetaEnum::fromType<decltype(value)>().valueToKey(int(value)) != nullptr;
	};

	if (caCertificates.isEmpty() || caCertificates.size() > VeyonCrypto::Certificate::MaximumChainLength ||
		std::all_of(caCertificates.cbegin(), caCertificates.cend(), TlsClientPolicy::isAcceptableCaCertificate) == false)
	{
		return false;
	}

	return (enrollmentUrl.isEmpty() || isValidEnrollmentUrl(enrollmentUrl)) &&
		(enrollmentToken.isEmpty() || (enrollmentUrl.isEmpty() == false && isValidEnrollmentToken(enrollmentToken))) &&
		(certValidityDays.has_value() == false ||
		 (*certValidityDays >= MinimumCertValidityDays && *certValidityDays <= MaximumCertValidityDays)) &&
		(renewBeforeDays.has_value() == false ||
		 (*renewBeforeDays >= MinimumRenewBeforeDays && *renewBeforeDays <= MaximumRenewBeforeDays)) &&
		(serverMode.has_value() == false || isValidEnum(*serverMode)) &&
		(clientMode.has_value() == false || isValidEnum(*clientMode));
}



bool TrustBundle::isValidEnrollmentUrl(const QString& url)
{
	return normalizedEnrollmentUrl(url).isEmpty() == false;
}



QString TrustBundle::normalizedEnrollmentUrl(const QString& url)
{
	static const auto authorityPrefix = QStringLiteral("://");

	if (url.isEmpty() || url.size() > MaximumUrlSize ||
		std::any_of(url.cbegin(), url.cend(), [](QChar character) {
		return character.unicode() <= 0x20 || character.unicode() == 0x7f || character == QLatin1Char('\\');
	}))
	{
		return {};
	}

	const QUrl parsedUrl(url, QUrl::StrictMode);
	const auto path = parsedUrl.path();
	const auto schemeEnd = url.indexOf(authorityPrefix);
	if (parsedUrl.isValid() == false ||
		parsedUrl.scheme() != QLatin1String("https") ||
		schemeEnd != parsedUrl.scheme().size() ||
		(path.isEmpty() == false && path != QLatin1String("/")) ||
		parsedUrl.hasQuery() ||
		parsedUrl.hasFragment())
	{
		return {};
	}

	// the authority as written: no user info (not even an empty one), host, optional port
	const auto authorityStart = schemeEnd + authorityPrefix.size();
	auto authorityEnd = url.indexOf(QLatin1Char('/'), authorityStart);
	if (authorityEnd < 0)
	{
		authorityEnd = url.size();
	}
	const auto authority = url.mid(authorityStart, authorityEnd - authorityStart);
	if (authority.contains(QLatin1Char('@')) || authority.contains(QLatin1Char('%')))
	{
		return {};
	}

	QString rawHost;
	QString portPart;
	const auto isBracketed = authority.startsWith(QLatin1Char('['));
	if (isBracketed)
	{
		const auto bracketEnd = authority.indexOf(QLatin1Char(']'));
		if (bracketEnd < 0)
		{
			return {};
		}
		rawHost = authority.mid(1, bracketEnd - 1);
		portPart = authority.mid(bracketEnd + 1);
	}
	else
	{
		const auto portStart = authority.indexOf(QLatin1Char(':'));
		rawHost = portStart < 0 ? authority : authority.left(portStart);
		portPart = portStart < 0 ? QString{} : authority.mid(portStart);
	}

	// ":<1..65535>" without leading zeros, or nothing
	int port = -1;
	if (portPart.isEmpty() == false)
	{
		bool isNumber = false;
		port = portPart.mid(1).toInt(&isNumber);
		if (portPart.startsWith(QLatin1Char(':')) == false || isNumber == false || port < 1 || port > 65535 ||
			portPart.mid(1) != QString::number(port))
		{
			return {};
		}
	}

	if (rawHost.isEmpty() || rawHost.endsWith(QLatin1Char('.')))
	{
		return {};
	}

	// IP addresses only in canonical form (no "0x7f.1", "127.1", "010.0.0.1")
	QHostAddress address;
	QString host;
	if (address.setAddress(rawHost))
	{
		const auto isIpv6 = address.protocol() == QAbstractSocket::IPv6Protocol;
		if (isIpv6 != isBracketed || (isIpv6 == false && rawHost != address.toString()))
		{
			return {};
		}
		host = address.toString();
	}
	else if (isBracketed == false)
	{
		host = TlsPeerVerifier::normalizedHost(rawHost);
	}

	// the host must be the one QUrl (and therefore the HTTP client) sees
	if (host.isEmpty() || TlsPeerVerifier::normalizedHost(parsedUrl.host()) != host ||
		parsedUrl.port() != port)
	{
		return {};
	}

	const auto hostPart = address.protocol() == QAbstractSocket::IPv6Protocol ? QStringLiteral("[%1]").arg(host) : host;
	if (port > 0)
	{
		return QStringLiteral("https://%1:%2").arg(hostPart).arg(port);
	}
	return QStringLiteral("https://%1").arg(hostPart);
}



bool TrustBundle::isValidEnrollmentToken(VeyonCrypto::ByteView token)
{
	if (token.isEmpty() || token.size() > MaximumTokenSize)
	{
		return false;
	}

	// JWS compact serialisation: three non-empty base64url parts separated by dots
	int separatorCount = 0;
	bool partIsEmpty = true;
	for (qsizetype index = 0; index < token.size(); ++index)
	{
		const auto character = token.data()[index];
		if (character == '.')
		{
			if (partIsEmpty)
			{
				return false;
			}
			++separatorCount;
			partIsEmpty = true;
		}
		else if ((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
				 (character >= '0' && character <= '9') || character == '-' || character == '_')
		{
			partIsEmpty = false;
		}
		else
		{
			return false;
		}
	}

	return separatorCount == 2 && partIsEmpty == false;
}
