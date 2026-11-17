// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

#include "IpcProtocol.h"
#include "SecureBuffer.h"
#include "VeyonCore.h"

#include <optional>

// Endpoint name and per-start token of a local IPC endpoint, transferred to the child process
// only via its stdin as the line "VEYONIPC1 <name> <hex64>\n" (docs/specs/02-ipc.md §2)
struct IpcStandardInputCredentials;

class VEYON_CORE_EXPORT IpcEndpointCredentials
{
public:
	// new endpoint "<prefix><sessionId>-<random8hex>" with a fresh 32-byte token
	[[nodiscard]] static std::optional<IpcEndpointCredentials> generate(QLatin1String endpointPrefix, int sessionId);

	// parses a line read by StandardInputReader::readSecretLine() (without the newline) and
	// wipes it in any case
	[[nodiscard]] static std::optional<IpcEndpointCredentials> parse(Veyon::Crypto::SecureBuffer& line);

	// reads and parses the credentials line from stdin within timeoutMs
	[[nodiscard]] static IpcStandardInputCredentials readFromStandardInput(int timeoutMs);

	static bool isValidEndpointName(const QString& name);

	[[nodiscard]] IpcEndpointCredentials clone() const;

	// the stdin line including the trailing newline
	[[nodiscard]] Veyon::Crypto::SecureBuffer toLine() const;

	const QString& endpointName() const
	{
		return m_endpointName;
	}

	const Veyon::Crypto::SecureBuffer& token() const
	{
		return m_token;
	}

	Veyon::Crypto::SecureBuffer takeToken()
	{
		return std::move(m_token);
	}

private:
	IpcEndpointCredentials(const QString& endpointName, Veyon::Crypto::SecureBuffer&& token) :
		m_endpointName(endpointName),
		m_token(std::move(token))
	{
	}

	QString m_endpointName;
	Veyon::Crypto::SecureBuffer m_token;
};



// credentials passed via stdin (IpcEndpointCredentials::readFromStandardInput())
struct IpcStandardInputCredentials
{
	enum class Result {
		Absent, // stdin is a terminal/not available or closed without data: started standalone
		Valid,
		Invalid, // malformed, truncated or not received in time: the caller must exit
	};
	Result result = Result::Absent;
	std::optional<IpcEndpointCredentials> credentials{};
};
