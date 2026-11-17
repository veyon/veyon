// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pki/TlsClientPolicy.h"
#include "pki/TrustBundle.h"

#include <cstdlib>

static void discardLogMessage(QtMsgType, const QMessageLogContext&, const QString&)
{
}



// trust bundle parser of PkiPublicApi (untrusted input: files from deployment shares, installer
// parameters, 06-pki.md §4); accepted bundles must survive a create/parse round trip unchanged
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	static const auto previousMessageHandler = qInstallMessageHandler(discardLogMessage);
	Q_UNUSED(previousMessageHandler)

	const auto bundle = TrustBundle::fromJson(Veyon::Crypto::ByteView(data, qsizetype(size)));
	if (bundle.has_value() == false)
	{
		return 0;
	}

	if (bundle->isValid() == false ||
		TlsClientPolicy::parseCaCertificates(bundle->caCertificatesPem().toLatin1()).has_value() == false)
	{
		abort();
	}

	const auto json = bundle->toJson();
	const auto reparsed = json.has_value() ? TrustBundle::fromJson(*json) : std::nullopt;
	// re-encoding may only fail because of the size limit
	if (reparsed.has_value() == false)
	{
		if (json.has_value())
		{
			abort();
		}
		return 0;
	}

	if (reparsed->caCertificatesPem() != bundle->caCertificatesPem() ||
		reparsed->enrollmentUrl != bundle->enrollmentUrl ||
		reparsed->enrollmentToken.toUnprotectedByteArray() != bundle->enrollmentToken.toUnprotectedByteArray() ||
		reparsed->certValidityDays != bundle->certValidityDays ||
		reparsed->renewBeforeDays != bundle->renewBeforeDays ||
		reparsed->serverMode != bundle->serverMode ||
		reparsed->clientMode != bundle->clientMode)
	{
		abort();
	}

	return 0;
}
