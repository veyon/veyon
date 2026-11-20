// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Property.h"
#include "Configuration/SecretPropertyRegistry.h"
#include "PluginInterface.h"
#include "SecretStore.h"

#include <array>

namespace Configuration {

static constexpr std::array<QLatin1String, 4> ReservedScopes{
	QLatin1String("core"),
	QLatin1String("ipc"),
	QLatin1String("pki"),
	QLatin1String("tls"),
};


// rows of the SecretReference properties of core itself (reserved scopes only); there are none
// yet, e.g. TLS/Enrollment/Token (tls/enrollmentToken) is added together with its property
static SecretPropertyList coreSecretProperties()
{
	return {};
}



static SecretPropertyList& registeredProperties()
{
	static SecretPropertyList properties = coreSecretProperties();
	return properties;
}



const SecretPropertyList& SecretPropertyRegistry::properties()
{
	return registeredProperties();
}



std::optional<SecretProperty> SecretPropertyRegistry::find(const QString& parentKey, const QString& key)
{
	for (const auto& secretProperty : std::as_const(registeredProperties()))
	{
		if (QString::compare(parentKey, secretProperty.parentKey, Qt::CaseInsensitive) == 0 &&
			QString::compare(key, secretProperty.key, Qt::CaseInsensitive) == 0)
		{
			return secretProperty;
		}
	}

	return {};
}



bool SecretPropertyRegistry::add(const SecretProperty& secretProperty, const QString& providerName)
{
	QString key;
	QString parentKey;
	if (splitConfigurationKey(secretProperty.propertyKey(), key, parentKey) == false)
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << providerName
					<< "- invalid property key";
		return false;
	}

	if (SecretStore::isValidKey(secretProperty.secretKey) == false)
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << providerName
					<< "- invalid secret key" << secretProperty.secretKey;
		return false;
	}

	if (isReservedScope(secretProperty.secretKey))
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << providerName
					<< "- secret key" << secretProperty.secretKey << "uses a reserved scope";
		return false;
	}

	for (const auto& registeredProperty : std::as_const(registeredProperties()))
	{
		const auto sameProperty = QString::compare(registeredProperty.propertyKey(), secretProperty.propertyKey(),
												   Qt::CaseInsensitive) == 0;
		const auto sameSecretKey = registeredProperty.secretKey == secretProperty.secretKey;
		if (sameProperty == false && sameSecretKey == false)
		{
			continue;
		}

		// the same row again, e.g. plugins loaded by another VeyonCore instance in this process
		if (sameProperty && sameSecretKey && registeredProperty.parentKey == secretProperty.parentKey &&
			registeredProperty.key == secretProperty.key &&
			registeredProperty.isLegacyPassword == secretProperty.isLegacyPassword)
		{
			return true;
		}

		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << providerName
					<< "- conflicts with the registered secret property" << registeredProperty.propertyKey()
					<< registeredProperty.secretKey;
		return false;
	}

	registeredProperties().append(secretProperty);

	return true;
}



void SecretPropertyRegistry::addPluginProperties(const PluginInterface& plugin)
{
	const auto pluginProperties = plugin.secretProperties();
	for (const auto& secretProperty : pluginProperties)
	{
		(void)add(secretProperty, plugin.name());
	}
}



bool SecretPropertyRegistry::isReservedScope(const QString& secretKey)
{
	const auto scope = secretKey.section(QLatin1Char('/'), 0, 0);
	for (const auto& reservedScope : ReservedScopes)
	{
		if (scope == reservedScope)
		{
			return true;
		}
	}

	return false;
}



SecretReference SecretPropertyRegistry::ownReference(const Property& property)
{
	const auto secretProperty = find(property.parentKey(), property.key());
	if (secretProperty.has_value() == false ||
		property.defaultValue() != SecretReference::defaultValue(secretProperty->secretKey))
	{
		vCritical() << "secret property" << property.absoluteKey() << "is not registered with its secret key";
		return {};
	}

	return SecretReference(secretProperty->secretKey);
}



QStringList SecretPropertyRegistry::secretKeysBelow(const QString& fullKey)
{
	QStringList secretKeys;

	for (const auto& secretProperty : std::as_const(registeredProperties()))
	{
		if (QString::compare(secretProperty.propertyKey(), fullKey, Qt::CaseInsensitive) == 0 ||
			QString::compare(secretProperty.parentKey, fullKey, Qt::CaseInsensitive) == 0 ||
			secretProperty.parentKey.startsWith(QString(fullKey + QLatin1Char('/')), Qt::CaseInsensitive))
		{
			secretKeys.append(secretProperty.secretKey);
		}
	}

	return secretKeys;
}



QStringList SecretPropertyRegistry::secretKeys()
{
	QStringList secretKeys;
	secretKeys.reserve(registeredProperties().size());

	for (const auto& secretProperty : std::as_const(registeredProperties()))
	{
		secretKeys.append(secretProperty.secretKey);
	}

	return secretKeys;
}



#if defined(VEYON_WITH_TESTS)
void SecretPropertyRegistry::resetForTests()
{
	registeredProperties() = coreSecretProperties();
}
#endif

}
