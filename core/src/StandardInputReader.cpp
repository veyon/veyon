// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SecureBuffer.h"
#include "StandardInputReader.h"
#include "StandardInputReaderPlatform.h"

#include <cerrno>
#include <chrono>


StandardInputReader::SecretLineResult StandardInputReader::readSecretLine(int fd, Veyon::Crypto::SecureBuffer& out,
																		  int maxLength, int timeoutMs)
{
	out.clear();

	if (fd < 0 || maxLength <= 0)
	{
		return SecretLineResult::Invalid;
	}

	Veyon::Crypto::SecureBuffer line(maxLength);
	qsizetype lineSize = 0;

	const auto hasDeadline = timeoutMs >= 0;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(qMax(0, timeoutMs));

	using StandardInputPlatform::WaitResult;

	while (true)
	{
		const auto waitResult = StandardInputPlatform::waitForInput(fd, hasDeadline, deadline);
		if (waitResult == WaitResult::EndOfInput)
		{
			return lineSize == 0 ? SecretLineResult::NoInput : SecretLineResult::Invalid;
		}
		if (waitResult == WaitResult::Failed)
		{
			return SecretLineResult::Invalid;
		}

		char character = 0;
		const auto bytesRead = StandardInputPlatform::readCharacter(fd, &character);
		if (bytesRead < 0 && errno == EINTR)
		{
			continue;
		}

		if (bytesRead == 0)
		{
			return lineSize == 0 ? SecretLineResult::NoInput : SecretLineResult::Invalid;
		}

		if (bytesRead != 1)
		{
			return SecretLineResult::Invalid;
		}

		if (character == '\n')
		{
			break;
		}

		if (character == '\0' || lineSize >= maxLength)
		{
			return SecretLineResult::Invalid;
		}

		line.data()[lineSize++] = character;
		character = 0;
	}

	// shrinking zeroises the dropped tail
	if (line.resize(lineSize) == false)
	{
		return SecretLineResult::Invalid;
	}

	out = std::move(line);
	return SecretLineResult::Read;
}
