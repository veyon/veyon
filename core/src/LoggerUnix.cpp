// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Logger.h"

#include <cstdio>
#include <syslog.h>


void Logger::logEarlyCritical(const char* message)
{
	// the stderr of servers is not read by the service unless logging to the system is enabled,
	// so additionally use syslog which ends up in the journal
	fprintf(stderr, "%s\n", message);
	syslog(LOG_DAEMON | LOG_CRIT, "%s", message);
}
