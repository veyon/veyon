// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>
#include <QString>

namespace Configuration {

// a configuration property holding a SecretReference and its one fixed SecretStore key
// (03-secretstore-config.md §3a); isLegacyPassword marks former Configuration::Password
// properties, whose values are migrated by LegacySecretMigration
struct SecretProperty
{
	QString parentKey;
	QString key;
	QString secretKey;
	bool isLegacyPassword = false;

	QString propertyKey() const
	{
		return parentKey + QLatin1Char('/') + key;
	}
};

using SecretPropertyList = QList<SecretProperty>;

}
