// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Password.h"
#include "CryptoCore.h"


namespace Configuration {

CryptoCore::PlaintextPassword Password::plainText() const
{
	return VeyonCore::cryptoCore().decryptPassword(m_encrypted);
}



Password Password::fromPlainText(const CryptoCore::PlaintextPassword& plaintext)
{
	Password password;
	password.m_encrypted = VeyonCore::cryptoCore().encryptPassword(plaintext);
	return password;
}



Password Password::fromEncrypted(const QString& encrypted)
{
	Password password;
	password.m_encrypted = encrypted;
	return password;
}

}
