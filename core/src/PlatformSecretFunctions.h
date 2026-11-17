// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformPluginInterface.h"
#include "SecureBuffer.h"

#include <optional>

// clazy:excludeall=copyable-polymorphic

// Storage backend of SecretStore (03-secretstore-config.md §2). Keys are validated by SecretStore
// before they reach the backend; implementations must nevertheless never build paths from
// unvalidated keys. Implementations refuse (and log the path, never the value) whenever the
// storage location or a secret file is accessible by anyone but the privileged owner.
class VEYON_CORE_EXPORT PlatformSecretFunctions
{
public:
	virtual ~PlatformSecretFunctions() = default;

	virtual std::optional<Veyon::Crypto::SecureBuffer> readSecret(const QString& key) = 0;
	virtual bool writeSecret(const QString& key, const Veyon::Crypto::SecureBuffer& value) = 0;
	virtual bool removeSecret(const QString& key) = 0;
	virtual QStringList secretKeys() = 0;
	// true if the process is privileged (root/SYSTEM/elevated) and the backend is usable
	virtual bool isAvailable() = 0;
	// creates the storage location or verifies (Windows: repairs, if safely possible) an existing
	// one, also when nothing is written yet; called at every start of veyon-service
	virtual bool prepareStorage() = 0;
};
