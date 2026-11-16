// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QRegularExpression>

#include "TlsClientPolicy.h"
#include "TlsConfiguration.h"
#include "TlsPeerVerifier.h"
#include "VeyonConfiguration.h"
#include "crypto/Hash.h"


// non-empty and [0-9a-f] only
static bool isLowerCaseHex(const QString& string)
{
	if (string.isEmpty())
	{
		return false;
	}

	for (const auto character : string)
	{
		if ((character < QLatin1Char('0') || character > QLatin1Char('9')) &&
			(character < QLatin1Char('a') || character > QLatin1Char('f')))
		{
			return false;
		}
	}

	return true;
}


TlsClientPolicy TlsClientPolicy::fromConfiguration(const VeyonConfiguration& configuration)
{
	TlsClientPolicy policy;
	policy.mode = TlsConfiguration::clientMode(configuration);
	policy.ipTargetPolicy = TlsConfiguration::ipTargetPolicy(configuration);

	const auto caCertificatesPem = configuration.tlsCaCertificate().trimmed();
	if (caCertificatesPem.isEmpty() == false)
	{
		const auto caCertificates = parseCaCertificates(caCertificatesPem.toUtf8());
		if (caCertificates.has_value())
		{
			policy.caCertificates = *caCertificates;
		}
		else
		{
			vWarning() << "TLS/CaCertificate is invalid, verification against a CA will fail";
			policy.hasInvalidCaCertificates = true;
		}
	}

	const auto pinEntries = configuration.tlsPinnedFingerprints();
	for (int index = 0; index < pinEntries.size(); ++index)
	{
		// empty entries (e.g. an empty list stored as [""]) configure nothing
		if (pinEntries.at(index).trimmed().isEmpty())
		{
			continue;
		}
		const auto pin = parsePinEntry(pinEntries.at(index));
		if (pin.has_value())
		{
			policy.pins.append(*pin);
		}
		else
		{
			// the position identifies the entry without logging configuration content
			vWarning() << "ignoring TLS/PinnedFingerprints entry" << index
					   << "(expected <host>=sha256:<64 hex digits>)";
			policy.hasIgnoredPinEntries = true;
		}
	}

	const auto revokedSerials = configuration.tlsRevokedSerials();
	for (int index = 0; index < revokedSerials.size(); ++index)
	{
		if (revokedSerials.at(index).trimmed().isEmpty())
		{
			continue;
		}
		const auto normalizedSerial = normalizedSerialNumber(revokedSerials.at(index));
		if (normalizedSerial.isEmpty())
		{
			vWarning() << "invalid TLS/RevokedSerials entry" << index << "- verification against a CA will fail";
			policy.hasInvalidRevokedSerials = true;
		}
		else
		{
			policy.revokedSerials.append(normalizedSerial);
		}
	}

	return policy;
}



TlsClientPolicy TlsClientPolicy::fromConfiguration()
{
	return fromConfiguration(VeyonCore::config());
}



std::optional<QList<VeyonCrypto::Certificate>> TlsClientPolicy::parseCaCertificates(const QByteArray& pem)
{
	const auto certificates = VeyonCrypto::Certificate::chainFromPem(pem);
	if (certificates.has_value() == false || certificates->isEmpty())
	{
		return std::nullopt;
	}

	for (const auto& certificate : *certificates)
	{
		if (isAcceptableCaCertificate(certificate) == false)
		{
			return std::nullopt;
		}
	}

	return certificates;
}



bool TlsClientPolicy::isAcceptableCaCertificate(const VeyonCrypto::Certificate& certificate)
{
	return certificate.isNull() == false &&
		certificate.publicKey().isNull() == false &&
		certificate.isCa() &&
		(certificate.hasKeyUsageExtension() == false ||
		 certificate.hasKeyUsage(VeyonCrypto::Certificate::KeyUsage::KeyCertSign)) &&
		certificate.hasUnsupportedCriticalExtension() == false;
}



std::optional<TlsPin> TlsClientPolicy::parsePinEntry(const QString& entry)
{
	const auto parts = entry.trimmed().split(QLatin1Char('='));
	if (parts.size() != 2)
	{
		return std::nullopt;
	}

	const auto host = TlsPeerVerifier::normalizedHost(parts.at(0));
	const auto spkiSha256 = parseFingerprint(parts.at(1));
	if (host.isEmpty() || spkiSha256.has_value() == false)
	{
		return std::nullopt;
	}

	return TlsPin{host, *spkiSha256};
}



std::optional<QByteArray> TlsClientPolicy::parseFingerprint(const QString& fingerprint)
{
	static const auto prefix = QStringLiteral("sha256:");
	const auto hashHexSize = VeyonCrypto::Hash::size(VeyonCrypto::Hash::Algorithm::Sha256) * 2;

	if (fingerprint.size() != prefix.size() + hashHexSize ||
		fingerprint.startsWith(prefix) == false)
	{
		return std::nullopt;
	}

	const auto hashHex = fingerprint.mid(prefix.size()).toLower();
	if (isLowerCaseHex(hashHex) == false)
	{
		return std::nullopt;
	}

	return QByteArray::fromHex(hashHex.toLatin1());
}



QString TlsClientPolicy::normalizedSerialNumber(const QString& serialNumberHex)
{
	auto serial = serialNumberHex.toLower();
	serial.remove(QLatin1Char(':'));
	serial.remove(QRegularExpression(QStringLiteral("\\s")));
	if (serial.startsWith(QLatin1String("0x")))
	{
		serial.remove(0, 2);
	}
	if (isLowerCaseHex(serial) == false)
	{
		return {};
	}

	auto firstSignificantDigit = 0;
	while (firstSignificantDigit < serial.size() - 1 && serial.at(firstSignificantDigit) == QLatin1Char('0'))
	{
		++firstSignificantDigit;
	}

	return serial.mid(firstSignificantDigit);
}
