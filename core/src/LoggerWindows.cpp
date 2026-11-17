// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include "Logger.h"

#include <cstdio>


void Logger::logEarlyCritical(const char* message)
{
	fprintf(stderr, "%s\n", message);
	OutputDebugStringA(message);
}
