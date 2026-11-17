// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDateTime>
#include <QSslCertificate>
#include <QSslKey>

#include "IpcMessage.h"
#include "SecureBuffer.h"

#include <optional>

class IpcClient;
class VeyonConfiguration;

// The TLS identity of this host (05-tls.md §2, 06-pki.md §3): the private key in the SecretStore
// (tls/hostKey, PKCS#8 DER) and the public certificate chain (leaf first) in
// TLS/HostCertificateFile or the default path. veyon-service loads and checks both and hands them
// to its session servers via IPC GetTlsHostKey (02-ipc.md §6), which keep them in memory only.
// Never logs key material.
class VEYON_CORE_EXPORT TlsHostCredentials
{
public:
	static constexpr auto PrivateKeySecret = QLatin1String("tls/hostKey");
	// warn about the expiry of the host certificate this many days ahead
	static constexpr int ExpiryWarningDays = 60;
	// notBefore tolerance for clocks that are slightly behind the issuing CA host (05-tls.md §5.2)
	static constexpr int NotYetValidToleranceSeconds = 5 * 60;

	enum class Error {
		None,
		MissingKey,
		InvalidKey,
		UnsupportedKeyAlgorithm,
		MissingCertificate,
		InvalidCertificate,
		KeyMismatch,
		NotYetValid,
		Expired,
	};

	// as stored and as transferred via IPC
	struct Encoded
	{
		Veyon::Crypto::SecureBuffer privateKeyDer;
		QByteArray certificateChainPem;
	};

	TlsHostCredentials() = default;

	const QList<QSslCertificate>& certificateChain() const
	{
		return m_certificateChain;
	}

	const QSslKey& privateKey() const
	{
		return m_privateKey;
	}

	// key is PKCS#8 (EC P-256 or RSA, the host key types of 06-pki.md §11), the chain 1..16 PEM
	// certificates, the key belongs to the first one, which is valid at the given time
	[[nodiscard]] static Error check(const Veyon::Crypto::SecureBuffer& privateKeyDer, const QByteArray& certificateChainPem,
									 const QDateTime& now);
	[[nodiscard]] static QString errorString(Error error);

	// service side: reads and checks tls/hostKey and the certificate file; logs why nothing was
	// returned and warns about an upcoming expiry
	[[nodiscard]] static std::optional<Encoded> load(const VeyonConfiguration& configuration,
													 const QDateTime& now = QDateTime::currentDateTimeUtc());

	// TLS/HostCertificateFile or, if empty, defaultCertificateFilePath()
	static QString certificateFilePath(const VeyonConfiguration& configuration);
	// /var/lib/veyon/tls/host-cert.pem, %ProgramData%\Veyon\tls\host-cert.pem (06-pki.md §3)
	static QString defaultCertificateFilePath();

	// checked credentials for QSslSocket
	[[nodiscard]] static std::optional<TlsHostCredentials> fromEncoded(const Encoded& encoded,
																	   const QDateTime& now = QDateTime::currentDateTimeUtc());

	// session server side: the result of GetTlsHostKey
	[[nodiscard]] static std::optional<TlsHostCredentials> fromServiceResponse(const IpcMessage::Response& response,
																			   const QDateTime& now = QDateTime::currentDateTimeUtc());
	[[nodiscard]] static std::optional<TlsHostCredentials> fetchFromService(IpcClient* serviceIpcClient);

private:
	QList<QSslCertificate> m_certificateChain;
	QSslKey m_privateKey;
};
