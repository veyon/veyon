// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Password.h"


namespace Configuration {

Password Password::fromEncrypted(const QString& encrypted)
{
	Password password;
	password.m_encrypted = encrypted;
	return password;
}

}
