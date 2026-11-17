// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SecureBuffer.h"
#include "VeyonCore.h"

#include <optional>
#include <veyonconfig.h>

class PlatformSecretFunctions;

// Privileged storage for secrets such as directory credentials and private keys
// (03-secretstore-config.md §2).
//
// Keys have the form "<scope>/<name>" (scope [a-z0-9-]{1,32}, name [A-Za-z0-9._-]{1,64} not ending
// with "."). Scopes are naming hygiene only, not access control: the access control is the process
// privilege boundary. The store is available only in processes initialized with the SecretStore
// flag (veyon-service, veyon-configurator and veyon-cli) and running as root/SYSTEM/administrator.
// It is never available in veyon-server, veyon-worker, the master, the capture helper or any other
// component, regardless of the user they run as; these components get the few secrets they need via the service's local IPC
// (02-ipc.md §6). When the store is not available all functions fail; there is never a fallback to
// a user-writable location.
class VEYON_CORE_EXPORT SecretStore
{
public:
	using Secret = Veyon::Crypto::SecureBuffer;

	static constexpr int MaximumScopeLength = 32;
	static constexpr int MaximumNameLength = 64;
	static constexpr qsizetype MaximumSecretSize = Veyon::Crypto::SecureBuffer::MaximumSize;

	[[nodiscard]] static bool isAvailable();
	[[nodiscard]] static std::optional<Secret> read(const QString& key);
	[[nodiscard]] static bool write(const QString& key, const Secret& value);
	[[nodiscard]] static bool remove(const QString& key);
	[[nodiscard]] static QStringList keys();

	// creates or verifies the storage location (at every start of a process with the SecretStore
	// flag, so that a directory pre-created by someone else is detected or fixed before the first
	// secret is written)
	[[nodiscard]] static bool prepareStorage();

	[[nodiscard]] static bool isValidKey(const QString& key);

#if defined(VEYON_WITH_TESTS)
	// unit tests without a VeyonCore instance (flags replace VeyonCore::initializationFlags());
	// nullptr restores the default behaviour
	static void setTestBackend(PlatformSecretFunctions* backend, VeyonCore::InitializationFlags flags);
#endif

private:
	static PlatformSecretFunctions* availableBackend();
};
