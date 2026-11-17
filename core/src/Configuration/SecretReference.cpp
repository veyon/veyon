// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/SecretReference.h"
#include "SecretStore.h"


namespace Configuration {

static constexpr auto SecretReferencePrefix = QLatin1String("secret:");


SecretReference::SecretReference(const QString& key) :
	m_key(SecretStore::isValidKey(key) ? key : QString())
{
}



QString SecretReference::toString() const
{
	if (isValid() == false)
	{
		return {};
	}

	return SecretReferencePrefix + m_key;
}



SecretReference SecretReference::fromString(const QString& value)
{
	if (value.startsWith(SecretReferencePrefix) == false)
	{
		return {};
	}

	return SecretReference(value.mid(SecretReferencePrefix.size()));
}



QVariant SecretReference::defaultValue(const QString& key)
{
	return SecretReference(key).toString();
}



bool SecretReference::isLegacyValue(const QString& value)
{
	return value.isEmpty() == false && value.startsWith(SecretReferencePrefix) == false;
}

}
