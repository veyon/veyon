// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

namespace Veyon::Crypto {
class SecureBuffer;
}

class VEYON_CORE_EXPORT StandardInputReader : public QObject
{
	Q_OBJECT
public:
	enum class SecretLineResult {
		Read,	 // one complete line
		NoInput, // EOF before the first byte
		Invalid, // too long, NUL byte, EOF inside the line, read error or timeout
	};
	Q_ENUM(SecretLineResult)

	StandardInputReader(QObject* parent) :
		QObject(parent) {}

	// asynchronously emits lineRead() for every line read from stdin (Linux: QSocketNotifier,
	// Windows: reader thread)
	static StandardInputReader* create(QObject* parent);

	// Synchronously reads one line of at most maxLength bytes (without the terminating newline)
	// from fd directly into out, without intermediate copies; out stays empty unless Read is
	// returned. timeoutMs (negative = none) limits the time for the whole line, on Windows for
	// pipes only. Reads byte by byte so that nothing after the line is consumed.
	[[nodiscard]] static SecretLineResult readSecretLine(int fd, Veyon::Crypto::SecureBuffer& out,
														 int maxLength = 128, int timeoutMs = -1);

Q_SIGNALS:
	void lineRead(const QString& line);
};
