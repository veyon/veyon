/*
 * ServerAuthenticationManager.cpp - implementation of ServerAuthenticationManager
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthenticationCredentials.h"
#include "ServerAuthenticationManager.h"
#include "CryptoCore.h"
#include "Filesystem.h"
#include "PlatformUserFunctions.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"


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

	if( VeyonCore::config().authenticationMethod() == VeyonCore::KeyFileAuthentication )
	{
		authTypes.append( RfbVeyonAuth::KeyFile );
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::LogonAuthentication )
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
	case RfbVeyonAuth::KeyFile:
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

	switch( client->authState() )
	{
	case VncServerClient::AuthFinishedSuccess:
		emit authenticationDone( AuthResultSuccessful, client->hostAddress(), client->username() );
		break;
	case VncServerClient::AuthFinishedFail:
		emit authenticationDone( AuthResultFailed, client->hostAddress(), client->username() );
		break;
	default:
		break;
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
		if( VariantArrayMessage( message.ioDevice() ).write( client->challenge() ).send() == false )
		{
			qWarning( "ServerAuthenticationManager::performKeyAuthentication(): failed to send challenge" );
			return VncServerClient::AuthFinishedFail;
		}
		return VncServerClient::AuthChallenge;

	case VncServerClient::AuthChallenge:
	{
		// get authentication key name
		const auto authKeyName = message.read().toString(); // Flawfinder: ignore

		if( VeyonCore::isAuthenticationKeyNameValid( authKeyName ) == false )
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): invalid auth key name!" );
			return VncServerClient::AuthFinishedFail;
		}

		// now try to verify received signed data using public key of the user
		// under which the client claims to run
		const auto signature = message.read().toByteArray(); // Flawfinder: ignore

		const auto publicKeyPath = VeyonCore::filesystem().publicKeyPath( authKeyName );

		qDebug() << "ServerAuthenticationManager: loading public key" << publicKeyPath;
		CryptoCore::PublicKey publicKey( publicKeyPath );

		if( publicKey.isNull() || publicKey.isPublic() == false ||
				publicKey.verifyMessage( client->challenge(), signature, CryptoCore::DefaultSignatureAlgorithm ) == false )
		{
			qWarning( "ServerAuthenticationManager::performKeyAuthentication(): FAIL" );
			return VncServerClient::AuthFinishedFail;
		}

		qDebug( "ServerAuthenticationManager::performKeyAuthentication(): SUCCESS" );
		return VncServerClient::AuthFinishedSuccess;
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

		qDebug( "ServerAuthenticationManager::performLogonAuthentication(): failed to send public key" );
		return VncServerClient::AuthFinishedFail;
	}

	case VncServerClient::AuthPassword:
	{
		CryptoCore::PrivateKey privateKey = CryptoCore::PrivateKey::fromPEM( client->privateKey() );

		CryptoCore::SecureArray encryptedPassword( message.read().toByteArray() ); // Flawfinder: ignore

		CryptoCore::SecureArray decryptedPassword;

		if( privateKey.decrypt( encryptedPassword,
								&decryptedPassword,
								CryptoCore::DefaultEncryptionAlgorithm ) == false )
		{
			qWarning( "ServerAuthenticationManager::performLogonAuthentication(): failed to decrypt password" );
			return VncServerClient::AuthFinishedFail;
		}

		qInfo() << "ServerAuthenticationManager::performLogonAuthentication(): authenticating user" << client->username();

		if( VeyonCore::platform().userFunctions().authenticate( client->username(),
																QString::fromUtf8( decryptedPassword.toByteArray() ) ) )
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
	Q_UNUSED(message)

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
	{
		const auto token = message.read().toString();  // Flawfinder: ignore

		if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::Token ) &&
				token == VeyonCore::authenticationCredentials().token() )
		{
			qDebug( "ServerAuthenticationManager::performTokenAuthentication(): SUCCESS" );
			return VncServerClient::AuthFinishedSuccess;
		}

		qDebug( "ServerAuthenticationManager::performTokenAuthentication(): FAIL" );
		return VncServerClient::AuthFinishedFail;
	}

	default:
		break;
	}

	return VncServerClient::AuthFinishedFail;
}
