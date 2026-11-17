// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

#include <optional>

// Process hardening of veyon-server and veyon-worker (docs/specs/02-ipc.md §1a): processes of the
// same uid (on Wayland: the student) must not be able to read their memory via ptrace,
// process_vm_readv, /proc/<pid>/mem or core dumps. All functions only use plain system calls (no
// Qt logging) since hardenCurrentProcess() has to run as the first statement of main(), before
// QCoreApplication, VeyonCore and the platform plugin exist and before anything is read from stdin.
class VEYON_CORE_EXPORT ProcessHardening
{
public:
	// Linux: PR_SET_DUMPABLE 0 and RLIMIT_CORE 0, then verifies that no tracer attached between
	// execve() and prctl(). Fails closed (log line): the caller must exit when false is returned.
	//
	// setuid()/setgid()/setresuid()/capset() and friends reset the dumpable flag towards
	// fs.suid_dumpable (02-ipc.md §1a.3): any future in-process credential change of the server or
	// worker must call this function again right afterwards. Today neither process changes its
	// credentials after execve() (privileges are dropped in the child before exec, see
	// LinuxServerProcess::setProcessUserId() and LinuxCoreFunctions::runProgramAsUser()).
	//
	// TODO(T-PROC-02): a same-uid process can still attach between execve() and prctl() unless
	// Yama ptrace_scope >= 1; before GetTlsHostKey gets a provider, make the binaries non-readable
	// for the session user or hand out the token only after a "hardened" confirmation.
	[[nodiscard]] static bool hardenCurrentProcess();

	// RLIMIT_CORE 0 (Unix), without logging; inherited by child processes
	[[nodiscard]] static bool disableCoreDumps();

	// Linux: TracerPid of /proc/self/status; nothing if it can't be determined or the platform
	// has no equivalent
	[[nodiscard]] static std::optional<long> tracerPid();
};
