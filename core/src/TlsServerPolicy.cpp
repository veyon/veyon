// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TlsServerPolicy.h"
#include "VeyonConfiguration.h"
#include "pki/TlsConfiguration.h"


TlsServerPolicy TlsServerPolicy::fromConfiguration(const VeyonConfiguration& configuration)
{
	TlsServerPolicy policy;
	policy.mode = TlsConfiguration::serverMode(configuration);
	policy.sniffTimeoutMs = clampedSniffTimeout(configuration.tlsSniffTimeoutMs());

	return policy;
}



int TlsServerPolicy::clampedSniffTimeout(int sniffTimeout)
{
	return qBound(MinimumSniffTimeout, sniffTimeout, MaximumSniffTimeout);
}
