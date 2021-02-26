/*
 * AuthenticationCredentials.cpp - class holding credentials for authentication
 *
 * Copyright (c) 2010-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include "AuthenticationCredentials.h"


AuthenticationCredentials::AuthenticationCredentials() :
	m_privateKey(),
	m_logonUsername(),
	m_logonPassword(),
	m_token(),
	m_internalVncServerPassword()
{
}



AuthenticationCredentials::AuthenticationCredentials( const AuthenticationCredentials &other ) :
	m_privateKey( other.privateKey() ),
	m_authenticationKeyName( other.authenticationKeyName() ),
	m_logonUsername( other.logonUsername() ),
	m_logonPassword( other.logonPassword() ),
	m_token( other.token() ),
	m_internalVncServerPassword( other.internalVncServerPassword() )
{
}



bool AuthenticationCredentials::hasCredentials( Type type ) const
{
	switch( type )
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

	vCritical() << "no valid credential type given:" << TypeFlags( type );

	return false;
}



bool AuthenticationCredentials::loadPrivateKey( const QString& privateKeyFile )
{
	vDebug() << privateKeyFile;

	if( privateKeyFile.isEmpty() )
	{
		return false;
	}

	return setPrivateKey( CryptoCore::PrivateKey( privateKeyFile ) );
}



bool AuthenticationCredentials::setPrivateKey( const CryptoCore::PrivateKey& privateKey )
{
	if( privateKey.isNull() == false && privateKey.isPrivate() )
	{
		m_privateKey = privateKey;

		return true;
	}

	return false;
}
