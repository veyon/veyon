// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "crypto/CertificateRequest.h"

// PKCS#10 request parsers of VeyonCrypto (untrusted input of the enrollment server); the first
// input byte selects DER or PEM parsing
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	const VeyonCrypto::ByteView input(data + 1, qsizetype(size - 1));
	const auto request = (data[0] & 1) ? VeyonCrypto::CertificateRequest::fromPem(input) : VeyonCrypto::CertificateRequest::fromDer(input);
	if (request.has_value())
	{
		(void)request->toPem();
		(void)request->subject();
		(void)request->subjectCommonName();
		(void)request->publicKey().spkiFingerprint();
		(void)VeyonCrypto::CertificateRequest::fromDer(request->toDer());
	}

	return 0;
}
