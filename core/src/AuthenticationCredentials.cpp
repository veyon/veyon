// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AuthenticationCredentials.h"


AuthenticationCredentials::AuthenticationCredentials() :
	m_privateKey(),
	m_logonUsername(),
	m_logonPassword(),
	m_token(),
	m_internalVncServerPassword()
{
}



AuthenticationCredentials::AuthenticationCredentials(const AuthenticationCredentials& other) :
	m_privateKey(other.privateKey()),
	m_authenticationKeyName(other.authenticationKeyName()),
	m_logonUsername(other.logonUsername()),
	m_logonPassword(other.logonPassword()),
	m_token(other.token()),
	m_internalVncServerPassword(other.internalVncServerPassword())
{
}



bool AuthenticationCredentials::hasCredentials(Type type) const
{
	switch (type)
	{
	case Type::PrivateKey:
		return m_privateKey.isNull() == false;

	case Type::UserLogon:
		return m_logonUsername.isEmpty() == false && m_logonPassword.isEmpty() == false;

	case Type::Token:
		return m_token.isEmpty() == false && m_token.size() == CryptoCore::ChallengeSize;

	default:
		break;
	}

	vCritical() << "no valid credential type given:" << TypeFlags(type);

	return false;
}



bool AuthenticationCredentials::loadPrivateKey(const QString& privateKeyFile)
{
	vDebug() << privateKeyFile;

	if (privateKeyFile.isEmpty())
	{
		return false;
	}

	return setPrivateKey(CryptoCore::PrivateKey(privateKeyFile));
}



bool AuthenticationCredentials::setPrivateKey(const CryptoCore::PrivateKey& privateKey)
{
	if (privateKey.isNull() == false && privateKey.isPrivate())
	{
		m_privateKey = privateKey;

		return true;
	}

	return false;
}
