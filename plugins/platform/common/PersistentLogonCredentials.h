// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

// clazy:exclude=copyable-polymorphic

class PersistentLogonCredentials
{
public:
	using Password = CryptoCore::SecureArray;

	static bool read(QString* username, Password* password);
	static bool write(const QString& username, const Password& password);
};
