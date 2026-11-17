// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "SecureBuffer.h"

#include <optional>

// Reading secrets from the terminal, standard input or files and writing files with secret
// content for veyon-cli
class SecretInputOutput
{
public:
	enum class LineResult {
		Read,	 // a line terminated by a newline or the end of input
		NoInput, // end of input before the first byte
		Invalid, // too long, NUL byte or read error
	};

	static constexpr int MaximumLineLength = 4096;

	// reads one line of at most MaximumLineLength bytes directly into a SecureBuffer, without the
	// line ending (LF or CRLF); also accepts input that ends without a newline
	[[nodiscard]] static LineResult readLine(int fd, Veyon::Crypto::SecureBuffer& line);

	// reads a line from standard input; on a terminal it shows the prompt and disables the echo,
	// which is restored also when the process is interrupted (SIGINT/SIGTERM/SIGHUP/SIGQUIT, Ctrl+C)
	[[nodiscard]] static LineResult readStandardInput(const QString& prompt, Veyon::Crypto::SecureBuffer& line);

	[[nodiscard]] static bool isStandardInputTerminal();

	// isPrivate (file with secrets): writes data to a new temporary file in the target directory,
	// created exclusively and without following symlinks, with mode 0600 (Linux) or a protected
	// DACL for SYSTEM and Administrators (Windows), then replaces the target. An existing target
	// that is no regular file or, on Linux, belongs to another user is refused; on Windows
	// directories and reparse points are refused (no owner check).
	// Otherwise the file is written like any other file (symlinks such as /dev/stdout are
	// followed, an existing file keeps its mode, the umask applies).
	[[nodiscard]] static bool writeFile(const QString& fileName, const QByteArray& data, bool isPrivate,
										QString* errorString);
};
