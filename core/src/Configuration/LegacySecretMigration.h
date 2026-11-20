// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class CryptoCore;

namespace Configuration {

class Object;

// Moves the values of the former Configuration::Password properties (RSA ciphertext with the
// public default-pkey.pem, i.e. readable by anyone with the binaries) into the SecretStore and
// replaces them with their SecretReference (03-secretstore-config.md §4). The migrated properties
// are the rows of the SecretPropertyRegistry with isLegacyPassword set; values of properties which
// are not registered (e.g. of a plugin which is not installed) are left untouched until a
// privileged start with the plugin installed migrates them (§3a). This is the only place
// that decrypts stored legacy values. It runs in privileged components only (SecretStore
// available): at every start of veyon-service and of an elevated veyon-cli/veyon-configurator, so
// that values re-applied by a GPO or configuration management are removed again, and for
// imported configuration files.
//
// At startup a legacy value never replaces or deletes a secret already in the store (which may
// have been rotated since), regardless of Core/SecretMigrationVersion, which configuration
// management may have removed with the rest of the configuration: it is scrubbed from the
// configuration with a warning (03-secretstore-config.md §4.4). Only explicit imports overwrite.
class VEYON_CORE_EXPORT LegacySecretMigration
{
public:
	static constexpr int MigrationVersion = 1;

	// one-time backup of the configuration before the first migration (03-secretstore-config.md §4.8)
	static QString preMigrationConfigurationKey()
	{
		return QStringLiteral("core/preMigrationConfig");
	}

	enum class Result {
		NothingToMigrate,
		Migrated,
		Failed, // SecretStore unavailable or a value could not be migrated (left in place)
	};

	[[nodiscard]] static bool hasLegacySecrets(const Object& configuration);

	enum class ExistingSecrets {
		Keep,	   // automatic migration (start of a privileged component)
		Overwrite, // explicit admin action (importing or loading a configuration file)
	};

	// writes each legacy value to the SecretStore and replaces it with its reference; sets
	// Core/SecretMigrationVersion once no legacy value is left; values that fail stay unchanged.
	// With ExistingSecrets::Keep a secret that is already stored is never replaced or deleted:
	// the legacy value is only removed from the configuration (with a warning). The keys of stored
	// secrets replaced (or deleted) with ExistingSecrets::Overwrite are appended to
	// replacedSecretKeys, so that the admin can be told about them.
	[[nodiscard]] static Result migrate(Object& configuration, const CryptoCore& cryptoCore,
										ExistingSecrets existingSecrets = ExistingSecrets::Keep,
										QStringList* replacedSecretKeys = nullptr);

	// keys of secrets the secret properties of the configuration refer to but which are not stored
	[[nodiscard]] static QStringList missingSecretKeys(const Object& configuration);

	// removes what never belongs into a stored configuration: the secrets document of an export
	// and references of secret properties to other keys than their own; true if anything changed
	static bool scrubConfiguration(Object& configuration);

	// scrubs and migrates the stored configuration (after a one-time backup of it), saves it and
	// applies the changed values to the running configuration
	static Result migrateStoredConfiguration(Object& storedConfiguration, Object& runningConfiguration,
											 const CryptoCore& cryptoCore);

	// removes legacy values (e.g. from an export of an unprivileged process); returns their number
	static int removeLegacySecrets(Object& configuration);
};

}
