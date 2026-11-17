// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtGlobal>

#include <chrono>

// platform part of StandardInputReader (internal, not installed), implemented in
// StandardInputReaderUnix.cpp / StandardInputReaderWindows.cpp; core/CMakeLists.txt selects the file
namespace StandardInputPlatform {

enum class WaitResult {
	Readable,
	EndOfInput,
	Failed,
};

using Deadline = std::chrono::steady_clock::time_point;

inline qint64 remainingMilliseconds(const Deadline& deadline)
{
	return qMax<qint64>(0, std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
}

// file descriptor of stdin, -1 if not available
int standardInputDescriptor();

bool isTerminal(int fd);

// waits until fd is readable or the deadline expired (Windows: pipes only)
WaitResult waitForInput(int fd, bool hasDeadline, const Deadline& deadline);

// reads a single byte, returns the result of read()
qint64 readCharacter(int fd, char* character);

} // namespace StandardInputPlatform
