/*
 * AuthenticationCredentials.h - class holding credentials for authentication
 *
 * Copyright (c) 2010-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "CryptoCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT AuthenticationCredentials
{
public:
	using Token = CryptoCore::SecureArray;
	using Password = CryptoCore::SecureArray;

	enum class Type
	{
		None = 0x00,
		PrivateKey = 0x01,
		UserLogon = 0x02,
		Token = 0x04,
		AllTypes = PrivateKey | UserLogon | Token
	} ;

	Q_DECLARE_FLAGS(TypeFlags, Type)

	AuthenticationCredentials();
	AuthenticationCredentials( const AuthenticationCredentials &other );

	bool hasCredentials( Type credentialType ) const;

	// private key auth
	bool loadPrivateKey( const QString& privateKeyFile );
	bool setPrivateKey( const CryptoCore::PrivateKey& privateKey );
	const CryptoCore::PrivateKey& privateKey() const
	{
		return m_privateKey;
	}

	const QString& authenticationKeyName() const
	{
		return m_authenticationKeyName;
	}

	void setAuthenticationKeyName( const QString& authenticationKeyName )
	{
		m_authenticationKeyName = authenticationKeyName;
	}

	// user logon auth
	void setLogonUsername( const QString& username )
	{
		m_logonUsername = username;
	}

	void setLogonPassword( const Password& password )
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

	void setToken( const Token& token )
	{
		m_token = token;
	}

	const Token& token() const
	{
		return m_token;
	}

	void setInternalVncServerPassword( const Password& password )
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

} ;
