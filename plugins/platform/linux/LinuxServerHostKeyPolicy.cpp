// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LinuxServerHostKeyPolicy.h"

#include <sys/stat.h>


QString LinuxServerHostKeyPolicy::refusal(const Conditions& conditions)
{
	if (conditions.runAsSessionUser == false || conditions.ptraceRestricted)
	{
		return {};
	}

	if (conditions.launchedThroughSystemdRun)
	{
		return QStringLiteral("processes of the session user can attach to veyon-server during start-up "
							  "(kernel.yama.ptrace_scope is 0 and the server is started through systemd-run) - "
							  "set kernel.yama.ptrace_scope >= 1");
	}

	if (conditions.nonDumpableFromExecFailure.isEmpty())
	{
		return {};
	}

	return QStringLiteral("processes of the session user can attach to veyon-server during start-up "
						  "(kernel.yama.ptrace_scope is 0 and %1) - set kernel.yama.ptrace_scope >= 1 "
						  "or install veyon-server with mode 0711")
		.arg(conditions.nonDumpableFromExecFailure);
}



QString LinuxServerHostKeyPolicy::nonDumpableFromExecFailure(const QString& serverFilePath, bool statSucceeded,
															 uid_t owner, mode_t mode, const QByteArray& suidDumpable)
{
	// execve() of a binary the user cannot read sets the dumpable flag to fs.suid_dumpable, which
	// keeps the process non-dumpable unless it is 1
	if (statSucceeded == false || owner != 0 || (mode & (S_IRGRP | S_IROTH)) != 0)
	{
		return QStringLiteral("%1 is readable by the session user").arg(serverFilePath);
	}

	const auto value = suidDumpable.trimmed();
	if (value != QByteArrayLiteral("0") && value != QByteArrayLiteral("2"))
	{
		return QStringLiteral("fs.suid_dumpable is not 0 or 2");
	}

	return {};
}
