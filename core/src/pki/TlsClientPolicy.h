// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Certificate.h"
#include "VeyonCore.h"

class VeyonConfiguration;

// SPKI pin (05-tls.md §5.1): SHA-256 over the DER SubjectPublicKeyInfo as contained in the peer
// certificate (VeyonCrypto::Certificate::spkiSha256()).
struct TlsPin
{
	// normalised connect target (TlsPeerVerifier::normalizedHost()) the pin authenticates; empty for
	// a bare pin, which applies to every target and therefore only exists per connection, set in code
	// (demo pin 05-tls.md §8a.4, loopback own-certificate pin §4.6a), never from the configuration
	QString host;
	QByteArray spkiSha256;
};

// Verification parameters for one outgoing TLS connection (05-tls.md §4.2, §5). The default
// constructed policy has no trust anchors, i.e. verification yields an unverified result. The CA
// path always checks the name; a per-connection policy that must not depend on names (demo client,
// 05-tls.md §8a.4) consists of a bare pin and no CA certificates.
struct VEYON_CORE_EXPORT TlsClientPolicy
{
	VeyonCore::TlsClientMode mode = VeyonCore::TlsClientMode::Prefer;
	QList<VeyonCrypto::Certificate> caCertificates;
	QList<TlsPin> pins;
	VeyonCore::TlsIpTargetPolicy ipTargetPolicy = VeyonCore::TlsIpTargetPolicy::AcceptCaSignedWithHostname;
	// hex serial numbers, compared in normalised form (normalizedSerialNumber())
	QStringList revokedSerials;

	// Fail-closed markers for configured trust anchors that could not be used: a TLS/CaCertificate
	// value that is set but invalid still counts as configured CA (the CA path then always fails
	// with untrusted-ca), and ignored TLS/PinnedFingerprints entries (bare or malformed) count as
	// existing pins (a target without an applicable pin then fails with pin-mismatch). A typo in
	// the configuration therefore never turns verification into an unverified result. Invalid
	// TLS/RevokedSerials entries make the CA path fail (untrusted-ca), so that a garbled entry never
	// silently un-revokes a certificate.
	bool hasInvalidCaCertificates = false;
	bool hasIgnoredPinEntries = false;
	bool hasInvalidRevokedSerials = false;

	// TLS/ClientMode, TLS/CaCertificate, TLS/PinnedFingerprints, TLS/RevokedSerials,
	// TLS/IpTargetPolicy; invalid values are logged (one line each) and handled as described above,
	// an invalid TLS/ClientMode or TLS/IpTargetPolicy as Required or RequireIpSan
	[[nodiscard]] static TlsClientPolicy fromConfiguration(const VeyonConfiguration& configuration);
	[[nodiscard]] static TlsClientPolicy fromConfiguration();

	// TLS/CaCertificate: PEM bundle of 1..Certificate::MaximumChainLength CA certificates
	// (basicConstraints cA, keyCertSign if keyUsage is present, a key accepted by the VeyonCrypto key
	// policy, no unsupported critical extension); fails for an empty or partly invalid bundle.
	// Limitation: CA certificates of an organisation PKI with critical extensions VeyonCrypto does
	// not model (e.g. nameConstraints, policyConstraints) are not supported and rejected.
	// Other PEM blocks are skipped.
	[[nodiscard]] static std::optional<QList<VeyonCrypto::Certificate>> parseCaCertificates(const QByteArray& pem);
	[[nodiscard]] static bool isAcceptableCaCertificate(const VeyonCrypto::Certificate& certificate);

	// "<host>=sha256:<64 hex>" (host = DNS name or IP literal); fails for bare or malformed entries
	[[nodiscard]] static std::optional<TlsPin> parsePinEntry(const QString& entry);
	// "sha256:<64 hex>" (hex case-insensitive)
	[[nodiscard]] static std::optional<QByteArray> parseFingerprint(const QString& fingerprint);

	// lower-case hex without leading zeros (Certificate::serialNumberHex() and configuration
	// entries are compared in this form); case, an optional "0x" prefix, ':' separators and
	// whitespace are accepted on input; empty for invalid input
	[[nodiscard]] static QString normalizedSerialNumber(const QString& serialNumberHex);

	bool hasCaCertificates() const
	{
		return caCertificates.isEmpty() == false || hasInvalidCaCertificates;
	}

	bool hasPins() const
	{
		return pins.isEmpty() == false || hasIgnoredPinEntries;
	}

	// a CA or pins are configured (TLS/AllowCredentialsOverUnverifiedChannel=Auto, 03-secretstore-config.md §5)
	bool hasTrustAnchor() const
	{
		return hasCaCertificates() || hasPins();
	}
};
