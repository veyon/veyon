// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT AuthenticationCredentials
{
public:
	using Token = CryptoCore::SecureArray;
	using Password = CryptoCore::SecureArray;

	enum class Type {
		None = 0x00,
		PrivateKey = 0x01,
		UserLogon = 0x02,
		Token = 0x04,
		AllTypes = PrivateKey | UserLogon | Token
	};

	Q_DECLARE_FLAGS(TypeFlags, Type)

	AuthenticationCredentials();
	AuthenticationCredentials(const AuthenticationCredentials& other);

	bool hasCredentials(Type credentialType) const;

	// private key auth
	bool loadPrivateKey(const QString& privateKeyFile);
	bool setPrivateKey(const CryptoCore::PrivateKey& privateKey);
	const CryptoCore::PrivateKey& privateKey() const
	{
		return m_privateKey;
	}

	const QString& authenticationKeyName() const
	{
		return m_authenticationKeyName;
	}

	void setAuthenticationKeyName(const QString& authenticationKeyName)
	{
		m_authenticationKeyName = authenticationKeyName;
	}

	// user logon auth
	void setLogonUsername(const QString& username)
	{
		m_logonUsername = username;
	}

	void setLogonPassword(const Password& password)
	{
		m_logonPassword = password;
	}

	const QString& logonUsername() const
	{
		return m_logonUsername;
	}

	const Password& logonPassword() const
	{
		return m_logonPassword;
	}

	void setToken(const Token& token)
	{
		m_token = token;
	}

	const Token& token() const
	{
		return m_token;
	}

	void setInternalVncServerPassword(const Password& password)
	{
		m_internalVncServerPassword = password;
	}

	const Password& internalVncServerPassword() const
	{
		return m_internalVncServerPassword;
	}

private:
	CryptoCore::PrivateKey m_privateKey;
	QString m_authenticationKeyName;

	QString m_logonUsername;
	Password m_logonPassword;

	Token m_token;

	Password m_internalVncServerPassword;
};
