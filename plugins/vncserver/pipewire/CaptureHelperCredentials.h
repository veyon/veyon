// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "SecureBuffer.h"

#include <optional>

// The only data veyon-capture-helper receives from veyon-server, via the helper's stdin as the
// line "VEYONCAP1 <hex64> <socketName>\n" (02-ipc.md §1a.5): a per-start random VNC password and
// the name of the Unix domain socket the helper listens on in the private endpoint directory
// ($XDG_RUNTIME_DIR/veyon, IpcServer::workerEndpointDirectory()). veyon-server authenticates with
// the password like with an external VNC server. The helper never gets the IPC token, host key or
// any other secret.
class CaptureHelperCredentials
{
public:
	static constexpr char LinePrefix[] = "VEYONCAP1";
	static constexpr qsizetype PasswordSize = 32;
	// VNC authentication uses only the first 8 password bytes as DES key (ignoring the most
	// significant bit of each byte), so its effective strength is 56 bits
	static constexpr qsizetype VncAuthenticationKeySize = 8;
	// "capture-<sessionId>-<8 hex digits>.sock"
	static constexpr qsizetype MaximumSocketNameSize = 8 + 9 + 1 + 8 + 5;
	// prefix SP hex64 SP socket name (without the newline)
	static constexpr qsizetype MaximumLineSize =
		qsizetype(sizeof(LinePrefix)) - 1 + 1 + PasswordSize * 2 + 1 + MaximumSocketNameSize;
	// veyon-server writes the line right after the helper has started
	static constexpr int StandardInputTimeout = 5000;

	// PasswordSize random bytes with the most significant bit set in each byte: VNC authentication
	// ignores that bit, and the DES key bytes never are NUL, which libvncserver's rfbEncryptBytes()
	// would take as the end of the password
	[[nodiscard]] static std::optional<Veyon::Crypto::SecureBuffer> generatePassword();

	// new random socket name for the given session
	[[nodiscard]] static std::optional<QString> generateSocketName(int sessionId);

	[[nodiscard]] static bool isValidSocketName(const QString& socketName);

	// the password must have exactly PasswordSize bytes and no NUL byte within the DES key, the
	// socket name must be valid
	[[nodiscard]] static std::optional<CaptureHelperCredentials> create(Veyon::Crypto::SecureBuffer&& password,
																		const QString& socketName);

	// parses a line read by StandardInputReader::readSecretLine() (without the newline) and
	// wipes it in any case
	[[nodiscard]] static std::optional<CaptureHelperCredentials> parse(Veyon::Crypto::SecureBuffer& line);

	// reads and parses the line from stdin within timeoutMs; unlike veyon-server the helper never
	// runs standalone, so missing input (terminal, EOF) is a failure as well
	[[nodiscard]] static std::optional<CaptureHelperCredentials> readFromStandardInput(int timeoutMs);

	// the stdin line including the trailing newline
	[[nodiscard]] Veyon::Crypto::SecureBuffer toLine() const;

	const Veyon::Crypto::SecureBuffer& password() const
	{
		return m_password;
	}

	const QString& socketName() const
	{
		return m_socketName;
	}

private:
	CaptureHelperCredentials(Veyon::Crypto::SecureBuffer&& password, const QString& socketName) :
		m_password(std::move(password)),
		m_socketName(socketName)
	{
	}

	Veyon::Crypto::SecureBuffer m_password;
	QString m_socketName;
};
