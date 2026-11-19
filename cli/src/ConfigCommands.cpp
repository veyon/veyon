// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QScopeGuard>

#include "CommandLineIO.h"
#include "ConfigCommands.h"
#include "Configuration/JsonStore.h"
#include "Configuration/LegacySecretMigration.h"
#include "Configuration/SecretProperties.h"
#include "ConfigurationManager.h"
#include "ConstantTime.h"
#include "CryptoCore.h"
#include "SecretExport.h"
#include "SecretInputOutput.h"
#include "SecretStore.h"

#ifndef Q_OS_WIN
#include <sys/stat.h>
#endif

namespace ConfigCommandOptions {
inline constexpr auto IncludeSecrets = QLatin1String("--include-secrets");
inline constexpr auto Kdf = QLatin1String("--kdf");
inline constexpr auto PassphraseFile = QLatin1String("--passphrase-file");
}

// configuration exports with large builtin directories may be a few MiB
static constexpr qint64 MaximumConfigurationFileSize = 64 * 1024 * 1024;


ConfigCommands::ConfigCommands(QObject* parent) :
	QObject(parent),
	m_commands({
		{QStringLiteral("clear"), tr("Clear system-wide Veyon configuration")},
		{QStringLiteral("list"), tr("List all configuration keys and values")},
		{QStringLiteral("import"), tr("Import configuration from given file (secrets with --include-secrets "
									  "[--passphrase-file FILE])")},
		{QStringLiteral("export"), tr("Export configuration to given file (secrets with --include-secrets "
									  "[--kdf scrypt|argon2id] [--passphrase-file FILE])")},
		{QStringLiteral("export-premigration"), tr("Export the configuration saved before the migration of "
												   "legacy passwords (for downgrades, contains these passwords)")},
		{QStringLiteral("get"), tr("Read and output configuration value for given key")},
		{QStringLiteral("set"), tr("Write given value to given configuration key")},
		{QStringLiteral("unset"), tr("Unset (remove) given configuration key")},
		{QStringLiteral("upgrade"), tr("Upgrade and save configuration of program and plugins")},
	})
{
}



QStringList ConfigCommands::commands() const
{
	return m_commands.keys();
}



QString ConfigCommands::commandHelp(const QString& command) const
{
	return m_commands.value(command);
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_clear(const QStringList& arguments)
{
	Q_UNUSED(arguments);

	// a cleared configuration must not keep old secrets effective
	if (SecretStore::isAvailable())
	{
		const auto storedKeys = SecretStore::keys();
		QStringList removedSecretKeys;
		for (const auto& secretProperty : Configuration::SecretProperties)
		{
			if (SecretStore::remove(secretProperty.secretKey) == false)
			{
				return operationError(tr("Could not remove secret %1.").arg(secretProperty.secretKey));
			}
			if (storedKeys.contains(secretProperty.secretKey))
			{
				removedSecretKeys.append(secretProperty.secretKey);
			}
		}
		if (removedSecretKeys.isEmpty() == false)
		{
			CommandLineIO::info(tr("Removed secrets: %1").arg(removedSecretKeys.join(QStringLiteral(", "))));
		}
		// kept on purpose: the only way back to Veyon 4.11
		const auto preMigrationKey = Configuration::LegacySecretMigration::preMigrationConfigurationKey();
		if (storedKeys.contains(preMigrationKey))
		{
			CommandLineIO::info(tr("The pre-migration configuration backup (secret %1) has been kept for a downgrade "
								   "to Veyon 4.11. It is not a configuration property and can't be removed with "
								   "'config unset'. If it is no longer needed, save it with 'config export-premigration' "
								   "if desired and delete it from the secret storage manually.")
									.arg(preMigrationKey));
		}
	}
	else
	{
		CommandLineIO::warning(tr("Stored secrets have not been removed (requires administrator privileges)."));
	}

	if (ConfigurationManager().clearConfiguration())
	{
		return Successful;
	}

	return Failed;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_list(const QStringList& arguments)
{
	auto listMode = ListMode::Values;

	if (arguments.value(0) == QLatin1String("defaults"))
	{
		listMode = ListMode::Defaults;
	}
	else if (arguments.value(0) == QLatin1String("types"))
	{
		listMode = ListMode::Types;
	}

	listConfiguration(listMode);

	return NoResult;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_import(const QStringList& arguments)
{
	const auto options = parseOptions(arguments);
	if (options.has_value() == false)
	{
		return InvalidArguments;
	}

	const auto& fileName = options->fileName;

	if (fileName.isEmpty() || QFile(fileName).exists() == false)
	{
		return operationError(tr("Please specify an existing configuration file to import."));
	}

	if (QFileInfo(fileName).isReadable() == false)
	{
		return operationError(tr("Configuration file is not readable!"));
	}

	QFile file(fileName);
	if (file.open(QFile::ReadOnly) == false || file.size() > MaximumConfigurationFileSize)
	{
		return operationError(tr("Could not read the configuration file."));
	}

	QJsonParseError parseError{};
	const auto jsonDocument = QJsonDocument::fromJson(file.readAll(), &parseError);
	if (parseError.error != QJsonParseError::NoError || jsonDocument.isObject() == false)
	{
		return operationError(tr("The configuration file is invalid."));
	}

	// the secrets document is never loaded into the configuration (JsonStore skips it)
	const auto json = jsonDocument.object();
	const auto hasSecretsDocument = json.contains(SecretExport::configurationKey());
	Configuration::Object importedConfiguration;
	Configuration::JsonStore::loadJsonObject(&importedConfiguration, json);

	// decrypt and check everything before anything is changed
	std::optional<SecretExport::Secrets> secrets;
	if (hasSecretsDocument && options->includeSecrets)
	{
		secrets = decryptSecrets(json.value(SecretExport::configurationKey()).toObject(), *options);
		if (secrets.has_value() == false)
		{
			return Failed;
		}
	}
	else if (hasSecretsDocument)
	{
		CommandLineIO::info(tr("Ignoring the encrypted secrets of the configuration file (use %1 to import them).")
								.arg(ConfigCommandOptions::IncludeSecrets));
	}
	else if (options->includeSecrets)
	{
		CommandLineIO::warning(tr("The configuration file contains no secrets."));
	}

	// never store legacy ciphertext (readable with the public default-pkey.pem) again
	const auto hasLegacySecrets = Configuration::LegacySecretMigration::hasLegacySecrets(importedConfiguration);
	if (hasLegacySecrets && SecretStore::isAvailable() == false)
	{
		return operationError(tr("The configuration file contains legacy encrypted passwords. "
								 "Importing it requires administrator privileges so that they "
								 "can be moved to the secure secret storage."));
	}

	Configuration::LegacySecretMigration::scrubConfiguration(importedConfiguration);

	// an explicit import replaces stored secrets - tell the admin which ones
	QStringList replacedSecretKeys;
	const auto replacedSecretsWarning = qScopeGuard([&replacedSecretKeys]() {
		if (replacedSecretKeys.isEmpty() == false)
		{
			CommandLineIO::warning(tr("The following stored secrets have been replaced with the imported values: %1")
									   .arg(replacedSecretKeys.join(QStringLiteral(", "))));
		}
	});

	const auto storedSecretKeys = secrets.has_value() ? SecretStore::keys() : QStringList{};

	if (hasLegacySecrets &&
		Configuration::LegacySecretMigration::migrate(importedConfiguration, VeyonCore::cryptoCore(),
													  Configuration::LegacySecretMigration::ExistingSecrets::Overwrite,
													  &replacedSecretKeys) != Configuration::LegacySecretMigration::Result::Migrated)
	{
		return operationError(tr("Could not migrate the legacy encrypted passwords of the configuration file."));
	}

	if (secrets.has_value())
	{
		for (const auto& entry : *secrets)
		{
			if (SecretStore::write(entry.first, entry.second) == false)
			{
				return operationError(tr("Could not store secret %1.").arg(entry.first));
			}
			if (storedSecretKeys.contains(entry.first) && replacedSecretKeys.contains(entry.first) == false)
			{
				replacedSecretKeys.append(entry.first);
			}
		}
	}

	// merge configuration
	VeyonCore::config() += importedConfiguration;

	if (secrets.has_value() == false && SecretStore::isAvailable())
	{
		const auto missingSecretKeys = Configuration::LegacySecretMigration::missingSecretKeys(VeyonCore::config());
		if (missingSecretKeys.isEmpty() == false)
		{
			CommandLineIO::warning(tr("The configuration references secrets that are not stored: %1 - import them "
									  "with %2 or set them with 'config set'")
									   .arg(missingSecretKeys.join(QStringLiteral(", ")), ConfigCommandOptions::IncludeSecrets));
		}
	}

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_export(const QStringList& arguments)
{
	const auto options = parseOptions(arguments);
	if (options.has_value() == false)
	{
		return InvalidArguments;
	}

	const auto& fileName = options->fileName;

	if (fileName.isEmpty())
	{
		return operationError(tr("Please specify a valid filename for the configuration export."));
	}

	QFileInfo fileInfo(fileName);
	if (fileInfo.exists() && fileInfo.isWritable() == false)
	{
		return operationError(tr("Output file is not writable!"));
	}

	if (fileInfo.exists() == false && QFileInfo(fileInfo.dir().path()).isWritable() == false)
	{
		return operationError(tr("Output directory is not writable!"));
	}

	// the export contains references to secrets only (03-secretstore-config.md §4.6)
	Configuration::Object exportedConfiguration(VeyonCore::config());
	if (Configuration::LegacySecretMigration::removeLegacySecrets(exportedConfiguration) > 0)
	{
		CommandLineIO::warning(tr("Legacy encrypted passwords have not been exported. Run Veyon "
								  "with administrator privileges once to move them to the secure "
								  "secret storage."));
	}

	auto json = Configuration::JsonStore::toJsonObject(&exportedConfiguration);

	if (options->includeSecrets)
	{
		const auto secretsDocument = exportSecrets(*options);
		if (secretsDocument.has_value() == false)
		{
			return Failed;
		}
		json[SecretExport::configurationKey()] = *secretsDocument;
	}

	QString errorString;
	if (SecretInputOutput::writeFile(fileName, QJsonDocument(json).toJson(), options->includeSecrets, &errorString) == false)
	{
		return operationError(tr("Could not write the configuration file: %1").arg(errorString));
	}

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_export_premigration(const QStringList& arguments)
{
	const auto fileName = arguments.value(0);
	if (fileName.isEmpty() || arguments.size() > 1)
	{
		return operationError(tr("Please specify a valid filename for the configuration export."));
	}

	if (SecretStore::isAvailable() == false)
	{
		return operationError(tr("Exporting the pre-migration configuration requires administrator privileges."));
	}

	const auto backup = SecretStore::read(Configuration::LegacySecretMigration::preMigrationConfigurationKey());
	if (backup.has_value() == false)
	{
		return operationError(tr("No configuration has been saved before a migration of legacy passwords."));
	}

	// contains the legacy encrypted passwords, i.e. readable with the public default key
	QString errorString;
	if (SecretInputOutput::writeFile(fileName, backup->toUnprotectedByteArray(), true, &errorString) == false)
	{
		return operationError(tr("Could not write the configuration file: %1").arg(errorString));
	}

	CommandLineIO::warning(tr("The file contains the legacy encrypted passwords - delete it after restoring."));

	return Successful;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_get(const QStringList& arguments)
{
	QString key = arguments.value(0);

	if (key.isEmpty())
	{
		return operationError(tr("Please specify a valid key."));
	}

	QString parentKey;
	if (Configuration::splitConfigurationKey(arguments.value(0), key, parentKey) == false)
	{
		return operationError(tr("Please specify a valid key."));
	}

	if (VeyonCore::config().hasValue(key, parentKey) == false)
	{
		return operationError(tr("Specified key does not exist in current configuration!"));
	}

	CommandLineIO::print(printableConfigurationValue(VeyonCore::config().value(key, parentKey, QVariant())));

	return NoResult;
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_set(const QStringList& arguments)
{
	auto key = arguments.value(0);
	auto value = arguments.value(1);
	auto type = arguments.value(2);

	if (key.isEmpty())
	{
		return operationError(tr("Please specify a valid key."));
	}

	QString parentKey;
	if (Configuration::splitConfigurationKey(arguments.value(0), key, parentKey) == false)
	{
		return operationError(tr("Please specify a valid key."));
	}

	const auto secretProperty = Configuration::findSecretProperty(parentKey, key);
	if (secretProperty)
	{
		return setSecret(*secretProperty, arguments.size() > 1 ? std::optional<QString>(value) : std::nullopt);
	}

	auto valueType = QMetaType::UnknownType;
	const auto property = Configuration::Property::find(&VeyonCore::config(), key, parentKey);

	if (value.isEmpty())
	{
		return operationError(tr("Please specify a valid value."));
	}

	if (property)
	{
		valueType = static_cast<QMetaType::Type>(property->variantValue().userType());
	}

	QVariant configValue = value;

	if (type == QLatin1String("json") ||
		valueType == QMetaType::QJsonArray)
	{
		configValue = QJsonDocument::fromJson(value.toUtf8()).array();
	}
	else if (type == QLatin1String("password"))
	{
		// passwords are never stored in the configuration (03-secretstore-config.md §3)
		return operationError(tr("The specified key is not a secret configuration property. "
								 "Passwords can't be stored in the configuration."));
	}
	else if (type == QLatin1String("list") ||
			 valueType == QMetaType::QStringList)
	{
		configValue = value.split(QLatin1Char(';'));
	}

	VeyonCore::config().setValue(key, configValue, parentKey);

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_unset(const QStringList& arguments)
{
	QString key = arguments.value(0);

	if (key.isEmpty())
	{
		return operationError(tr("Please specify a valid key."));
	}

	QString parentKey;
	if (Configuration::splitConfigurationKey(arguments.value(0), key, parentKey) == false)
	{
		return operationError(tr("Please specify a valid key."));
	}

	// a secret property or a group containing secret properties: remove their secrets as well
	const auto fullKey = parentKey.isEmpty() ? key : QString(parentKey + QLatin1Char('/') + key);
	const auto removedSecretKeys = Configuration::secretKeysBelow(fullKey);
	if (removedSecretKeys.isEmpty() == false)
	{
		if (SecretStore::isAvailable() == false)
		{
			return operationError(tr("Removing secrets requires administrator privileges."));
		}
		for (const auto& secretKey : removedSecretKeys)
		{
			if (SecretStore::remove(secretKey) == false)
			{
				return operationError(tr("Could not remove secret %1.").arg(secretKey));
			}
		}
		CommandLineIO::info(tr("Removed secrets: %1").arg(removedSecretKeys.join(QStringLiteral(", "))));
	}

	VeyonCore::config().removeValue(key, parentKey);

	return applyConfiguration();
}



CommandLinePluginInterface::RunResult ConfigCommands::handle_upgrade(const QStringList& arguments)
{
	Q_UNUSED(arguments);

	// upgrade already happened while loading plugins so only save upgraded configuration
	return applyConfiguration();
}



std::optional<ConfigCommands::Options> ConfigCommands::parseOptions(const QStringList& arguments)
{
	Options options;

	for (int i = 0; i < arguments.size(); ++i)
	{
		const auto& argument = arguments[i];
		if (argument == ConfigCommandOptions::IncludeSecrets)
		{
			options.includeSecrets = true;
		}
		else if (argument == ConfigCommandOptions::Kdf && i + 1 < arguments.size())
		{
			const auto kdf = arguments[++i];
			if (kdf == QLatin1String("argon2id"))
			{
				if (Veyon::Crypto::PasswordKdf::isArgon2idAvailable() == false)
				{
					CommandLineIO::error(tr("Argon2id requires OpenSSL 3.2 or newer."));
					return {};
				}
				options.kdf = SecretExport::Kdf::Argon2id;
			}
			else if (kdf != QLatin1String("scrypt"))
			{
				CommandLineIO::error(tr("Invalid key derivation function %1 (supported: scrypt, argon2id).").arg(kdf));
				return {};
			}
		}
		else if (argument == ConfigCommandOptions::PassphraseFile && i + 1 < arguments.size())
		{
			options.passphraseFile = arguments[++i];
		}
		else if (argument.startsWith(QLatin1String("--")) || options.fileName.isEmpty() == false)
		{
			CommandLineIO::error(tr("Invalid argument %1.").arg(argument));
			return {};
		}
		else
		{
			options.fileName = argument;
		}
	}

	return options;
}



QStringList ConfigCommands::configuredSecretKeys()
{
	QStringList keys;

	for (const auto& secretProperty : Configuration::SecretProperties)
	{
		keys.append(secretProperty.secretKey);
	}

	return keys;
}



std::optional<Veyon::Crypto::SecureBuffer> ConfigCommands::readSecretInput(const QString& prompt, bool confirm)
{
	Veyon::Crypto::SecureBuffer secret;
	const auto result = SecretInputOutput::readStandardInput(prompt, secret);
	if (result == SecretInputOutput::LineResult::Invalid)
	{
		CommandLineIO::error(tr("Could not read the input."));
		return {};
	}
	// removing a secret is done with "config unset" only
	if (result == SecretInputOutput::LineResult::NoInput || secret.isEmpty())
	{
		CommandLineIO::error(tr("No value given."));
		return {};
	}

	if (confirm && SecretInputOutput::isStandardInputTerminal())
	{
		Veyon::Crypto::SecureBuffer confirmation;
		if (SecretInputOutput::readStandardInput(tr("Repeat: "), confirmation) != SecretInputOutput::LineResult::Read ||
			Veyon::Crypto::constantTimeEquals(secret, confirmation) == false)
		{
			CommandLineIO::error(tr("The entered values do not match."));
			return {};
		}
	}

	return secret;
}



std::optional<Veyon::Crypto::SecureBuffer> ConfigCommands::readPassphrase(const Options& options, bool confirm)
{
	std::optional<Veyon::Crypto::SecureBuffer> passphrase;

	if (options.passphraseFile.isEmpty())
	{
		passphrase = readSecretInput(tr("Passphrase: "), confirm);
	}
	else
	{
		QFile passphraseFile(options.passphraseFile);
#ifndef Q_OS_WIN
		struct stat status{};
		if (stat(QFile::encodeName(options.passphraseFile).constData(), &status) != 0 || (status.st_mode & 0007) != 0)
		{
			CommandLineIO::error(tr("The passphrase file must exist and must not be accessible by other users."));
			return {};
		}
#endif
		if (passphraseFile.open(QFile::ReadOnly) == false)
		{
			CommandLineIO::error(tr("Could not open the passphrase file."));
			return {};
		}
		Veyon::Crypto::SecureBuffer line;
		if (SecretInputOutput::readLine(int(passphraseFile.handle()), line) != SecretInputOutput::LineResult::Read)
		{
			CommandLineIO::error(tr("Could not read the passphrase file."));
			return {};
		}
		passphrase = std::move(line);
	}

	if (passphrase.has_value() && SecretExport::isAcceptablePassphrase(*passphrase) == false)
	{
		CommandLineIO::error(tr("The passphrase must have at least %1 characters.").arg(SecretExport::MinimumPassphraseLength));
		return {};
	}

	return passphrase;
}



std::optional<QJsonObject> ConfigCommands::exportSecrets(const Options& options)
{
	if (SecretStore::isAvailable() == false)
	{
		CommandLineIO::error(tr("Exporting secrets requires administrator privileges."));
		return {};
	}

	// only secrets referenced by the configuration, never host keys or CA keys
	SecretExport::Secrets secrets;
	const auto keys = configuredSecretKeys();
	for (const auto& key : keys)
	{
		auto secret = SecretStore::read(key);
		if (secret.has_value())
		{
			secrets.emplace(key, std::move(*secret));
		}
	}

	if (secrets.empty())
	{
		CommandLineIO::warning(tr("No secrets are stored."));
	}

	const auto passphrase = readPassphrase(options, true);
	if (passphrase.has_value() == false)
	{
		return {};
	}

	const auto document = SecretExport::encrypt(secrets, *passphrase, options.kdf);
	if (document.has_value() == false)
	{
		CommandLineIO::error(tr("Could not encrypt the secrets."));
	}

	return document;
}



std::optional<SecretExport::Secrets> ConfigCommands::decryptSecrets(const QJsonObject& secretsDocument, const Options& options)
{
	if (SecretStore::isAvailable() == false)
	{
		CommandLineIO::error(tr("Importing secrets requires administrator privileges."));
		return {};
	}

	const auto passphrase = readPassphrase(options, false);
	if (passphrase.has_value() == false)
	{
		return {};
	}

	auto secrets = SecretExport::decrypt(secretsDocument, *passphrase);
	if (secrets.has_value() == false)
	{
		CommandLineIO::error(tr("Could not decrypt the secrets (wrong passphrase or damaged file)."));
		return {};
	}

	// never import other secrets such as host or CA keys
	const auto allowedKeys = configuredSecretKeys();
	for (const auto& entry : *secrets)
	{
		if (allowedKeys.contains(entry.first) == false)
		{
			CommandLineIO::error(tr("The configuration file contains an unexpected secret %1.").arg(entry.first));
			return {};
		}
	}

	return secrets;
}



CommandLinePluginInterface::RunResult ConfigCommands::setSecret(const Configuration::SecretProperty& secretProperty,
																const std::optional<QString>& value)
{
	if (SecretStore::isAvailable() == false)
	{
		return operationError(tr("Writing secrets requires administrator privileges."));
	}

	const Configuration::SecretReference reference{QString(secretProperty.secretKey)};
	const QString propertyKey = secretProperty.parentKey + QLatin1Char('/') + secretProperty.key;

	std::optional<Veyon::Crypto::SecureBuffer> secret;
	if (value.has_value() == false || *value == QLatin1String("-"))
	{
		secret = readSecretInput(tr("Value for %1: ").arg(propertyKey), true);
	}
	else if (value->isEmpty())
	{
		return operationError(tr("No value given. Use \"config unset\" to remove a secret."));
	}
	else
	{
		CommandLineIO::warning(tr("Secrets passed on the command line are visible to other processes. "
								  "Omit the value to enter it interactively or via standard input."));
		auto plainText = value->toUtf8();
		secret = Veyon::Crypto::SecureBuffer::fromByteArray(plainText);
		plainText.fill(0);
	}

	if (secret.has_value() == false)
	{
		return Failed;
	}

	if (SecretStore::write(reference.key(), *secret) == false)
	{
		return operationError(tr("Could not store the secret."));
	}

	VeyonCore::config().setValue(secretProperty.key, reference.toString(), secretProperty.parentKey);

	return applyConfiguration();
}



void ConfigCommands::listConfiguration(ListMode listMode) const
{
	QTextStream stdoutStream(stdout);

	const auto properties = VeyonCore::config().findChildren<Configuration::Property*>();

	for (auto property : properties)
	{
		if (property->flags().testFlag(Configuration::Property::Flag::Legacy))
		{
			continue;
		}

		stdoutStream << property->absoluteKey() << "=";
		switch (listMode)
		{
		case ListMode::Values:
			stdoutStream << printableConfigurationValue(property->variantValue());
			break;
		case ListMode::Defaults:
			stdoutStream << printableConfigurationValue(property->defaultValue());
			break;
		case ListMode::Types:
			stdoutStream << QStringLiteral("[%1]").arg(QString::fromLatin1(
														   property->defaultValue().typeName())
														   .replace(QLatin1Char('Q'), QString())
														   .toLower());
			break;
		}
		stdoutStream << Qt::endl;
	}
}



CommandLinePluginInterface::RunResult ConfigCommands::applyConfiguration()
{
	ConfigurationManager configurationManager;

	if (configurationManager.saveConfiguration() == false ||
		configurationManager.applyConfiguration() == false)
	{
		return operationError(configurationManager.errorString());
	}

	return Successful;
}



QString ConfigCommands::printableConfigurationValue(const QVariant& value)
{
	if (value.userType() == QMetaType::QString ||
		value.userType() == QMetaType::QUuid ||
		value.userType() == QMetaType::UInt ||
		value.userType() == QMetaType::Int ||
		value.userType() == QMetaType::Bool)
	{
		return value.toString();
	}

	if (value.userType() == QMetaType::QStringList)
	{
		return value.toStringList().join(QLatin1Char(';'));
	}

	if (value.userType() == QMetaType::QJsonArray)
	{
		return QString::fromUtf8(QJsonDocument(value.toJsonArray()).toJson(QJsonDocument::Compact));
	}

	if (value.userType() == QMetaType::QJsonObject)
	{
		return QString::fromUtf8(QJsonDocument(value.toJsonObject()).toJson(QJsonDocument::Compact));
	}

	if (QMetaType(value.userType()).flags().testFlag(QMetaType::IsEnumeration))
	{
		return QString::number(value.toInt());
	}

	return {};
}



CommandLinePluginInterface::RunResult ConfigCommands::operationError(const QString& message)
{
	CommandLineIO::error(message);

	return Failed;
}
