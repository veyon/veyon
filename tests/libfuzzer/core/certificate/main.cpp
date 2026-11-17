// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Certificate.h"

// X.509 certificate parsers of VeyonCrypto (untrusted input: TLS peers, trust bundles,
// enrollment responses); the first input byte selects DER or PEM chain parsing
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	const Veyon::Crypto::ByteView input(data + 1, qsizetype(size - 1));

	QList<Veyon::Crypto::Certificate> certificates;
	if (data[0] & 1)
	{
		certificates = Veyon::Crypto::Certificate::chainFromPem(input).value_or(QList<Veyon::Crypto::Certificate>{});
	}
	else if (const auto certificate = Veyon::Crypto::Certificate::fromDer(input))
	{
		certificates.append(*certificate);
	}

	for (const auto& certificate : std::as_const(certificates))
	{
		// exercise every accessor and the conversions of parsed certificates
		(void)certificate.toDer();
		(void)certificate.toPem();
		(void)certificate.toQSslCertificate();
		(void)certificate.subject();
		(void)certificate.issuer();
		(void)certificate.subjectCommonName();
		(void)certificate.serialNumberHex();
		(void)certificate.isValidAt(certificate.notBefore());
		(void)certificate.dnsNames();
		(void)certificate.ipAddresses();
		(void)certificate.hasExtendedKeyUsage(Veyon::Crypto::Certificate::ExtendedKeyUsage::ServerAuth);
		(void)certificate.hasKeyUsage(Veyon::Crypto::Certificate::KeyUsage::KeyCertSign);
		(void)certificate.isCa();
		(void)certificate.criticalExtensions();
		(void)certificate.hasUnsupportedCriticalExtension();
		(void)certificate.publicKey().spkiSha256();
		(void)certificate.pathLengthConstraint();
		(void)certificate.spkiFingerprint();
		(void)certificate.isSignedBy(certificate.publicKey());
		(void)Veyon::Crypto::Certificate::fromDer(certificate.toDer());
	}

	return 0;
}
