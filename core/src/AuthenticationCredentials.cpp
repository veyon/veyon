/*
 * AuthenticationCredentials.cpp - class holding credentials for authentication
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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
	m_privateKey(),
	m_logonUsername( other.logonUsername() ),
	m_logonPassword( other.logonPassword() ),
	m_token( other.token() ),
	m_internalVncServerPassword( other.internalVncServerPassword() )
{
}



bool AuthenticationCredentials::hasCredentials( TypeFlags credentialType ) const
{
	if( credentialType & PrivateKey )
	{
		return m_privateKey.isNull() == false;
	}

	if( credentialType & UserLogon )
	{
		return m_logonUsername.isEmpty() == false &&
				m_logonPassword.isEmpty() == false;
	}

	if( credentialType & Token )
	{
		return m_token.isEmpty() == false &&
				QByteArray::fromBase64( m_token.toUtf8() ).size() == CryptoCore::ChallengeSize;
	}

	qCritical( "AuthenticationCredentials::hasCredentials(): no valid credential type given: %d", credentialType );

	return false;
}



bool AuthenticationCredentials::loadPrivateKey( const QString& privateKeyFile )
{
	qDebug() << "AuthenticationCredentials: loading private key" << privateKeyFile;

	if( privateKeyFile.isEmpty() )
	{
		return false;
	}

	m_privateKey = CryptoCore::PrivateKey( privateKeyFile );

	return m_privateKey.isNull() == false && m_privateKey.isPrivate();
}
