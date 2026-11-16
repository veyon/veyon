// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDateTime>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>

#include "TlsClientPolicy.h"
#include "TlsError.h"

#include <functional>

// Identity decision for the server side of an outgoing TLS connection (05-tls.md §5, part of
// PkiPublicApi, 06-pki.md §1). Everything it is given about the peer is hostile input.
//
// Usage by the TLS transport (WP7), requires Qt's OpenSSL backend (spike S8): call attach() for
// the client socket before connectToHostEncrypted() and do not read or write application data
// before its result handler reported an accepted result. attach() implements the only contract
// that works with QSslSocket (errors that are not ignored inside the sslErrors() signal abort the
// handshake):
// - configure() sets only the policy's CAs (never the system store) and the peer verify mode;
// - if sslErrors() is emitted, verify() runs inside that signal with QSslSocket::peerCertificateChain()
//   and exactly the reported errors; only a Verified result ignores exactly these errors;
// - without SSL errors verify() runs on encrypted(), before any application byte is read/written;
// - a rejected peer is aborted, so no application data is exchanged.
//
// Decision (05-tls.md §5.1-5.3):
// - The peer certificate must never be a CA certificate (basicConstraints cA), on both paths.
// - Pins that apply to the target (bare pins, or host-bound pins whose host equals the target or,
//   for an IP target, the directory host name): a match verifies regardless of chain, validity,
//   name and revocation (pin = identity); otherwise the CA path decides if a CA is configured,
//   else pin-mismatch.
// - No applicable pin: CA path if a CA is configured; else pin-mismatch if any pin exists (a pin of
//   host B never verifies target A); else Status::Unverified.
// - CA path: the chain must be accepted by Qt/OpenSSL (only certificate time and host name errors
//   are left to this class) and is verified again independently: path to a configured CA with
//   signature, CA and keyUsage, path length and critical extension checks; validity of every
//   certificate with a tolerance of ±5 min; leaf extendedKeyUsage serverAuth; serials not in
//   TLS/RevokedSerials; subjectAltName match against the target (never the common name).
class VEYON_CORE_EXPORT TlsPeerVerifier
{
public:
	static constexpr qint64 ClockToleranceSeconds = 5 * 60;
	// outside the tolerance but at most this far: reported as clock-skew instead of
	// expired/not-yet-valid (most likely a wrong system clock on one side)
	static constexpr qint64 ClockSkewReportLimitSeconds = 60 * 60;

	enum class Status {
		Verified,
		Unverified, // encrypted, but no trust anchor configured
		Rejected,
	};

	struct Target
	{
		// connect target as passed to the socket: DNS name or IP literal
		QString host;
		// IP targets only: the host name the directory associates with this IP address
		// (TLS/IpTargetPolicy AcceptCaSignedWithHostname and host-bound pins)
		QString directoryHostName = {};
		// IP targets only: forward-confirmed reverse DNS name of this IP address, looked up by the
		// caller (TLS/IpTargetPolicy AcceptCaSignedWithReverseDnsHostname only)
		QString confirmedReverseDnsHostName = {};
	};

	struct Result
	{
		Status status = Status::Rejected;
		TlsError error = TlsError::None;
		int clockSkewMinutes = 0;
		// one-line reason for the log (no secrets; certificate data only as fingerprints/serials)
		QString reason = {};

		bool isVerified() const
		{
			return status == Status::Verified;
		}

		// translated message for the user (master UI)
		QString message() const
		{
			return tlsErrorMessage(error, clockSkewMinutes);
		}
	};

	explicit TlsPeerVerifier(const TlsClientPolicy& policy);

	const TlsClientPolicy& policy() const
	{
		return m_policy;
	}

	// CA certificates of the policy only (not the system store) and VerifyPeer if the policy has a
	// CA, otherwise no CA certificates and VerifyNone (pin-only or unverified connection)
	void configure(QSslConfiguration& configuration) const;

	using ResultHandler = std::function<void(const Result& result)>;

	// configures the client socket and verifies the peer during its next handshake (see above);
	// resultHandler is called exactly once: for a rejected peer (the socket is aborted, also when the
	// handshake fails before a decision, with handshake-failed) or, for an accepted result
	// (Verified, Unverified), on encrypted(). The verifier may be destroyed after the call.
	void attach(QSslSocket* socket, const Target& target, const ResultHandler& resultHandler) const;

	[[nodiscard]] Result verify(const QList<QSslCertificate>& peerCertificateChain,
								const QList<QSslError>& sslErrors,
								const Target& target,
								const QDateTime& currentTime = QDateTime::currentDateTimeUtc()) const;

	// connect target / pin host in the form names are compared in: IP literals (optionally in
	// brackets) as QHostAddress::toString() without scope id, DNS names lower-case ASCII (IDNA) without
	// one trailing dot; empty for anything else (e.g. wildcards, empty labels, invalid characters,
	// names that are IP literals after this normalisation such as "192.0.2.10." or full-width digits).
	// Notes: the IPv6 scope id is stripped, so a pin or IP SAN for a link-local address applies on
	// every interface (certificates cannot carry scope ids); QUrl::toAce() implements IDNA 2003 with
	// Qt 5 and UTS #46 with Qt 6, so a few non-ASCII names (e.g. with "ß") normalise differently
	// between Qt versions - use ASCII (A-label) names in pins and the directory for such hosts.
	[[nodiscard]] static QString normalizedHost(const QString& host);

	// subjectAltName DNS entry against a normalised DNS name (no IP literal): case-insensitive,
	// one trailing dot ignored, a wildcard only as the complete left-most label of a name with at
	// least three labels, matching exactly one non-empty label
	[[nodiscard]] static bool dnsNameMatches(const QString& subjectAltName, const QString& normalizedDnsName);

private:
	Result verifyCaPath(const QList<VeyonCrypto::Certificate>& chain, const QList<QSslError>& sslErrors,
						const Target& target, const QString& normalizedTarget, const QDateTime& currentTime) const;
	// leaf first, configured CA last; empty if there is no acceptable path; issuers valid at
	// currentTime are preferred
	QList<VeyonCrypto::Certificate> buildPath(const QList<VeyonCrypto::Certificate>& chain, const QDateTime& currentTime) const;
	bool matchesName(const VeyonCrypto::Certificate& certificate, const Target& target,
					 const QString& normalizedTarget) const;
	static Result checkValidity(const VeyonCrypto::Certificate& certificate, const QDateTime& currentTime);
	static Result rejected(TlsError error, const QString& reason);

	TlsClientPolicy m_policy;
};
