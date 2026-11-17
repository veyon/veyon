// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QStringList>

#include "CryptoError.h"
#include "OpenSslUtils.h"

#include <openssl/crypto.h>
#include <openssl/err.h>

namespace Veyon::Crypto {

static thread_local QString threadLastError;



QString lastError()
{
	return threadLastError;
}



void clearLastError()
{
	threadLastError.clear();
}



QString openSslVersion()
{
	return QString::fromLatin1(OpenSSL_version(OPENSSL_VERSION));
}



namespace Internal {

void setError(const char* operation, const char* reason)
{
	threadLastError = QLatin1String(operation) + QLatin1String(": ") + QLatin1String(reason);
}



void setOpenSslError(const char* operation)
{
	static constexpr int MaximumReportedErrors = 4;

	QStringList reasons;
	while (const auto code = ERR_get_error())
	{
		if (reasons.size() < MaximumReportedErrors)
		{
			// ERR_error_string_n() only renders library/reason codes, never the additional error
			// data some OpenSSL functions attach (which may contain input fragments)
			char text[256] = {};
			ERR_error_string_n(code, text, sizeof(text));
			reasons.append(QString::fromLatin1(text));
		}
	}

	threadLastError = QLatin1String(operation) + QLatin1String(": ") +
		(reasons.isEmpty() ? QStringLiteral("operation failed") : reasons.join(QStringLiteral("; ")));
}

} // namespace Internal

} // namespace Veyon::Crypto
