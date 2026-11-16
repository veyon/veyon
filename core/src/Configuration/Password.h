// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

namespace Configuration {

class VEYON_CORE_EXPORT Password
{
	Q_GADGET
public:
	Password() = default;

	CryptoCore::PlaintextPassword plainText() const;

	const QString& encrypted() const
	{
		return m_encrypted;
	}

	static Password fromPlainText(const CryptoCore::PlaintextPassword& plainText);
	static Password fromEncrypted(const QString& encrypted);

private:
	QString m_encrypted;
};

}

Q_DECLARE_METATYPE(Configuration::Password)
