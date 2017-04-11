/*
 * ServerAuthenticationManager.cpp - implementation of ServerAuthenticationManager
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QHostAddress>

#include "ServerAuthenticationManager.h"
#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "LogonAuthentication.h"
#include "VariantArrayMessage.h"


ServerAuthenticationManager::ServerAuthenticationManager( QObject* parent ) :
	QObject( parent ),
	m_allowedIPs(),
	m_failedAuthHosts()
{


}



QVector<RfbVeyonAuth::Type> ServerAuthenticationManager::supportedAuthTypes() const
{
	QVector<RfbVeyonAuth::Type> authTypes;

	authTypes.append( RfbVeyonAuth::HostWhiteList );

	if( VeyonCore::config().isKeyAuthenticationEnabled() )
	{
		authTypes.append( RfbVeyonAuth::DSA );
	}

	if( VeyonCore::config().isLogonAuthenticationEnabled() )
	{
		authTypes.append( RfbVeyonAuth::Logon );
	}

	if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::Token ) )
	{
		authTypes.append( RfbVeyonAuth::Token );
	}

	return authTypes;
}



void ServerAuthenticationManager::processAuthenticationMessage( VncServerClient* client,
																VariantArrayMessage& message )
{
	qDebug() << "ServerAuthenticationManager::processAuthenticationMessage():"
			 << "state" << client->authState()
			 << "type" << client->authType()
			 << "host" << client->hostAddress()
			 << "user" << client->username();

	switch( client->authType() )
	{
	// no authentication
	case RfbVeyonAuth::None:
		client->setAuthState( VncServerClient::AuthFinishedSuccess );
		break;

		// host has to be in list of allowed hosts
	case RfbVeyonAuth::HostWhiteList:
		client->setAuthState( performHostWhitelistAuth( client, message ) );
		break;

		// authentication via DSA-challenge/-response
	case RfbVeyonAuth::DSA:
		client->setAuthState( performKeyAuthentication( client, message ) );
		break;

	case RfbVeyonAuth::Logon:
		client->setAuthState( performLogonAuthentication( client, message ) );
		break;

	case RfbVeyonAuth::Token:
		client->setAuthState( performTokenAuthentication( client, message ) );
		break;

	default:
		break;
	}

	if( client->authState() == VncServerClient::AuthFinishedFail )
	{
		emit authenticationError( client->hostAddress(), client->username() );
	}
}



void ServerAuthenticationManager::setAllowedIPs(const QStringList &allowedIPs)
{
	QMutexLocker l( &m_dataMutex );
	m_allowedIPs = allowedIPs;
}



VncServerClient::AuthState ServerAuthenticationManager::performKeyAuthentication( VncServerClient* client,
																				  VariantArrayMessage& message )
{
	switch( client->authState() )
	{
	case VncServerClient::AuthInit:
		client->setChallenge( CryptoCore::generateChallenge() );
		if( VariantArrayMessage( message.ioDevice() ).write( client->challenge() ).send() )
		{
			return VncServerClient::AuthChallenge;
		}
		else
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): failed to send challenge" );
			return VncServerClient::AuthFinishedFail;
		}
		break;

	case VncServerClient::AuthChallenge:
	{
		// get user role
		const VeyonCore::UserRoles urole = static_cast<VeyonCore::UserRoles>( message.read().toInt() );

		// now try to verify received signed data using public key of the user
		// under which the client claims to run
		const QByteArray signature = message.read().toByteArray();

		qDebug() << "Loading public key" << LocalSystem::Path::publicKeyPath( urole ) << "for role" << urole;
		// (publicKeyPath does range-checking of urole)
		CryptoCore::PublicKey publicKey( LocalSystem::Path::publicKeyPath( urole ) );

		if( publicKey.verifyMessage( client->challenge(), signature, CryptoCore::DefaultSignatureAlgorithm ) )
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): SUCCESS" );
			return VncServerClient::AuthFinishedSuccess;
		}
		else
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): FAIL" );
			return VncServerClient::AuthFinishedFail;
		}
		break;
	}

	default:
		break;
	}

	return VncServerClient::AuthFinishedFail;
}



VncServerClient::AuthState ServerAuthenticationManager::performLogonAuthentication( VncServerClient* client,
																					VariantArrayMessage& message )
{
	switch( client->authState() )
	{
	case VncServerClient::AuthInit:
	{
		CryptoCore::PrivateKey privateKey = CryptoCore::KeyGenerator().createRSA( CryptoCore::RsaKeySize );

		client->setPrivateKey( privateKey.toPEM() );

		CryptoCore::PublicKey publicKey = privateKey.toPublicKey();

		if( VariantArrayMessage( message.ioDevice() ).write( publicKey.toPEM() ).send() )
		{
			return VncServerClient::AuthPassword;
		}
		else
		{
			qDebug( "ServerAuthenticationManager::performLogonAuthentication(): failed to send public key" );
			return VncServerClient::AuthFinishedFail;
		}
	}

	case VncServerClient::AuthPassword:
	{
		CryptoCore::PrivateKey privateKey = CryptoCore::PrivateKey::fromPEM( client->privateKey() );

		CryptoCore::SecureArray encryptedPassword( message.read().toByteArray() );

		CryptoCore::SecureArray decryptedPassword;

		if( privateKey.decrypt( encryptedPassword,
								&decryptedPassword,
								CryptoCore::DefaultEncryptionAlgorithm ) == false )
		{
			qWarning( "ServerAuthenticationManager::performLogonAuthentication(): failed to decrypt password" );
			return VncServerClient::AuthFinishedFail;
		}

		AuthenticationCredentials credentials;
		credentials.setLogonUsername( client->username() );
		credentials.setLogonPassword( QString::fromUtf8( decryptedPassword.toByteArray() ) );

		if( LogonAuthentication::authenticateUser( credentials ) )
		{
			qDebug( "ServerAuthenticationManager::performLogonAuthentication(): SUCCESS" );
			return VncServerClient::AuthFinishedSuccess;
		}

		qDebug( "ServerAuthenticationManager::performLogonAuthentication(): FAIL" );
		return VncServerClient::AuthFinishedFail;
	}

	default:
		break;
	}

	return VncServerClient::AuthFinishedFail;
}


VncServerClient::AuthState ServerAuthenticationManager::performHostWhitelistAuth( VncServerClient* client,
																				  VariantArrayMessage& message )
{
	QMutexLocker l( &m_dataMutex );

	if( m_allowedIPs.isEmpty() )
	{
		qWarning( "ServerAuthenticationManager: empty list of allowed IPs" );
		return VncServerClient::AuthFinishedFail;
	}

	if( m_allowedIPs.contains( client->hostAddress() ) )
	{
		qDebug( "ServerAuthenticationManager::performHostWhitelistAuth(): SUCCESS" );
		return VncServerClient::AuthFinishedSuccess;
	}

	qWarning( "ServerAuthenticationManager::performHostWhitelistAuth(): FAIL" );

	// authentication failed
	return VncServerClient::AuthFinishedFail;
}



VncServerClient::AuthState ServerAuthenticationManager::performTokenAuthentication( VncServerClient* client,
																					VariantArrayMessage& message )
{
	switch( client->authState() )
	{
	case VncServerClient::AuthInit:
		return VncServerClient::AuthToken;

	case VncServerClient::AuthToken:
		if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::Token ) &&
				message.read().toString() == VeyonCore::authenticationCredentials().token() )
		{
			qDebug( "ServerAuthenticationManager::performTokenAuthentication(): SUCCESS" );
			return VncServerClient::AuthFinishedSuccess;
		}

		qDebug( "ServerAuthenticationManager::performTokenAuthentication(): FAIL" );
		return VncServerClient::AuthFinishedFail;

	default:
		break;
	}

	return VncServerClient::AuthFinishedFail;
}
