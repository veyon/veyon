// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"
#include "SecureBuffer.h"

#include <optional>

// clazy:exclude=copyable-polymorphic

class PersistentLogonCredentials
{
public:
	using Password = CryptoCore::SecureArray;

	static constexpr qsizetype MaximumUsernameSize = 1024;
	static constexpr qsizetype MaximumPasswordSize = 4096;

	static bool read(QString* username, Password* password);
	static bool write(const QString& username, const Password& password);

	// The service data blob holds the plain password: it only travels over the authenticated and
	// encrypted IPC channel and lives in the service's memory (02-ipc.md §6, 05-tls.md §7).
	// Layout: username size (quint32, big endian), username (UTF-8), password size, password.
	[[nodiscard]] static std::optional<Veyon::Crypto::SecureBuffer> encode(const QString& username,
																		   const Password& password);
	// false (outputs unchanged) for empty fields, oversized fields and sizes not matching the data
	[[nodiscard]] static bool decode(const Veyon::Crypto::SecureBuffer& data, QString* username, Password* password);
};
