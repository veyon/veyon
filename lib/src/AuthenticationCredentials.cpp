/*
 * AuthenticationCredentials.cpp - class holding credentials for authentication
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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
#include "DsaKey.h"
#include "Logger.h"


AuthenticationCredentials::AuthenticationCredentials() :
	m_privateKey( NULL ),
	m_logonUsername(),
	m_logonPassword(),
	m_commonSecret()
{
}



AuthenticationCredentials::AuthenticationCredentials( const AuthenticationCredentials &other ) :
	m_privateKey( NULL ),
	m_logonUsername( other.logonUsername() ),
	m_logonPassword( other.logonPassword() ),
	m_commonSecret( other.commonSecret() )
{
}



bool AuthenticationCredentials::hasCredentials( TypeFlags credentialType ) const
{
	if( credentialType & PrivateKey )
	{
		return m_privateKey && m_privateKey->isValid();
	}

	if( credentialType & UserLogon )
	{
		return m_logonUsername.isEmpty() == false &&
				m_logonPassword.isEmpty() == false;
	}

	if( credentialType & CommonSecret )
	{
		return !m_commonSecret.isEmpty() &&
				QByteArray::fromBase64( m_commonSecret.toAscii() ).size() ==
												DsaKey::DefaultChallengeSize;
	}

	ilog_failedf( "credential type check", "%d", credentialType );

	return false;
}



bool AuthenticationCredentials::loadPrivateKey( const QString &privKeyFile )
{
	if( m_privateKey )
	{
		delete m_privateKey;
		m_privateKey = NULL;
	}

	if( privKeyFile.isEmpty() )
	{
		return false;
	}

	m_privateKey = new PrivateDSAKey( privKeyFile );

	return m_privateKey->isValid();
}



void AuthenticationCredentials::setLogonUsername( const QString &username )
{
	m_logonUsername = username;
}



void AuthenticationCredentials::setLogonPassword( const QString &password )
{
	m_logonPassword = password;
}

