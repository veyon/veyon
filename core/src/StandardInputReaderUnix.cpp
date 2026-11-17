// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSocketNotifier>
#include <QTextStream>

#include "StandardInputReader.h"
#include "StandardInputReaderPlatform.h"

#include <cerrno>
#include <poll.h>
#include <unistd.h>


namespace StandardInputPlatform {

int standardInputDescriptor()
{
	return STDIN_FILENO;
}



bool isTerminal(int fd)
{
	return isatty(fd);
}



WaitResult waitForInput(int fd, bool hasDeadline, const Deadline& deadline)
{
	if (hasDeadline == false)
	{
		return WaitResult::Readable;
	}

	while (true)
	{
		pollfd pollFd{fd, POLLIN, 0};
		const auto pollResult = poll(&pollFd, 1, int(remainingMilliseconds(deadline)));
		if (pollResult < 0 && errno == EINTR)
		{
			continue;
		}

		// POLLHUP without data is reported as readable as well, read() then returns EOF
		return pollResult > 0 ? WaitResult::Readable : WaitResult::Failed;
	}
}



qint64 readCharacter(int fd, char* character)
{
	return read(fd, character, 1);
}



} // namespace StandardInputPlatform



StandardInputReader* StandardInputReader::create(QObject* parent)
{
	auto reader = new StandardInputReader(parent);

	auto notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, reader);
	QObject::connect(notifier, &QSocketNotifier::activated,
					 reader, [reader]() {
		reader->lineRead(QTextStream(stdin).readLine());
	});

	return reader;
}
