// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#if defined(_WIN32)
#include <windows.h>

#include <sddl.h>
#endif

#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>

#include "Random.h"
#include "SecretInputOutput.h"

#include <array>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef Q_OS_WIN
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
// SYSTEM and Administrators only, not inherited
static constexpr auto PrivateFileSecurityDescriptor = L"D:P(A;;FA;;;SY)(A;;FA;;;BA)";

static HANDLE consoleHandle = nullptr;
static DWORD savedConsoleMode = 0;
static std::atomic<bool> isConsoleEchoDisabled{false};

static BOOL WINAPI restoreConsoleOnControlEvent(DWORD controlType)
{
	Q_UNUSED(controlType)
	if (isConsoleEchoDisabled)
	{
		SetConsoleMode(consoleHandle, savedConsoleMode);
	}
	// continue with the default handling (termination)
	return FALSE;
}
#else
static constexpr std::array<int, 4> InterruptSignals{SIGINT, SIGTERM, SIGHUP, SIGQUIT};

static termios savedTerminalAttributes{};
static volatile sig_atomic_t isTerminalEchoDisabled = 0;
static std::array<struct sigaction, InterruptSignals.size()> previousSignalActions{};

static void restoreTerminalOnSignal(int signalNumber)
{
	if (isTerminalEchoDisabled)
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &savedTerminalAttributes);
	}

	// then terminate as without this handler
	for (size_t i = 0; i < InterruptSignals.size(); ++i)
	{
		if (InterruptSignals[i] == signalNumber)
		{
			sigaction(signalNumber, &previousSignalActions[i], nullptr);
		}
	}
	raise(signalNumber);
}
#endif


// disables the echo of the terminal while it exists
class TerminalEchoSuppressor
{
public:
	TerminalEchoSuppressor()
	{
#ifdef Q_OS_WIN
		consoleHandle = GetStdHandle(STD_INPUT_HANDLE);
		if (GetConsoleMode(consoleHandle, &savedConsoleMode))
		{
			SetConsoleCtrlHandler(restoreConsoleOnControlEvent, TRUE);
			isConsoleEchoDisabled = true;
			SetConsoleMode(consoleHandle, savedConsoleMode & ~DWORD(ENABLE_ECHO_INPUT));
		}
#else
		if (tcgetattr(STDIN_FILENO, &savedTerminalAttributes) == 0)
		{
			struct sigaction action{};
			action.sa_handler = restoreTerminalOnSignal;
			sigemptyset(&action.sa_mask);
			for (size_t i = 0; i < InterruptSignals.size(); ++i)
			{
				sigaction(InterruptSignals[i], &action, &previousSignalActions[i]);
			}

			auto silentAttributes = savedTerminalAttributes;
			silentAttributes.c_lflag &= ~tcflag_t(ECHO);
			isTerminalEchoDisabled = 1;
			tcsetattr(STDIN_FILENO, TCSAFLUSH, &silentAttributes);
		}
#endif
	}

	~TerminalEchoSuppressor()
	{
#ifdef Q_OS_WIN
		if (isConsoleEchoDisabled)
		{
			SetConsoleMode(consoleHandle, savedConsoleMode);
			isConsoleEchoDisabled = false;
			SetConsoleCtrlHandler(restoreConsoleOnControlEvent, FALSE);
		}
#else
		if (isTerminalEchoDisabled)
		{
			tcsetattr(STDIN_FILENO, TCSAFLUSH, &savedTerminalAttributes);
			isTerminalEchoDisabled = 0;
			for (size_t i = 0; i < InterruptSignals.size(); ++i)
			{
				sigaction(InterruptSignals[i], &previousSignalActions[i], nullptr);
			}
		}
#endif
	}

	TerminalEchoSuppressor(const TerminalEchoSuppressor&) = delete;
	TerminalEchoSuppressor& operator=(const TerminalEchoSuppressor&) = delete;
};



SecretInputOutput::LineResult SecretInputOutput::readLine(int fd, Veyon::Crypto::SecureBuffer& line)
{
	line.clear();

	Veyon::Crypto::SecureBuffer buffer(MaximumLineLength);
	qsizetype size = 0;
	auto isEmptyInput = true;

	while (true)
	{
		char character = 0;
#ifdef Q_OS_WIN
		const auto bytesRead = _read(fd, &character, 1);
#else
		const auto bytesRead = read(fd, &character, 1);
#endif
		if (bytesRead < 0 && errno == EINTR)
		{
			continue;
		}
		if (bytesRead < 0)
		{
			return LineResult::Invalid;
		}
		if (bytesRead == 0)
		{
			if (isEmptyInput)
			{
				return LineResult::NoInput;
			}
			break;
		}

		isEmptyInput = false;

		if (character == '\n')
		{
			break;
		}
		if (character == '\0' || size >= MaximumLineLength)
		{
			return LineResult::Invalid;
		}
		buffer.data()[size++] = character;
	}

	if (size > 0 && buffer.constData()[size - 1] == '\r')
	{
		--size;
	}

	line = Veyon::Crypto::SecureBuffer(buffer.constData(), size);

	return LineResult::Read;
}



SecretInputOutput::LineResult SecretInputOutput::readStandardInput(const QString& prompt, Veyon::Crypto::SecureBuffer& line)
{
	if (isStandardInputTerminal() == false)
	{
		return readLine(fileno(stdin), line);
	}

	fprintf(stderr, "%s", prompt.toLocal8Bit().constData());
	fflush(stderr);

	TerminalEchoSuppressor echoSuppressor;
	const auto result = readLine(fileno(stdin), line);

	fprintf(stderr, "\n");

	return result;
}



bool SecretInputOutput::isStandardInputTerminal()
{
#ifdef Q_OS_WIN
	return _isatty(_fileno(stdin)) != 0;
#else
	return isatty(STDIN_FILENO) != 0;
#endif
}



bool SecretInputOutput::writeFile(const QString& fileName, const QByteArray& data, bool isPrivate, QString* errorString)
{
	const auto setError = [errorString](const QString& message) {
		if (errorString)
		{
			*errorString = message;
		}
		return false;
	};

	// without secrets: as before (follows symlinks, e.g. /dev/stdout, keeps the mode of an
	// existing file, umask applies)
	if (isPrivate == false)
	{
		QFile file(fileName);
		if (file.open(QFile::WriteOnly | QFile::Truncate) == false || file.write(data) != data.size())
		{
			return setError(file.errorString());
		}
		return true;
	}

	const QFileInfo fileInfo(fileName);
	const auto targetPath = fileInfo.absoluteFilePath();
	const auto randomSuffix = Veyon::Crypto::Random::hex(8);
	if (randomSuffix.has_value() == false)
	{
		return setError(QStringLiteral("no random numbers"));
	}
	const QString temporaryPath = fileInfo.absolutePath() + QLatin1String("/.") + fileInfo.fileName() +
		QLatin1Char('.') + *randomSuffix + QLatin1String(".tmp");

#ifdef Q_OS_WIN
	const auto targetAttributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(targetPath.utf16()));
	if (targetAttributes != INVALID_FILE_ATTRIBUTES &&
		(targetAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0)
	{
		return setError(QStringLiteral("refusing to replace a directory or reparse point"));
	}

	SECURITY_ATTRIBUTES securityAttributes{};
	securityAttributes.nLength = sizeof(securityAttributes);
	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(PrivateFileSecurityDescriptor, SDDL_REVISION_1,
															 &securityAttributes.lpSecurityDescriptor, nullptr) == FALSE)
	{
		return setError(QStringLiteral("could not create security descriptor"));
	}
	const auto freeDescriptor = qScopeGuard([&securityAttributes]() {
		if (securityAttributes.lpSecurityDescriptor)
		{
			LocalFree(securityAttributes.lpSecurityDescriptor);
		}
	});

	const auto temporaryPathWide = reinterpret_cast<LPCWSTR>(temporaryPath.utf16());
	const auto file = CreateFileW(temporaryPathWide, GENERIC_WRITE, 0, &securityAttributes,
								  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return setError(QStringLiteral("could not create file (error %1)").arg(GetLastError()));
	}

	DWORD bytesWritten = 0;
	const auto written = WriteFile(file, data.constData(), DWORD(data.size()), &bytesWritten, nullptr) &&
		bytesWritten == DWORD(data.size()) && FlushFileBuffers(file);
	CloseHandle(file);

	if (written == false ||
		MoveFileExW(temporaryPathWide, reinterpret_cast<LPCWSTR>(targetPath.utf16()),
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE)
	{
		const auto error = GetLastError();
		DeleteFileW(temporaryPathWide);
		return setError(QStringLiteral("could not write file (error %1)").arg(error));
	}
#else
	const auto target = QFile::encodeName(targetPath);
	struct stat targetStatus{};
	if (lstat(target.constData(), &targetStatus) == 0)
	{
		if (S_ISREG(targetStatus.st_mode) == false || targetStatus.st_uid != geteuid())
		{
			return setError(QStringLiteral("refusing to replace a file which is no regular file or belongs to another user"));
		}
	}
	else if (errno != ENOENT)
	{
		return setError(QStringLiteral("could not access file (error %1)").arg(errno));
	}

	const auto temporary = QFile::encodeName(temporaryPath);
	const mode_t mode = 0600;
	const auto fd = open(temporary.constData(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC, mode);
	if (fd < 0)
	{
		return setError(QStringLiteral("could not create file (error %1)").arg(errno));
	}

	auto success = fchmod(fd, mode) == 0;
	qsizetype bytesWritten = 0;
	while (success && bytesWritten < data.size())
	{
		const auto result = write(fd, data.constData() + bytesWritten, size_t(data.size() - bytesWritten));
		if (result < 0 && errno == EINTR)
		{
			continue;
		}
		success = result > 0;
		bytesWritten += qMax<qsizetype>(result, 0);
	}
	success = success && fsync(fd) == 0;
	success = (close(fd) == 0) && success;

	if (success == false || rename(temporary.constData(), target.constData()) != 0)
	{
		const auto error = errno;
		unlink(temporary.constData());
		return setError(QStringLiteral("could not write file (error %1)").arg(error));
	}
#endif

	return true;
}
