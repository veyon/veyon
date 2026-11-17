// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QSslCertificate>

#include "PublicKey.h"

namespace VeyonCrypto {

// X.509 certificate (read-only model).
//
// Parsing is meant for untrusted input: size-limited (per certificate, also inside PEM) and fail
// closed: a certificate is rejected if one of the extensions this class reports (subjectAltName,
// extendedKeyUsage, basicConstraints, keyUsage) is duplicated or cannot be decoded, if a DNS name
// is not plain ASCII without NUL bytes or an IP address has an invalid length, or if the validity
// cannot be decoded. Parsing does not mean trusting: chain, name, pin and revocation checks are the
// job of the verifier (PkiPublicApi, 05-tls.md §5).
//
// Encoding checks (PEM content goes through fromDer()): the whole input must be consumed and equal
// OpenSSL's re-encoding of the decoded certificate, which rejects BER forms and explicitly encoded
// DEFAULT values in the certificate structure itself; the SubjectPublicKeyInfo must be canonical
// (see PublicKey::fromDer()), extensions must mark criticality as DER does, and isSignedBy() only
// accepts the one encoding of each signature AlgorithmIdentifier. This is NOT a full DER check:
// OpenSSL keeps names and extension values as read, so BER forms inside them are accepted, and
// ECDSA signatures are malleable (r, n - s verifies as well). Consumers must therefore never use the
// equality of DER encodings or certificate fingerprints as identity: identity is the SPKI pin
// (spkiSha256()) or the verification of the chain.
//
// Immutable, implicitly shared and safe to use from several threads.
class VEYON_CRYPTO_EXPORT Certificate
{
public:
	static constexpr qsizetype MaximumEncodedSize = 64 * 1024;
	static constexpr qsizetype MaximumPemSize = 1024 * 1024;
	static constexpr int MaximumChainLength = 16;

	enum class ExtendedKeyUsage {
		ServerAuth,
		ClientAuth,
	};

	enum class KeyUsage {
		DigitalSignature,
		KeyEncipherment,
		KeyCertSign,
		CrlSign,
	};

	Certificate();
	~Certificate();
	Certificate(const Certificate& other);
	Certificate(Certificate&& other) noexcept;
	Certificate& operator=(const Certificate& other);
	Certificate& operator=(Certificate&& other) noexcept;

	[[nodiscard]] static std::optional<Certificate> fromDer(ByteView der);
	// exactly one PEM "CERTIFICATE" (or legacy "X509 CERTIFICATE") block (blocks with other labels
	// are skipped, blocks with RFC 1421 headers are rejected)
	[[nodiscard]] static std::optional<Certificate> fromPem(ByteView pem);
	// one or more "CERTIFICATE" blocks (1..MaximumChainLength) in the given order; fails if any
	// block cannot be parsed
	[[nodiscard]] static std::optional<QList<Certificate>> chainFromPem(ByteView pem);

	bool isNull() const;

	QByteArray toDer() const;
	QByteArray toPem() const;
	QSslCertificate toQSslCertificate() const;

	// Display-safe RFC 2253 representations (control, format and line/paragraph separator
	// characters escaped as a backslash, "u" and four hex digits) and the subject's common name,
	// which is empty if missing, ambiguous (several CN attributes), longer than 64 characters or
	// containing such characters. Attacker-controlled for foreign certificates: display/log only.
	QString subject() const;
	QString issuer() const;
	QString subjectCommonName() const;

	// big-endian serial number without leading zero bytes and its lower-case hex form (two digits
	// per byte, never empty); certificates with a zero, negative or longer than 20 octets serial
	// number (RFC 5280 §4.1.2.2) are rejected by the parsers
	QByteArray serialNumber() const;
	QString serialNumberHex() const;

	// UTC
	QDateTime notBefore() const;
	QDateTime notAfter() const;
	[[nodiscard]] bool isValidAt(const QDateTime& time) const;

	// subjectAltName entries as encoded (DNS names are not case-normalised)
	QStringList dnsNames() const;
	QList<QHostAddress> ipAddresses() const;

	// without an extendedKeyUsage extension hasExtendedKeyUsage() returns false for every usage
	bool hasExtendedKeyUsageExtension() const;
	bool hasExtendedKeyUsage(ExtendedKeyUsage usage) const;

	// without a keyUsage extension hasKeyUsage() returns false for every usage
	bool hasKeyUsageExtension() const;
	bool hasKeyUsage(KeyUsage usage) const;

	// OIDs (dotted) of all critical extensions, and whether one of them is not modelled by this
	// class (anything but basicConstraints, keyUsage, extendedKeyUsage, subjectAltName, e.g.
	// nameConstraints or unknown ones). Verifiers that rely on this class instead of a full chain
	// validation (e.g. pins, 05-tls.md §5.1) must reject certificates for which it returns true
	// (RFC 5280 §4.2). True for a null certificate.
	QStringList criticalExtensions() const;
	[[nodiscard]] bool hasUnsupportedCriticalExtension() const;

	// basicConstraints cA flag; path length constraint or -1 if none
	bool isCa() const;
	int pathLengthConstraint() const;

	// null if the key is not accepted by the VeyonCrypto key policy (see KeyAlgorithm)
	PublicKey publicKey() const;

	// The SPKI pin: SHA-256 over the DER SubjectPublicKeyInfo exactly as contained in the
	// certificate (RFC 7469) and its "sha256:<lower-case hex>" form; available for every key type.
	// This is the only pin definition: TLS verification (05-tls.md §5.1, WP7) and every other
	// consumer must compute pins with Certificate::fromDer(peerCertificate.toDer())->spkiSha256(),
	// never from QSslCertificate::publicKey().toDer(), which re-encodes the key (e.g. compressed
	// EC points) and can yield a different value.
	QByteArray spkiSha256() const;
	QString spkiFingerprint() const;

	// verifies the certificate's signature with issuerKey (no other checks)
	[[nodiscard]] bool isSignedBy(const PublicKey& issuerKey) const;

	// compares the DER encoding (byte equality, not identity, see above); null certificates are
	// never equal
	bool operator==(const Certificate& other) const;
	bool operator!=(const Certificate& other) const
	{
		return (*this == other) == false;
	}

private:
	friend struct Internal::Access;
	class Private;
	explicit Certificate(std::shared_ptr<const Private> d);

	std::shared_ptr<const Private> d;
};

} // namespace VeyonCrypto
