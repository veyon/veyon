// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QStringList>

#include <array>

namespace Configuration {

// Static registry of all configuration properties holding a SecretReference and their fixed
// SecretStore keys (03-secretstore-config.md §5). It is the only source for deciding whether a
// configuration key is a secret (veyon-cli config set/unset/clear, exports and imports of secrets,
// scrubbing at privileged start), independent of which plugins are loaded.
//
// Every SecretReference property must have a row here with its default value set to
// SecretReference::defaultValue(secretKey). Rows with isLegacyPassword = true were
// Configuration::Password properties before 4.12 and are migrated by LegacySecretMigration.
// Plugins with secrets (e.g. the Entra ID connector, WP15, with its former password property)
// add their rows here.
struct SecretProperty
{
	QLatin1String parentKey;
	QLatin1String key;
	QLatin1String secretKey;
	bool isLegacyPassword;
};

inline constexpr std::array<SecretProperty, 2> SecretProperties{{
	{QLatin1String("LDAP"), QLatin1String("BindPassword"), QLatin1String("ldap/bindPassword"), true},
	{QLatin1String("ExternalVncServer"), QLatin1String("Password"), QLatin1String("vncserver-external/password"), true},
}};

// case-insensitive like the registry-based configuration on Windows
inline const SecretProperty* findSecretProperty(const QString& parentKey, const QString& key)
{
	for (const auto& secretProperty : SecretProperties)
	{
		if (QString::compare(parentKey, secretProperty.parentKey, Qt::CaseInsensitive) == 0 &&
			QString::compare(key, secretProperty.key, Qt::CaseInsensitive) == 0)
		{
			return &secretProperty;
		}
	}

	return nullptr;
}



// splits "Group/Sub/Key" into key and parent key; false for empty components (e.g. a leading or
// doubled "/"), which would otherwise bypass findSecretProperty()
inline bool splitConfigurationKey(const QString& fullKey, QString& key, QString& parentKey)
{
	const auto keyParts = fullKey.split(QLatin1Char('/'));
	for (const auto& keyPart : keyParts)
	{
		if (keyPart.isEmpty())
		{
			return false;
		}
	}

	key = keyParts.last();
	parentKey = keyParts.mid(0, keyParts.size() - 1).join(QLatin1Char('/'));

	return true;
}



// store keys of the secret property fullKey or of all secret properties in the group fullKey
inline QStringList secretKeysBelow(const QString& fullKey)
{
	QStringList secretKeys;

	for (const auto& secretProperty : SecretProperties)
	{
		const QString parentKey = secretProperty.parentKey;
		const QString propertyKey = parentKey + QLatin1Char('/') + secretProperty.key;
		if (QString::compare(propertyKey, fullKey, Qt::CaseInsensitive) == 0 ||
			QString::compare(parentKey, fullKey, Qt::CaseInsensitive) == 0 ||
			parentKey.startsWith(QString(fullKey + QLatin1Char('/')), Qt::CaseInsensitive))
		{
			secretKeys.append(secretProperty.secretKey);
		}
	}

	return secretKeys;
}

}
