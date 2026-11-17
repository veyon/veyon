// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TlsHostCredentials.h"


QString TlsHostCredentials::defaultCertificateFilePath()
{
	return QStringLiteral("/var/lib/veyon/tls/host-cert.pem");
}
