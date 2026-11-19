// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonDocument>

#include "Configuration/JsonStore.h"
#include "Configuration/LegacySecretMigration.h"
#include "Configuration/Object.h"
#include "Configuration/Password.h"
#include "Configuration/SecretProperties.h"
#include "Configuration/SecretReference.h"
#include "CryptoCore.h"
#include "SecretExport.h"
#include "SecretStore.h"

#include <optional>

namespace Configuration {

// values of this length are plain text left behind by Veyon versions without encryption (see the
// former upgrade paths of the LDAP and external VNC server plugins); RSA-4096 ciphertexts are
// 1024 hex digits long
static constexpr int MaximumPlaintextPasswordLength = 64;

static constexpr auto CoreGroup = QLatin1String("Core");
static constexpr auto SecretMigrationVersionKey = QLatin1String("SecretMigrationVersion");


static QString storedValue(const Object& configuration, const SecretProperty& legacySecret)
{
	return configuration.value(legacySecret.key, legacySecret.parentKey, {}).toString();
}



static std::optional<Veyon::Crypto::SecureBuffer> decryptLegacyValue(const QString& legacyValue, const CryptoCore& cryptoCore)
{
	if (legacyValue.size() < MaximumPlaintextPasswordLength)
	{
		auto plainText = legacyValue.toUtf8();
		auto secret = Veyon::Crypto::SecureBuffer::fromByteArray(plainText);
		plainText.fill(0);
		return secret;
	}

	bool ok = false;
	const auto plainText = cryptoCore.decryptPassword(Password::fromEncrypted(legacyValue).encrypted(), &ok);
	if (ok == false)
	{
		return {};
	}

	return Veyon::Crypto::SecureBuffer(plainText.constData(), plainText.size());
}



bool LegacySecretMigration::hasLegacySecrets(const Object& configuration)
{
	for (const auto& secretProperty : SecretProperties)
	{
		if (secretProperty.isLegacyPassword && SecretReference::isLegacyValue(storedValue(configuration, secretProperty)))
		{
			return true;
		}
	}

	return false;
}



LegacySecretMigration::Result LegacySecretMigration::migrate(Object& configuration, const CryptoCore& cryptoCore,
															 ExistingSecrets existingSecrets, QStringList* replacedSecretKeys)
{
	if (hasLegacySecrets(configuration) == false)
	{
		return Result::NothingToMigrate;
	}

	if (SecretStore::isAvailable() == false)
	{
		vWarning() << "can't migrate legacy secrets without access to the secret store";
		return Result::Failed;
	}

	const auto storedKeys = SecretStore::keys();

	auto result = Result::Migrated;

	for (const auto& secretProperty : SecretProperties)
	{
		const auto legacyValue = storedValue(configuration, secretProperty);
		if (secretProperty.isLegacyPassword == false || SecretReference::isLegacyValue(legacyValue) == false)
		{
			continue;
		}

		const QString propertyKey = secretProperty.parentKey + QLatin1Char('/') + secretProperty.key;
		const auto reference = SecretReference(secretProperty.secretKey).toString();

		// a legacy value (e.g. re-applied by a GPO or configuration management) never replaces a
		// stored secret, which may have been rotated since, unless an admin explicitly imports it
		if (storedKeys.contains(secretProperty.secretKey))
		{
			if (existingSecrets == ExistingSecrets::Keep)
			{
				vWarning() << "ignoring legacy encrypted value of" << propertyKey
						   << "- keeping the stored secret" << secretProperty.secretKey << "- update the policy";
				configuration.setValue(secretProperty.key, reference, secretProperty.parentKey);
				continue;
			}
			vWarning() << "replacing stored secret" << secretProperty.secretKey << "with the imported legacy value of"
					   << propertyKey;
		}

		const auto secret = decryptLegacyValue(legacyValue, cryptoCore);
		if (secret.has_value() == false)
		{
			vCritical() << "failed to decrypt legacy value of" << propertyKey;
			result = Result::Failed;
			continue;
		}

		// an empty password is "no secret"
		const auto stored = secret->isEmpty() ? SecretStore::remove(secretProperty.secretKey) : SecretStore::write(secretProperty.secretKey, *secret);
		if (stored == false)
		{
			vCritical() << "failed to store secret" << secretProperty.secretKey << "- keeping legacy value of" << propertyKey;
			result = Result::Failed;
			continue;
		}

		configuration.setValue(secretProperty.key, reference, secretProperty.parentKey);
		vInfo() << "migrated" << propertyKey << "to secret" << secretProperty.secretKey;

		if (replacedSecretKeys && storedKeys.contains(secretProperty.secretKey) &&
			replacedSecretKeys->contains(secretProperty.secretKey) == false)
		{
			replacedSecretKeys->append(secretProperty.secretKey);
		}
	}

	if (result == Result::Migrated)
	{
		configuration.setValue(SecretMigrationVersionKey, MigrationVersion, CoreGroup);
	}

	return result;
}



QStringList LegacySecretMigration::missingSecretKeys(const Object& configuration)
{
	const auto storedKeys = SecretStore::keys();

	QStringList missingKeys;
	for (const auto& secretProperty : SecretProperties)
	{
		const auto reference = SecretReference::fromString(storedValue(configuration, secretProperty));
		if (reference.isValid() && storedKeys.contains(reference.key()) == false &&
			missingKeys.contains(reference.key()) == false)
		{
			missingKeys.append(reference.key());
		}
	}

	return missingKeys;
}



bool LegacySecretMigration::scrubConfiguration(Object& configuration)
{
	auto changed = false;

	if (configuration.hasValue(SecretExport::configurationKey(), {}))
	{
		vWarning() << "removing secrets document of an export from the configuration";
		configuration.removeValue(SecretExport::configurationKey(), {});
		changed = true;
	}

	for (const auto& secretProperty : SecretProperties)
	{
		const auto value = storedValue(configuration, secretProperty);
		const auto ownReference = SecretReference(secretProperty.secretKey).toString();
		if (value.isEmpty() == false && SecretReference::isLegacyValue(value) == false && value != ownReference)
		{
			vWarning() << "replacing reference to a different secret in" << secretProperty.parentKey << secretProperty.key;
			configuration.setValue(secretProperty.key, ownReference, secretProperty.parentKey);
			changed = true;
		}
	}

	return changed;
}



LegacySecretMigration::Result LegacySecretMigration::migrateStoredConfiguration(Object& storedConfiguration,
																				Object& runningConfiguration,
																				const CryptoCore& cryptoCore)
{
	if (SecretStore::isAvailable() == false)
	{
		return hasLegacySecrets(storedConfiguration) ? Result::Failed : Result::NothingToMigrate;
	}

	const auto scrubbed = scrubConfiguration(storedConfiguration);
	scrubConfiguration(runningConfiguration);

	auto result = scrubbed ? Result::Migrated : Result::NothingToMigrate;

	if (hasLegacySecrets(storedConfiguration))
	{
		if (storedConfiguration.value(SecretMigrationVersionKey, CoreGroup, 0).toInt() >= MigrationVersion)
		{
			vWarning() << "legacy encrypted secret re-applied by configuration management - update the policy";
		}

		if (SecretStore::keys().contains(preMigrationConfigurationKey()) == false)
		{
			const auto backup = QJsonDocument(JsonStore::toJsonObject(&storedConfiguration)).toJson();
			if (backup.size() > Veyon::Crypto::SecureBuffer::MaximumSize ||
				SecretStore::write(preMigrationConfigurationKey(), Veyon::Crypto::SecureBuffer::fromByteArray(backup)) == false)
			{
				vCritical() << "failed to back up the configuration - not migrating legacy secrets";
				return Result::Failed;
			}
		}

		result = migrate(storedConfiguration, cryptoCore);
	}

	if (result == Result::NothingToMigrate)
	{
		return result;
	}

	// save migrated values even after partial failures: the remaining legacy values are retried
	// at the next start
	if (storedConfiguration.isStoreWritable() == false)
	{
		vCritical() << "configuration not writable - legacy secrets remain in the configuration";
		return Result::Failed;
	}
	storedConfiguration.flushStore();

	for (const auto& secretProperty : SecretProperties)
	{
		const auto value = storedValue(storedConfiguration, secretProperty);
		if (SecretReference::fromString(value).isValid())
		{
			runningConfiguration.setValue(secretProperty.key, value, secretProperty.parentKey);
		}
	}
	runningConfiguration.setValue(SecretMigrationVersionKey,
								  storedConfiguration.value(SecretMigrationVersionKey, CoreGroup, 0), CoreGroup);

	return result;
}



int LegacySecretMigration::removeLegacySecrets(Object& configuration)
{
	int count = 0;

	for (const auto& secretProperty : SecretProperties)
	{
		if (secretProperty.isLegacyPassword && SecretReference::isLegacyValue(storedValue(configuration, secretProperty)))
		{
			configuration.removeValue(secretProperty.key, secretProperty.parentKey);
			++count;
		}
	}

	return count;
}

}
