// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QStringList>

#include "Configuration/SecretProperty.h"
#include "Configuration/SecretReference.h"
#include "VeyonCore.h"

#include <optional>
#include <veyonconfig.h>

class PluginInterface;

namespace Configuration {

class Property;

// Registry of all configuration properties holding a SecretReference and their fixed SecretStore
// keys (03-secretstore-config.md §3a). It is the only source for deciding whether a configuration
// key is a secret (veyon-cli config set/unset/clear, exports and imports of secrets, scrubbing and
// the legacy migration at privileged start) and which key a property refers to, so that a modified
// configuration can never redirect a property to another secret.
//
// It holds the rows of core (reserved scopes only) plus the rows declared by the loaded plugins
// (PluginInterface::secretProperties()), added by PluginManager::loadPlugins() at startup, i.e.
// before anything uses the registry. A configuration value of a property which is not registered
// (e.g. of a plugin which is not installed) is never resolved and left untouched.
class VEYON_CORE_EXPORT SecretPropertyRegistry
{
public:
	static const SecretPropertyList& properties();

	// case-insensitive like the registry-based configuration on Windows
	static std::optional<SecretProperty> find(const QString& parentKey, const QString& key);

	// adds a row unless its secret key is invalid or in a reserved scope, or its property or
	// secret key is registered already (first registration wins); rejected rows are logged
	static bool add(const SecretProperty& secretProperty, const QString& providerName);

	static void addPluginProperties(const PluginInterface& plugin);

	// "core", "ipc", "pki" and "tls" are used by core only
	static bool isReservedScope(const QString& secretKey);

	// reference to the registered key of a SecretReference property, invalid (and logged) if the
	// property is not registered or its default value names a different key
	static SecretReference ownReference(const Property& property);

	// store keys of the secret property fullKey or of all secret properties in the group fullKey
	static QStringList secretKeysBelow(const QString& fullKey);

	static QStringList secretKeys();

#if defined(VEYON_WITH_TESTS)
	// removes all rows added by plugins
	static void resetForTests();
#endif
};



// splits "Group/Sub/Key" into key and parent key; false for empty components (e.g. a leading or
// doubled "/"), which would otherwise bypass SecretPropertyRegistry::find()
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

}
