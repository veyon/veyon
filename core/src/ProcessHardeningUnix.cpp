// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Logger.h"
#include "ProcessHardening.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#ifdef Q_OS_LINUX
#include <sys/prctl.h>
#endif


bool ProcessHardening::hardenCurrentProcess()
{
#ifdef Q_OS_LINUX
	// a non-dumpable process can only be ptraced with CAP_SYS_PTRACE and its /proc/<pid>/{mem,
	// environ,maps,...} entries are owned by root
	if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) != 0 || prctl(PR_GET_DUMPABLE, 0, 0, 0, 0) != 0)
	{
		Logger::logEarlyCritical("failed to make process non-dumpable - refusing to run");
		return false;
	}
#endif

	if (disableCoreDumps() == false)
	{
		Logger::logEarlyCritical("failed to disable core dumps - refusing to run");
		return false;
	}

#ifdef Q_OS_LINUX
	// close the race of a tracer attaching between execve() and prctl()
	const auto tracer = tracerPid();
	if (tracer.has_value() == false)
	{
		Logger::logEarlyCritical("could not determine tracer of process - refusing to run");
		return false;
	}

	if (*tracer != 0)
	{
		Logger::logEarlyCritical("process is being traced - refusing to run");
		return false;
	}
#endif

	return true;
}



bool ProcessHardening::disableCoreDumps()
{
	const rlimit noCoreDumps{0, 0};
	return setrlimit(RLIMIT_CORE, &noCoreDumps) == 0;
}



std::optional<long> ProcessHardening::tracerPid()
{
#ifdef Q_OS_LINUX
	const auto fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
	if (fd < 0)
	{
		return {};
	}

	char status[8192];
	size_t size = 0;
	while (size < sizeof(status) - 1)
	{
		const auto bytesRead = read(fd, status + size, sizeof(status) - 1 - size);
		if (bytesRead < 0 && errno == EINTR)
		{
			continue;
		}
		if (bytesRead <= 0)
		{
			break;
		}
		size += size_t(bytesRead);
	}
	close(fd);
	status[size] = '\0';

	static constexpr char tracerPidField[] = "\nTracerPid:";
	const auto field = strstr(status, tracerPidField);
	if (field == nullptr)
	{
		return {};
	}

	char* end = nullptr;
	errno = 0;
	const auto value = strtol(field + sizeof(tracerPidField) - 1, &end, 10);
	if (errno != 0 || end == field + sizeof(tracerPidField) - 1 || (*end != '\n' && *end != '\0'))
	{
		return {};
	}

	return value;
#else
	return {};
#endif
}
