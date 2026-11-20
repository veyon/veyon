// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <QVariant>

#include "VeyonCore.h"

namespace Configuration {

// Configuration value referring to a secret in the SecretStore (03-secretstore-config.md §3). The
// configuration stores the string "secret:<key>" only, never the secret itself.
//
// Each property of this type has a fixed store key, given as its default value
// (SecretReference::defaultValue()). A stored value never selects a different key, so a modified
// configuration (import, GPO) cannot redirect a property to another secret.
class VEYON_CORE_EXPORT SecretReference
{
	Q_GADGET
public:
	SecretReference() = default;

	// invalid (empty key) if key is not a valid SecretStore key
	explicit SecretReference(const QString& key);

	const QString& key() const
	{
		return m_key;
	}

	bool isValid() const
	{
		return m_key.isEmpty() == false;
	}

	QString toString() const;

	// "secret:<valid key>", otherwise an invalid reference
	static SecretReference fromString(const QString& value);

	// default value for the OP() entry of a SecretReference property
	static QVariant defaultValue(const QString& key);

	// value with the "secret:" prefix, whether its key is valid or not
	static bool isReference(const QString& value);

	// non-empty value of a former Configuration::Password property (RSA ciphertext with the key
	// from default-pkey.pem, or a plain value of a very old version), see LegacySecretMigration
	static bool isLegacyValue(const QString& value);

private:
	QString m_key;
};

}

Q_DECLARE_METATYPE(Configuration::SecretReference)
