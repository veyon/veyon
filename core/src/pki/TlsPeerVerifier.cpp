// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>
#include <QUrl>
#include <QVector>

#include "TlsPeerVerifier.h"

#include <algorithm>
#include <memory>
#include <optional>


TlsPeerVerifier::TlsPeerVerifier(const TlsClientPolicy& policy) :
	m_policy(policy)
{
	// policies built in code may contain serials in any accepted notation
	QStringList revokedSerials;
	for (const auto& serial : std::as_const(m_policy.revokedSerials))
	{
		if (serial.trimmed().isEmpty())
		{
			continue;
		}
		const auto normalizedSerial = TlsClientPolicy::normalizedSerialNumber(serial);
		if (normalizedSerial.isEmpty())
		{
			m_policy.hasInvalidRevokedSerials = true;
		}
		else
		{
			revokedSerials.append(normalizedSerial);
		}
	}
	m_policy.revokedSerials = revokedSerials;
}



void TlsPeerVerifier::configure(QSslConfiguration& configuration) const
{
	QList<QSslCertificate> caCertificates;
	caCertificates.reserve(m_policy.caCertificates.size());
	for (const auto& caCertificate : m_policy.caCertificates)
	{
		caCertificates.append(caCertificate.toQSslCertificate());
	}

	// replaces the default (system) CA certificates and disables loading them on demand
	configuration.setCaCertificates(caCertificates);
	configuration.setPeerVerifyMode(m_policy.hasCaCertificates() ? QSslSocket::VerifyPeer : QSslSocket::VerifyNone);
}



void TlsPeerVerifier::attach(QSslSocket* socket, const Target& target, const ResultHandler& resultHandler) const
{
	auto configuration = socket->sslConfiguration();
	configure(configuration);
	socket->setSslConfiguration(configuration);

	struct State
	{
		TlsPeerVerifier verifier;
		Target target;
		ResultHandler resultHandler;
		std::optional<Result> result = {};
		bool reported = false;
	};
	const auto state = std::make_shared<State>(State{*this, target, resultHandler});

	const auto report = [state](const Result& result) {
		if (state->reported == false)
		{
			state->reported = true;
			state->resultHandler(result);
		}
	};

	const auto reject = [socket, report](const Result& result) {
		// not inside the signal emission of the TLS backend; errors that are not ignored abort the
		// handshake anyway, this also covers VerifyNone (pin-only) connections
		QMetaObject::invokeMethod(socket, [socket]() { socket->abort(); }, Qt::QueuedConnection);
		report(result);
	};

	QObject::connect(socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), socket,
					 [socket, state, reject](const QList<QSslError>& errors) {
		if (state->result.has_value())
		{
			return;
		}
		state->result = state->verifier.verify(socket->peerCertificateChain(), errors, state->target,
											   QDateTime::currentDateTimeUtc());
		if (state->result->status == Status::Verified)
		{
			socket->ignoreSslErrors(errors);
		}
		else if (state->result->status == Status::Rejected)
		{
			reject(*state->result);
		}
	});

	QObject::connect(socket, &QSslSocket::encrypted, socket, [socket, state, report, reject]() {
		if (state->result.has_value() == false)
		{
			state->result = state->verifier.verify(socket->peerCertificateChain(), {}, state->target,
												   QDateTime::currentDateTimeUtc());
		}
		if (state->result->status == Status::Rejected)
		{
			reject(*state->result);
		}
		else
		{
			report(*state->result);
		}
	});

	QObject::connect(socket, &QAbstractSocket::errorOccurred, socket, [socket, report]() {
		report(rejected(TlsError::HandshakeFailed, socket->errorString()));
	});
}



TlsPeerVerifier::Result TlsPeerVerifier::verify(const QList<QSslCertificate>& peerCertificateChain,
												const QList<QSslError>& sslErrors,
												const Target& target,
												const QDateTime& currentTime) const
{
	if (peerCertificateChain.isEmpty() || peerCertificateChain.first().isNull())
	{
		return rejected(TlsError::HandshakeFailed, QStringLiteral("no peer certificate"));
	}

	// only parsed certificates take part in the decision; additional certificates sent by the peer
	// that cannot be parsed are never trusted and therefore skipped
	QList<Veyon::Crypto::Certificate> chain;
	const auto leaf = Veyon::Crypto::Certificate::fromDer(peerCertificateChain.first().toDer());
	if (leaf.has_value())
	{
		chain.append(*leaf);
		const auto chainLength = std::min<qsizetype>(peerCertificateChain.size(), qsizetype(Veyon::Crypto::Certificate::MaximumChainLength));
		for (qsizetype index = 1; index < chainLength; ++index)
		{
			const auto certificate = Veyon::Crypto::Certificate::fromDer(peerCertificateChain.at(index).toDer());
			if (certificate.has_value())
			{
				chain.append(*certificate);
			}
		}
	}

	const auto normalizedTarget = normalizedHost(target.host);
	const auto isIpTarget = normalizedTarget.isEmpty() == false && QHostAddress(normalizedTarget).isNull() == false;
	const auto normalizedDirectoryHostName = isIpTarget ? normalizedHost(target.directoryHostName) : QString{};

	QList<QByteArray> applicablePins;
	for (const auto& pin : m_policy.pins)
	{
		if (pin.host.isEmpty() ||
			(normalizedTarget.isEmpty() == false && pin.host == normalizedTarget) ||
			(normalizedDirectoryHostName.isEmpty() == false && pin.host == normalizedDirectoryHostName))
		{
			applicablePins.append(pin.spkiSha256);
		}
	}

	if (applicablePins.isEmpty() == false)
	{
		// pins replace all other checks, so certificates with critical extensions this
		// verifier does not evaluate can never match (RFC 5280 §4.2)
		if (leaf.has_value() && leaf->hasUnsupportedCriticalExtension() == false && leaf->isCa() == false &&
			applicablePins.contains(leaf->spkiSha256()))
		{
			return {Status::Verified, TlsError::None, 0, QStringLiteral("pinned key %1").arg(leaf->spkiFingerprint())};
		}

		auto pinMismatchReason = QStringLiteral("invalid peer certificate");
		if (leaf.has_value() && leaf->hasUnsupportedCriticalExtension())
		{
			pinMismatchReason = QStringLiteral("certificate with key %1 has an unsupported critical extension")
									.arg(leaf->spkiFingerprint());
		}
		else if (leaf.has_value() && leaf->isCa())
		{
			pinMismatchReason = QStringLiteral("certificate with key %1 is a CA certificate").arg(leaf->spkiFingerprint());
		}
		else if (leaf.has_value())
		{
			pinMismatchReason = QStringLiteral("key %1 does not match the pins for the target").arg(leaf->spkiFingerprint());
		}
		if (m_policy.hasCaCertificates() == false)
		{
			return rejected(TlsError::PinMismatch, pinMismatchReason);
		}

		auto result = verifyCaPath(chain, sslErrors, target, normalizedTarget, currentTime);
		result.reason = pinMismatchReason + QStringLiteral(", ") + result.reason;
		return result;
	}

	if (m_policy.hasCaCertificates())
	{
		return verifyCaPath(chain, sslErrors, target, normalizedTarget, currentTime);
	}

	if (m_policy.hasPins())
	{
		return rejected(TlsError::PinMismatch, QStringLiteral("no pin for the target"));
	}

	return {Status::Unverified, TlsError::None, 0, QStringLiteral("no trust anchor configured")};
}



QString TlsPeerVerifier::normalizedHost(const QString& host)
{
	auto name = host;
	const auto isBracketed = name.startsWith(QLatin1Char('[')) && name.endsWith(QLatin1Char(']'));
	if (isBracketed)
	{
		name = name.mid(1, name.size() - 2);
	}

	QHostAddress address;
	if (address.setAddress(name))
	{
		if (isBracketed && address.protocol() != QAbstractSocket::IPv6Protocol)
		{
			return {};
		}
		address.setScopeId({});
		return address.toString();
	}

	if (isBracketed)
	{
		return {};
	}

	if (name.endsWith(QLatin1Char('.')))
	{
		name.chop(1);
	}
	if (name.isEmpty())
	{
		return {};
	}

	const auto asciiName = QString::fromLatin1(QUrl::toAce(name)).toLower();
	static constexpr auto MaximumNameLength = 253;
	static constexpr auto MaximumLabelLength = 63;
	if (asciiName.isEmpty() || asciiName.size() > MaximumNameLength)
	{
		return {};
	}

	// names that only look like DNS names (trailing dot, full-width digits) must not compare
	// equal to an IP literal target or pin
	if (QHostAddress(asciiName).isNull() == false)
	{
		return {};
	}

	const auto labels = asciiName.split(QLatin1Char('.'));
	for (const auto& label : labels)
	{
		if (label.isEmpty() || label.size() > MaximumLabelLength)
		{
			return {};
		}
		for (const auto character : label)
		{
			if ((character < QLatin1Char('a') || character > QLatin1Char('z')) &&
				(character < QLatin1Char('0') || character > QLatin1Char('9')) &&
				character != QLatin1Char('-') && character != QLatin1Char('_'))
			{
				return {};
			}
		}
	}

	return asciiName;
}



bool TlsPeerVerifier::dnsNameMatches(const QString& subjectAltName, const QString& normalizedDnsName)
{
	auto pattern = subjectAltName.toLower();
	if (pattern.endsWith(QLatin1Char('.')))
	{
		pattern.chop(1);
	}
	if (pattern.isEmpty() || normalizedDnsName.isEmpty())
	{
		return false;
	}

	if (pattern.contains(QLatin1Char('*')) == false)
	{
		return pattern == normalizedDnsName;
	}

	// only "*.<at least two labels>", no partial wildcards such as "w*.example.org"
	static const auto wildcardPrefix = QStringLiteral("*.");
	const auto suffix = pattern.mid(wildcardPrefix.size());
	if (pattern.startsWith(wildcardPrefix) == false ||
		suffix.contains(QLatin1Char('*')) ||
		suffix.contains(QLatin1Char('.')) == false ||
		suffix.startsWith(QLatin1Char('.')) || suffix.endsWith(QLatin1Char('.')) ||
		suffix.contains(QStringLiteral("..")))
	{
		return false;
	}

	const auto firstLabelEnd = normalizedDnsName.indexOf(QLatin1Char('.'));
	return firstLabelEnd > 0 && normalizedDnsName.mid(firstLabelEnd + 1) == suffix;
}



TlsPeerVerifier::Result TlsPeerVerifier::verifyCaPath(const QList<Veyon::Crypto::Certificate>& chain,
													  const QList<QSslError>& sslErrors,
													  const Target& target, const QString& normalizedTarget,
													  const QDateTime& currentTime) const
{
	if (chain.isEmpty())
	{
		return rejected(TlsError::UntrustedCa, QStringLiteral("invalid peer certificate"));
	}
	const auto& leaf = chain.first();

	// Qt/OpenSSL chain verification (only the policy's CAs, see configure()); time errors are
	// re-evaluated below with the clock tolerance, host names by the explicit SAN check
	for (const auto& sslError : sslErrors)
	{
		switch (sslError.error())
		{
		case QSslError::NoError:
		case QSslError::HostNameMismatch:
		case QSslError::CertificateExpired:
		case QSslError::CertificateNotYetValid:
			break;
		case QSslError::CertificateRevoked:
			return rejected(TlsError::Revoked, sslError.errorString());
		default:
			return rejected(TlsError::UntrustedCa, sslError.errorString());
		}
	}

	if (m_policy.hasInvalidRevokedSerials)
	{
		return rejected(TlsError::UntrustedCa, QStringLiteral("TLS/RevokedSerials contains invalid entries"));
	}

	// a server certificate must be an end-entity certificate
	if (leaf.isCa())
	{
		const auto reason = QStringLiteral("certificate %1 is a CA certificate");
		return rejected(TlsError::UntrustedCa, reason.arg(leaf.serialNumberHex()));
	}

	// independent verification of the path, so that the decision never depends on the socket
	// having been configured and its errors having been passed correctly
	const auto path = buildPath(chain, currentTime);
	if (path.isEmpty())
	{
		const auto reason = QStringLiteral("no valid path from certificate %1 to a configured CA");
		return rejected(TlsError::UntrustedCa, reason.arg(leaf.serialNumberHex()));
	}

	for (qsizetype index = 0; index < path.size() - 1; ++index)
	{
		const auto serial = TlsClientPolicy::normalizedSerialNumber(path.at(index).serialNumberHex());
		if (m_policy.revokedSerials.contains(serial))
		{
			return rejected(TlsError::Revoked, QStringLiteral("certificate serial %1 is revoked").arg(serial));
		}
	}

	for (const auto& certificate : path)
	{
		const auto validity = checkValidity(certificate, currentTime);
		if (validity.error != TlsError::None)
		{
			return validity;
		}
	}

	if (leaf.hasExtendedKeyUsage(Veyon::Crypto::Certificate::ExtendedKeyUsage::ServerAuth) == false)
	{
		const auto reason = QStringLiteral("certificate %1 has no extendedKeyUsage serverAuth");
		return rejected(TlsError::UntrustedCa, reason.arg(leaf.serialNumberHex()));
	}

	if (matchesName(leaf, target, normalizedTarget) == false)
	{
		const auto reason = QStringLiteral("certificate %1 does not match the target");
		return rejected(TlsError::NameMismatch, reason.arg(leaf.serialNumberHex()));
	}

	const auto reason = QStringLiteral("certificate %1 issued by configured CA %2");
	return {Status::Verified, TlsError::None, 0, reason.arg(leaf.serialNumberHex(), path.last().spkiFingerprint())};
}



QList<Veyon::Crypto::Certificate> TlsPeerVerifier::buildPath(const QList<Veyon::Crypto::Certificate>& chain,
															 const QDateTime& currentTime) const
{
	const auto isIssuer = [](const Veyon::Crypto::Certificate& issuer, const Veyon::Crypto::Certificate& subject, int intermediateCount) {
		return TlsClientPolicy::isAcceptableCaCertificate(issuer) &&
			(issuer.pathLengthConstraint() < 0 || intermediateCount <= issuer.pathLengthConstraint()) &&
			issuer.subject() == subject.issuer() &&
			subject.isSignedBy(issuer.publicKey());
	};

	QList<Veyon::Crypto::Certificate> path{chain.first()};
	QVector<bool> used(chain.size(), false);
	used[0] = true;

	while (path.size() <= Veyon::Crypto::Certificate::MaximumChainLength)
	{
		const auto& current = path.last();
		if (current.hasUnsupportedCriticalExtension())
		{
			return {};
		}

		// among matching issuers (e.g. a renewed CA certificate with the same subject and key)
		// prefer currently valid ones, so that an expired one listed first does not fail the path
		const auto intermediateCount = int(path.size() - 1);
		bool foundIntermediate = false;
		for (const auto requireValidity : {true, false})
		{
			const auto isUsable = [&](const Veyon::Crypto::Certificate& issuer) {
				return (requireValidity == false || checkValidity(issuer, currentTime).error == TlsError::None) &&
					isIssuer(issuer, current, intermediateCount);
			};

			for (const auto& caCertificate : m_policy.caCertificates)
			{
				if (isUsable(caCertificate))
				{
					path.append(caCertificate);
					return path;
				}
			}

			for (qsizetype index = 1; index < chain.size(); ++index)
			{
				if (used.at(index) == false && isUsable(chain.at(index)))
				{
					used[index] = true;
					path.append(chain.at(index));
					foundIntermediate = true;
					break;
				}
			}

			if (foundIntermediate)
			{
				break;
			}
		}

		if (foundIntermediate == false)
		{
			return {};
		}
	}

	return {};
}



bool TlsPeerVerifier::matchesName(const Veyon::Crypto::Certificate& certificate, const Target& target,
								  const QString& normalizedTarget) const
{
	if (normalizedTarget.isEmpty())
	{
		return false;
	}

	const QHostAddress targetAddress(normalizedTarget);
	if (targetAddress.isNull())
	{
		const auto dnsNames = certificate.dnsNames();
		return std::any_of(dnsNames.cbegin(), dnsNames.cend(), [&](const QString& dnsName) {
			return dnsNameMatches(dnsName, normalizedTarget);
		});
	}

	const auto ipAddresses = certificate.ipAddresses();
	if (std::any_of(ipAddresses.cbegin(), ipAddresses.cend(), [&](const QHostAddress& address) {
		return address == targetAddress;
	}))
	{
		return true;
	}

	// host names associated with the IP target by trusted sources must match a DNS name exactly
	// (no wildcard); the reverse DNS policy extends the directory policy
	QStringList acceptedHostNames;
	switch (m_policy.ipTargetPolicy)
	{
	case VeyonCore::TlsIpTargetPolicy::RequireIpSan:
		break;
	case VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithReverseDnsHostname:
		acceptedHostNames.append(normalizedHost(target.confirmedReverseDnsHostName));
		Q_FALLTHROUGH();
	case VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithHostname:
		acceptedHostNames.append(normalizedHost(target.directoryHostName));
		break;
	}

	const auto dnsNames = certificate.dnsNames();
	for (const auto& hostName : std::as_const(acceptedHostNames))
	{
		// only names, never IP literals (those must match an IP address entry)
		if (hostName.isEmpty() || QHostAddress(hostName).isNull() == false)
		{
			continue;
		}
		for (const auto& dnsName : dnsNames)
		{
			if (dnsName.contains(QLatin1Char('*')) == false && dnsNameMatches(dnsName, hostName))
			{
				return true;
			}
		}
	}

	return false;
}



TlsPeerVerifier::Result TlsPeerVerifier::checkValidity(const Veyon::Crypto::Certificate& certificate,
													   const QDateTime& currentTime)
{
	const auto notBefore = certificate.notBefore();
	const auto notAfter = certificate.notAfter();
	if (notBefore.isValid() == false || notAfter.isValid() == false || currentTime.isValid() == false)
	{
		return rejected(TlsError::UntrustedCa, QStringLiteral("invalid validity period"));
	}

	const auto rejectTime = [&](qint64 deviationSeconds, TlsError error) {
		const auto reason = QStringLiteral("certificate %1 is outside its validity period by %2 s")
								.arg(certificate.serialNumberHex())
								.arg(deviationSeconds);
		if (deviationSeconds <= ClockSkewReportLimitSeconds)
		{
			auto result = rejected(TlsError::ClockSkew, reason);
			result.clockSkewMinutes = int((deviationSeconds + 59) / 60);
			return result;
		}
		return rejected(error, reason);
	};

	if (currentTime.secsTo(notBefore) > ClockToleranceSeconds)
	{
		return rejectTime(currentTime.secsTo(notBefore), TlsError::NotYetValid);
	}

	if (notAfter.secsTo(currentTime) > ClockToleranceSeconds)
	{
		return rejectTime(notAfter.secsTo(currentTime), TlsError::Expired);
	}

	return {Status::Verified, TlsError::None, 0, {}};
}



TlsPeerVerifier::Result TlsPeerVerifier::rejected(TlsError error, const QString& reason)
{
	return {Status::Rejected, error, 0, reason};
}
