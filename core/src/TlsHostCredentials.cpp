// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFile>

#include "Certificate.h"
#include "PrivateKey.h"
#include "SecretStore.h"
#include "TlsHostCredentials.h"
#include "VeyonConfiguration.h"


TlsHostCredentials::Error TlsHostCredentials::check(const Veyon::Crypto::SecureBuffer& privateKeyDer,
													const QByteArray& certificateChainPem, const QDateTime& now)
{
	if (privateKeyDer.isEmpty())
	{
		return Error::MissingKey;
	}

	const auto privateKey = Veyon::Crypto::PrivateKey::fromPkcs8Der(privateKeyDer);
	if (privateKey.has_value() == false)
	{
		return Error::InvalidKey;
	}

	// QSslKey supports no Ed25519 keys (reserved for 5.0, 06-pki.md §9.4)
	if (privateKey->algorithm() != Veyon::Crypto::KeyAlgorithm::EcP256 &&
		privateKey->algorithm() != Veyon::Crypto::KeyAlgorithm::Rsa)
	{
		return Error::UnsupportedKeyAlgorithm;
	}

	if (certificateChainPem.trimmed().isEmpty())
	{
		return Error::MissingCertificate;
	}

	const auto chain = Veyon::Crypto::Certificate::chainFromPem(certificateChainPem);
	if (chain.has_value() == false || chain->isEmpty())
	{
		return Error::InvalidCertificate;
	}

	const auto& certificate = chain->first();
	if (certificate.publicKey().isNull() || certificate.publicKey() != privateKey->publicKey())
	{
		return Error::KeyMismatch;
	}

	if (now < certificate.notBefore().addSecs(-NotYetValidToleranceSeconds))
	{
		return Error::NotYetValid;
	}

	if (now > certificate.notAfter())
	{
		return Error::Expired;
	}

	return Error::None;
}



bool TlsHostCredentials::isUsable(Error error)
{
	return error == Error::None || error == Error::NotYetValid || error == Error::Expired;
}



QString TlsHostCredentials::errorString(Error error)
{
	switch (error)
	{
	case Error::None: return {};
	case Error::MissingKey: return QStringLiteral("no host key");
	case Error::InvalidKey: return QStringLiteral("invalid host key");
	case Error::UnsupportedKeyAlgorithm: return QStringLiteral("unsupported host key algorithm");
	case Error::MissingCertificate: return QStringLiteral("no host certificate");
	case Error::InvalidCertificate: return QStringLiteral("invalid host certificate");
	case Error::KeyMismatch: return QStringLiteral("host certificate does not match the host key");
	case Error::NotYetValid: return QStringLiteral("host certificate not yet valid");
	case Error::Expired: return QStringLiteral("host certificate expired");
	}

	return {};
}



std::optional<TlsHostCredentials::Encoded> TlsHostCredentials::load(const VeyonConfiguration& configuration,
																	const QDateTime& now)
{
	auto privateKeyDer = SecretStore::read(PrivateKeySecret);
	if (privateKeyDer.has_value() == false || privateKeyDer->isEmpty())
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(Error::MissingKey)) << PrivateKeySecret;
		return std::nullopt;
	}

	const auto certificateFilePath = TlsHostCredentials::certificateFilePath(configuration);
	QFile certificateFile(certificateFilePath);
	if (certificateFile.open(QFile::ReadOnly) == false)
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(Error::MissingCertificate)) << certificateFilePath;
		return std::nullopt;
	}

	const auto certificateChainPem = certificateFile.read(Veyon::Crypto::Certificate::MaximumPemSize + 1);
	if (certificateChainPem.size() > Veyon::Crypto::Certificate::MaximumPemSize)
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(Error::InvalidCertificate)) << certificateFilePath;
		return std::nullopt;
	}

	const auto error = check(*privateKeyDer, certificateChainPem, now);
	if (isUsable(error) == false)
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(error)) << certificateFilePath;
		return std::nullopt;
	}

	const auto certificate = Veyon::Crypto::Certificate::chainFromPem(certificateChainPem)->first();
	if (error == Error::Expired)
	{
		vCritical() << "tls: host certificate" << certificateFilePath << "expired on"
					<< certificate.notAfter().toString(Qt::ISODate) << "- still offered";
	}
	else if (error == Error::NotYetValid)
	{
		vCritical() << "tls: host certificate" << certificateFilePath << "not valid before"
					<< certificate.notBefore().toString(Qt::ISODate) << "- still offered";
	}
	else if (now.addDays(ExpiryWarningDays) >= certificate.notAfter())
	{
		vWarning() << "tls: host certificate" << certificateFilePath << "expires on"
				   << certificate.notAfter().toString(Qt::ISODate);
	}

	return Encoded{std::move(*privateKeyDer), certificateChainPem};
}



QString TlsHostCredentials::certificateFilePath(const VeyonConfiguration& configuration)
{
	const auto configuredPath = configuration.tlsHostCertificateFile();
	if (configuredPath.isEmpty())
	{
		return defaultCertificateFilePath();
	}

	return configuredPath;
}



std::optional<TlsHostCredentials> TlsHostCredentials::fromEncoded(const Encoded& encoded, const QDateTime& now)
{
	const auto error = check(encoded.privateKeyDer, encoded.certificateChainPem, now);
	if (isUsable(error) == false)
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(error));
		return std::nullopt;
	}
	if (error != Error::None)
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(error)) << "- still offered";
	}

	const auto privateKey = Veyon::Crypto::PrivateKey::fromPkcs8Der(encoded.privateKeyDer);
	const auto privateKeyPem = privateKey->toPkcs8Pem();
	if (privateKeyPem.has_value() == false)
	{
		vWarning() << "tls:" << qUtf8Printable(errorString(Error::InvalidKey));
		return std::nullopt;
	}

	// QSslKey keeps its own copy (OpenSSL key object), only this temporary copy can be wiped
	auto unprotectedPem = privateKeyPem->toUnprotectedByteArray();
	TlsHostCredentials credentials;
	credentials.m_privateKey = QSslKey(unprotectedPem,
									   privateKey->algorithm() == Veyon::Crypto::KeyAlgorithm::Rsa ? QSsl::Rsa : QSsl::Ec,
									   QSsl::Pem, QSsl::PrivateKey);
	unprotectedPem.fill(0);

	const auto chain = Veyon::Crypto::Certificate::chainFromPem(encoded.certificateChainPem);
	for (const auto& certificate : *chain)
	{
		credentials.m_certificateChain.append(certificate.toQSslCertificate());
	}

	if (credentials.m_privateKey.isNull() || credentials.m_certificateChain.first().isNull())
	{
		vWarning() << "tls: host credentials not accepted by the TLS backend";
		return std::nullopt;
	}

	return credentials;
}



std::optional<TlsHostCredentials> TlsHostCredentials::fromServiceResponse(const IpcMessage::Response& response,
																		  const QDateTime& now)
{
	if (response.ok == false)
	{
		vWarning() << "tls: no host key from the service:" << response.error;
		return std::nullopt;
	}

	const auto encodedKey = response.result.value(IpcProtocol::Key::TlsKey);
	const auto certificateChain = response.result.value(IpcProtocol::Key::CertificateChain);
	if (encodedKey.isString() == false || certificateChain.isString() == false)
	{
		vWarning() << "tls: malformed host key response";
		return std::nullopt;
	}

	auto decodedKey = QByteArray::fromBase64Encoding(encodedKey.toString().toLatin1(),
													 QByteArray::AbortOnBase64DecodingErrors);
	Encoded encoded{Veyon::Crypto::SecureBuffer::fromByteArray(decodedKey.decoded),
					certificateChain.toString().toUtf8()};
	decodedKey.decoded.fill(0);

	if (decodedKey.decodingStatus != QByteArray::Base64DecodingStatus::Ok)
	{
		vWarning() << "tls: malformed host key response";
		return std::nullopt;
	}

	return fromEncoded(encoded, now);
}
