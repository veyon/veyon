// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

namespace Configuration {

// Read-only legacy type: value of a password property of Veyon < 4.12 (hex encoded RSA ciphertext
// with the public key of default-pkey.pem, i.e. obfuscation only). New code stores secrets in the
// SecretStore and uses SecretReference properties; LegacySecretMigration is the only reader.
class VEYON_CORE_EXPORT Password
{
	Q_GADGET
public:
	Password() = default;

	const QString& encrypted() const
	{
		return m_encrypted;
	}

	static Password fromEncrypted(const QString& encrypted);

private:
	QString m_encrypted;
};

}

Q_DECLARE_METATYPE(Configuration::Password)
