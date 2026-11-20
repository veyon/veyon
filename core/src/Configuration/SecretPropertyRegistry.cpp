// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMutex>

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



namespace {

struct RegisteredRow
{
	SecretProperty secretProperty;
	Plugin::Uid providerUid; // null for rows of core
	QString providerName;
};

struct Registry
{
	QList<RegisteredRow> rows;
	SecretPropertyList properties;
	// lower-case keys of rejected conflicting rows, so that the result does not depend on the order
	// in which the rows are added
	QStringList conflictingPropertyKeys;
	QStringList conflictingSecretKeys;

	Registry()
	{
		reset();
	}

	void reset()
	{
		rows.clear();
		const auto coreProperties = coreSecretProperties();
		for (const auto& secretProperty : coreProperties)
		{
			rows.append({secretProperty, {}, QStringLiteral("core")});
		}
		conflictingPropertyKeys.clear();
		conflictingSecretKeys.clear();
		updateProperties();
	}

	void updateProperties()
	{
		properties.clear();
		properties.reserve(rows.size());
		for (const auto& row : std::as_const(rows))
		{
			properties.append(row.secretProperty);
		}
	}
};

// properties logged by ownReference() already
QMutex loggedPropertiesMutex;
QStringList loggedProperties;

}



static Registry& registry()
{
	static Registry registry;
	return registry;
}



// a copy which does not share data with the given string, which may be a QStringLiteral in the
// library of a plugin
static QString deepCopy(const QString& string)
{
	return string.isNull() ? QString() : QString(string.constData(), string.size());
}



static QString describeProvider(Plugin::Uid providerUid, const QString& providerName)
{
	return providerUid.isNull() ? providerName : QStringLiteral("%1 (%2)").arg(providerName, providerUid.toString(QUuid::WithoutBraces));
}



const SecretPropertyList& SecretPropertyRegistry::properties()
{
	return registry().properties;
}



std::optional<SecretProperty> SecretPropertyRegistry::find(const QString& parentKey, const QString& key)
{
	for (const auto& secretProperty : std::as_const(registry().properties))
	{
		if (QString::compare(parentKey, secretProperty.parentKey, Qt::CaseInsensitive) == 0 &&
			QString::compare(key, secretProperty.key, Qt::CaseInsensitive) == 0)
		{
			return secretProperty;
		}
	}

	return {};
}



bool SecretPropertyRegistry::add(const SecretProperty& declaredProperty, Plugin::Uid providerUid, const QString& declaredProviderName)
{
	const SecretProperty secretProperty{deepCopy(declaredProperty.parentKey), deepCopy(declaredProperty.key),
										deepCopy(declaredProperty.secretKey), declaredProperty.isLegacyPassword};
	const auto providerName = deepCopy(declaredProviderName);
	const auto provider = describeProvider(providerUid, providerName);

	QString key;
	QString parentKey;
	if (splitConfigurationKey(secretProperty.propertyKey(), key, parentKey) == false)
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << provider
					<< "- invalid property key";
		return false;
	}

	if (SecretStore::isValidKey(secretProperty.secretKey) == false)
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << provider
					<< "- invalid secret key" << secretProperty.secretKey;
		return false;
	}

	if (isReservedScope(secretProperty.secretKey))
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << provider
					<< "- secret key" << secretProperty.secretKey << "uses a reserved scope";
		return false;
	}

	auto& rows = registry().rows;
	const auto propertyKey = secretProperty.propertyKey().toLower();
	const auto secretKey = secretProperty.secretKey.toLower();

	auto conflicts = registry().conflictingPropertyKeys.contains(propertyKey) ||
		registry().conflictingSecretKeys.contains(secretKey);
	if (conflicts)
	{
		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << "of" << provider
					<< "- conflicts with secret properties rejected before";
	}

	QList<qsizetype> conflictingRows;
	for (qsizetype i = 0; i < rows.size(); ++i)
	{
		const auto& registeredRow = rows.at(i);
		const auto& registeredProperty = registeredRow.secretProperty;
		const auto sameProperty = registeredProperty.propertyKey().toLower() == propertyKey;
		const auto sameSecretKey = registeredProperty.secretKey.toLower() == secretKey;
		if (sameProperty == false && sameSecretKey == false)
		{
			continue;
		}

		// the same row again from the same provider, e.g. plugins loaded by another VeyonCore
		// instance in this process
		if (conflicts == false && registeredRow.providerUid.isNull() == false && registeredRow.providerUid == providerUid &&
			registeredProperty.parentKey == secretProperty.parentKey && registeredProperty.key == secretProperty.key &&
			registeredProperty.secretKey == secretProperty.secretKey &&
			registeredProperty.isLegacyPassword == secretProperty.isLegacyPassword)
		{
			return true;
		}

		vCritical() << "rejecting secret property" << secretProperty.propertyKey() << secretProperty.secretKey << "of" << provider
					<< "- conflicts with the secret property" << registeredProperty.propertyKey()
					<< registeredProperty.secretKey << "of" << describeProvider(registeredRow.providerUid, registeredRow.providerName);
		conflicts = true;

		// core rows always win
		if (registeredRow.providerUid.isNull() == false)
		{
			conflictingRows.append(i);
		}
	}

	if (conflicts == false)
	{
		rows.append({secretProperty, providerUid, providerName});
		registry().updateProperties();
		return true;
	}

	registry().conflictingPropertyKeys.append(propertyKey);
	registry().conflictingSecretKeys.append(secretKey);

	for (auto it = conflictingRows.crbegin(); it != conflictingRows.crend(); ++it)
	{
		const auto registeredRow = rows.takeAt(*it);
		vCritical() << "rejecting the registered secret property" << registeredRow.secretProperty.propertyKey()
					<< registeredRow.secretProperty.secretKey << "of"
					<< describeProvider(registeredRow.providerUid, registeredRow.providerName)
					<< "- conflicts with the secret property" << secretProperty.propertyKey() << secretProperty.secretKey
					<< "of" << provider;
		registry().conflictingPropertyKeys.append(registeredRow.secretProperty.propertyKey().toLower());
		registry().conflictingSecretKeys.append(registeredRow.secretProperty.secretKey.toLower());
	}

	registry().updateProperties();

	return false;
}



void SecretPropertyRegistry::addPluginProperties(const PluginInterface& plugin)
{
	const auto pluginProperties = plugin.secretProperties();
	for (const auto& secretProperty : pluginProperties)
	{
		(void)add(secretProperty, plugin.uid(), plugin.name());
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
		const auto absoluteKey = property.absoluteKey();
		QMutexLocker locker(&loggedPropertiesMutex);
		if (loggedProperties.contains(absoluteKey) == false)
		{
			loggedProperties.append(absoluteKey);
			vCritical() << "secret property" << absoluteKey << "is not registered with its secret key - disabled";
		}
		return {};
	}

	return SecretReference(secretProperty->secretKey);
}



QStringList SecretPropertyRegistry::secretKeysBelow(const QString& fullKey)
{
	QStringList secretKeys;

	for (const auto& secretProperty : std::as_const(registry().properties))
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
	secretKeys.reserve(registry().properties.size());

	for (const auto& secretProperty : std::as_const(registry().properties))
	{
		secretKeys.append(secretProperty.secretKey);
	}

	return secretKeys;
}



std::optional<QStringList> SecretPropertyRegistry::removeUnreservedSecrets(QString* failedKey)
{
	QStringList removedKeys;

	const auto storedKeys = SecretStore::keys();
	for (const auto& secretKey : storedKeys)
	{
		if (isReservedScope(secretKey))
		{
			continue;
		}

		if (SecretStore::remove(secretKey) == false)
		{
			vCritical() << "failed to remove secret" << secretKey;
			if (failedKey)
			{
				*failedKey = secretKey;
			}
			return {};
		}

		vInfo() << "removed secret" << secretKey;
		removedKeys.append(secretKey);
	}

	return removedKeys;
}



#if defined(VEYON_WITH_TESTS)
void SecretPropertyRegistry::resetForTests()
{
	registry().reset();

	QMutexLocker locker(&loggedPropertiesMutex);
	loggedProperties.clear();
}
#endif

}
