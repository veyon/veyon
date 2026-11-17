// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "ProcessHardening.h"

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <grp.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// docs/specs/02-ipc.md §8.12: a hardened process can't be inspected by another process of the
// same uid, and refuses to run when traced at start. All checks run in forked children (without
// exec) so that the test process itself stays dumpable; as root the children switch to nobody
// first because CAP_SYS_PTRACE would bypass the protection.
class ProcessHardeningTest : public QObject
{
	Q_OBJECT
private:
	static constexpr uid_t UnprivilegedId = 65534;

	enum ExitCode {
		Passed = 0,
		DropPrivilegesFailed = 10,
		ForkFailed,
		HelperNotReady,
		ControlAttachFailed,
		ControlMemoryFailed,
		AttachNotDenied,
		MemoryNotDenied,
		HardeningFailed,
		StillDumpable,
		CoreDumpsEnabled,
		TracedHelperRan,
	};

	static bool dropPrivileges()
	{
		if (geteuid() != 0)
		{
			return true;
		}
		return setgroups(0, nullptr) == 0 && setgid(UnprivilegedId) == 0 && setuid(UnprivilegedId) == 0;
	}

	// forks a helper which hardens itself (or makes itself dumpable for the control case),
	// reports readiness through a pipe and then waits until it is killed
	static pid_t startHelper(bool harden)
	{
		int readyPipe[2];
		if (pipe(readyPipe) != 0)
		{
			return -1;
		}

		const auto pid = fork();
		if (pid == 0)
		{
			close(readyPipe[0]);
			const auto ok = harden ? ProcessHardening::hardenCurrentProcess() :
									 prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == 0;
			const char status = ok ? '1' : '0';
			if (write(readyPipe[1], &status, 1) != 1)
			{
				_exit(HelperNotReady);
			}
			while (true)
			{
				pause();
			}
		}

		close(readyPipe[1]);
		char status = 0;
		const auto ready = pid > 0 && read(readyPipe[0], &status, 1) == 1 && status == '1';
		close(readyPipe[0]);

		if (ready == false && pid > 0)
		{
			kill(pid, SIGKILL);
			waitpid(pid, nullptr, 0);
			return -1;
		}

		return pid;
	}

	static void stopHelper(pid_t pid)
	{
		kill(pid, SIGKILL);
		waitpid(pid, nullptr, 0);
	}

	static int tryAttach(pid_t pid)
	{
		if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == 0)
		{
			waitpid(pid, nullptr, 0);
			ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
			return 0;
		}
		return errno;
	}

	static int tryReadMemory(pid_t pid)
	{
		const QByteArray path = QByteArray("/proc/") + QByteArray::number(pid) + "/mem";
		const auto fd = open(path.constData(), O_RDONLY);
		if (fd >= 0)
		{
			close(fd);
			return 0;
		}
		return errno;
	}

	static int runInChild(int (*function)())
	{
		const auto pid = fork();
		if (pid == 0)
		{
			_exit(function());
		}

		int status = 0;
		if (pid < 0 || waitpid(pid, &status, 0) != pid || WIFEXITED(status) == false)
		{
			return -1;
		}
		return WEXITSTATUS(status);
	}

	static int sameUidAccess()
	{
		if (dropPrivileges() == false)
		{
			return DropPrivilegesFailed;
		}

		// control: without hardening another process of the same uid (here: the parent) can attach
		const auto plainHelper = startHelper(false);
		if (plainHelper < 0)
		{
			return HelperNotReady;
		}
		const auto controlAttach = tryAttach(plainHelper);
		const auto controlMemory = tryReadMemory(plainHelper);
		stopHelper(plainHelper);
		if (controlAttach != 0)
		{
			return ControlAttachFailed;
		}
		if (controlMemory != 0)
		{
			return ControlMemoryFailed;
		}

		const auto hardenedHelper = startHelper(true);
		if (hardenedHelper < 0)
		{
			return HardeningFailed;
		}
		const auto attachError = tryAttach(hardenedHelper);
		const auto memoryError = tryReadMemory(hardenedHelper);
		stopHelper(hardenedHelper);

		if (attachError != EPERM)
		{
			return AttachNotDenied;
		}
		// /proc/<pid>/mem of a non-dumpable process is owned by root (spike S7: EACCES)
		if (memoryError != EPERM && memoryError != EACCES)
		{
			return MemoryNotDenied;
		}

		return Passed;
	}

	static int dumpableAndCoreLimit()
	{
		if (ProcessHardening::hardenCurrentProcess() == false)
		{
			return HardeningFailed;
		}
		if (prctl(PR_GET_DUMPABLE, 0, 0, 0, 0) != 0)
		{
			return StillDumpable;
		}
		rlimit limit{};
		if (getrlimit(RLIMIT_CORE, &limit) != 0 || limit.rlim_cur != 0 || limit.rlim_max != 0)
		{
			return CoreDumpsEnabled;
		}
		// re-assertion (e.g. after a credential change) works as well
		return ProcessHardening::hardenCurrentProcess() ? Passed : HardeningFailed;
	}

	static int tracedAtStart()
	{
		const auto pid = fork();
		if (pid == 0)
		{
			// simulates a tracer that attached between execve() and prctl()
			ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
			raise(SIGSTOP);
			_exit(ProcessHardening::hardenCurrentProcess() ? TracedHelperRan : Passed);
		}
		if (pid < 0)
		{
			return ForkFailed;
		}

		int status = 0;
		waitpid(pid, &status, 0);
		ptrace(PTRACE_CONT, pid, nullptr, nullptr);
		while (waitpid(pid, &status, 0) == pid && WIFEXITED(status) == false && WIFSIGNALED(status) == false)
		{
			ptrace(PTRACE_CONT, pid, nullptr, WIFSTOPPED(status) ? WSTOPSIG(status) : 0);
		}

		return WIFEXITED(status) ? WEXITSTATUS(status) : TracedHelperRan;
	}

private Q_SLOTS:
	void tracerPidOfUntracedProcess()
	{
		const auto tracer = ProcessHardening::tracerPid();
		QVERIFY(tracer.has_value());
		QCOMPARE(*tracer, 0L);
	}

	void dumpableAndCoreLimitDisabled()
	{
		QCOMPARE(runInChild(&dumpableAndCoreLimit), int(Passed));
	}

	void sameUidAccessDenied()
	{
		const auto result = runInChild(&sameUidAccess);
		if (result == ControlAttachFailed || result == ControlMemoryFailed)
		{
			QSKIP("ptrace between processes of the same uid is not possible in this environment");
		}
		QCOMPARE(result, int(Passed));
	}

	void refusesToRunWhenTraced()
	{
		QCOMPARE(runInChild(&tracedAtStart), int(Passed));
	}
};


QTEST_GUILESS_MAIN(ProcessHardeningTest)
#include "ProcessHardeningTest.moc"
