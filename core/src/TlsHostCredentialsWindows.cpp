// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>

#include "PlatformFilesystemFunctions.h"
#include "PlatformPluginInterface.h"
#include "TlsHostCredentials.h"


QString TlsHostCredentials::defaultCertificateFilePath()
{
	// %ProgramData%\Veyon
	return QDir::toNativeSeparators(VeyonCore::platform().filesystemFunctions().globalAppDataPath() +
									QStringLiteral("/tls/host-cert.pem"));
}
