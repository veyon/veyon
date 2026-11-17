// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ProcessHardening.h"


// Stubs: processes running as SYSTEM need no dumpability control (other users can't open them for
// memory access), and the exclusion from Windows Error Reporting (WER) dumps is handled by the
// installer (nsis).
bool ProcessHardening::hardenCurrentProcess()
{
	return true;
}



bool ProcessHardening::disableCoreDumps()
{
	return true;
}



std::optional<long> ProcessHardening::tracerPid()
{
	return {};
}
