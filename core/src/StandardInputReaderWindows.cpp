// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include <QPointer>
#include <QThread>

#include "StandardInputReader.h"
#include "StandardInputReaderPlatform.h"

#include <cstdio>
#include <io.h>
#include <iostream>


namespace StandardInputPlatform {

int standardInputDescriptor()
{
	return _fileno(stdin);
}



bool isTerminal(int fd)
{
	return _isatty(fd);
}



WaitResult waitForInput(int fd, bool hasDeadline, const Deadline& deadline)
{
	static constexpr DWORD PollInterval = 10;

	const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
	if (handle == INVALID_HANDLE_VALUE)
	{
		return WaitResult::Failed;
	}

	// only pipes can block indefinitely (files and NUL return immediately)
	if (hasDeadline == false || GetFileType(handle) != FILE_TYPE_PIPE)
	{
		return WaitResult::Readable;
	}

	while (true)
	{
		DWORD available = 0;
		if (PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr) == FALSE)
		{
			return GetLastError() == ERROR_BROKEN_PIPE ? WaitResult::EndOfInput : WaitResult::Failed;
		}

		if (available > 0)
		{
			return WaitResult::Readable;
		}

		const auto remaining = remainingMilliseconds(deadline);
		if (remaining <= 0)
		{
			return WaitResult::Failed;
		}

		Sleep(DWORD(qMin<qint64>(remaining, PollInterval)));
	}
}



qint64 readCharacter(int fd, char* character)
{
	return _read(fd, character, 1);
}



} // namespace StandardInputPlatform



class StandardInputReaderThread : public QThread
{
public:
	StandardInputReaderThread(StandardInputReader* reader, QObject* parent) :
		QThread(parent),
		m_reader(reader)
	{
	}

protected:
	void run() override
	{
		std::ios_base::sync_with_stdio(false);

		while (isInterruptionRequested() == false)
		{
			std::string line;
			if (std::getline(std::cin, line))
			{
				if (m_reader)
				{
					m_reader->lineRead(QString::fromStdString(line));
				}
			}
			else
			{
				// stdin stays in failed state after EOF or a read error, so there's nothing more to read –
				// stop instead of spinning and flooding the log (and the event log) every 10 ms
				vDebug() << "EOF reached or failed to read from stdin";
				break;
			}
			QThread::msleep(10);
		}
	}

private:
	QPointer<StandardInputReader> m_reader;
};



StandardInputReader* StandardInputReader::create(QObject* parent)
{
	auto reader = new StandardInputReader(parent);
	// the thread may be blocked in std::getline() at any time, so it must never be owned by a parent which
	// destroys it while running (fatal "QThread: Destroyed while thread is still running", #1206) – instead
	// let it delete itself once finished; if still blocked at process exit, the OS simply terminates it
	auto thread = new StandardInputReaderThread(reader, nullptr);

	QObject::connect(reader, &QObject::destroyed, thread, &QThread::requestInterruption);
	QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);

	thread->start();

	return reader;
}
