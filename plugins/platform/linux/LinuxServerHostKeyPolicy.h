// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include <sys/types.h>

// Decides whether veyon-service may hand the TLS host key to a session server (threat model
// T-PROC-02, 02-ipc.md §1a.4, §6): a server running as the session user (Wayland) makes itself
// non-dumpable only in main(), so a process of the session user may attach before. That is
// prevented by Yama (kernel.yama.ptrace_scope >= 1) or, for a server the service's child execs
// directly after setuid(), by the kernel making it non-dumpable at execve(): the binary is owned by
// root and not readable by group/other, and fs.suid_dumpable is not 1. The latter does not hold
// when the server is started through systemd-run: systemd-run runs as the session user and is
// dumpable, and a ptrace attachment to it survives the execve() of the server.
class LinuxServerHostKeyPolicy
{
public:
	struct Conditions
	{
		bool runAsSessionUser = true;
		bool ptraceRestricted = false;
		bool launchedThroughSystemdRun = true;
		// see nonDumpableFromExecFailure(), empty if the condition holds
		QString nonDumpableFromExecFailure = QStringLiteral("unknown");
	};

	// empty if the server may get the host key, else the cause and remedy
	static QString refusal(const Conditions& conditions);

	// why the server binary would not be non-dumpable from execve() on, empty if it would be;
	// statSucceeded false or suidDumpable empty (unreadable) fail closed
	static QString nonDumpableFromExecFailure(const QString& serverFilePath, bool statSucceeded, uid_t owner,
											  mode_t mode, const QByteArray& suidDumpable);
};
