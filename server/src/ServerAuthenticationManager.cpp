/*
 * ServerAuthenticationManager.cpp - implementation of ServerAuthenticationManager
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthenticationCredentials.h"
#include "ServerAuthenticationManager.h"
#include "CryptoCore.h"
#include "Filesystem.h"
#include "PlatformUserFunctions.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"


ServerAuthenticationManager::ServerAuthenticationManager( QObject* parent ) :
	QObject( parent )
{
}



QVector<RfbVeyonAuth::Type> ServerAuthenticationManager::supportedAuthTypes() const
{
	QVector<RfbVeyonAuth::Type> authTypes;

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication )
	{
		authTypes.append( RfbVeyonAuth::KeyFile );
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::LogonAuthentication )
	{
		authTypes.append( RfbVeyonAuth::Logon );
	}

	if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::Type::Token ) )
	{
		authTypes.append( RfbVeyonAuth::Token );
	}

	return authTypes;
}



void ServerAuthenticationManager::processAuthenticationMessage( VncServerClient* client,
																VariantArrayMessage& message )
{
	vDebug() << "state" << client->authState()
			 << "type" << client->authType()
			 << "host" << client->hostAddress()
			 << "user" << client->username();

	switch( client->authType() )
	{
	// no authentication
	case RfbVeyonAuth::None:
		client->setAuthState( VncServerClient::AuthState::Successful );
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
		// unknown or unsupported auth type
		client->setAuthState( VncServerClient::AuthState::Failed );
		break;
	}

	switch( client->authState() )
	{
	case VncServerClient::AuthState::Failed:
	case VncServerClient::AuthState::Successful:
		Q_EMIT finished( client );
		break;
	default:
		break;
	}
}



VncServerClient::AuthState ServerAuthenticationManager::performKeyAuthentication( VncServerClient* client,
																				  VariantArrayMessage& message )
{
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		client->setChallenge( CryptoCore::generateChallenge() );
		if( VariantArrayMessage( message.ioDevice() ).write( client->challenge() ).send() == false )
		{
			vWarning() << "failed to send challenge";
			return VncServerClient::AuthState::Failed;
		}
		return VncServerClient::AuthState::Challenge;

	case VncServerClient::AuthState::Challenge:
	{
		// get authentication key name
		const auto authKeyName = message.read().toString(); // Flawfinder: ignore

		if( VeyonCore::isAuthenticationKeyNameValid( authKeyName ) == false )
		{
			vDebug() << "invalid auth key name!";
			return VncServerClient::AuthState::Failed;
		}

		// now try to verify received signed data using public key of the user
		// under which the client claims to run
		const auto signature = message.read().toByteArray(); // Flawfinder: ignore

		const auto publicKeyPath = VeyonCore::filesystem().publicKeyPath( authKeyName );

		CryptoCore::PublicKey publicKey( publicKeyPath );
		if( publicKey.isNull() || publicKey.isPublic() == false )
		{
			vWarning() << "failed to load public key from" << publicKeyPath;
			return VncServerClient::AuthState::Failed;
		}

		vDebug() << "loaded public key from" << publicKeyPath;
		if( publicKey.verifyMessage( client->challenge(), signature, CryptoCore::DefaultSignatureAlgorithm ) == false )
		{
			vWarning() << "FAIL";
			return VncServerClient::AuthState::Failed;
		}

		vDebug() << "SUCCESS";
		return VncServerClient::AuthState::Successful;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}



VncServerClient::AuthState ServerAuthenticationManager::performLogonAuthentication( VncServerClient* client,
																					VariantArrayMessage& message )
{
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		client->setPrivateKey( CryptoCore::KeyGenerator().createRSA( CryptoCore::RsaKeySize ) );

		if( VariantArrayMessage( message.ioDevice() ).write( client->privateKey().toPublicKey().toPEM() ).send() )
		{
			return VncServerClient::AuthState::Password;
		}

		vDebug() << "failed to send public key";
		return VncServerClient::AuthState::Failed;

	case VncServerClient::AuthState::Password:
	{
		auto privateKey = client->privateKey();

		CryptoCore::SecureArray encryptedPassword( message.read().toByteArray() ); // Flawfinder: ignore

		CryptoCore::SecureArray decryptedPassword;

		if( privateKey.decrypt( encryptedPassword,
								&decryptedPassword,
								CryptoCore::DefaultEncryptionAlgorithm ) == false )
		{
			vWarning() << "failed to decrypt password";
			return VncServerClient::AuthState::Failed;
		}

		vInfo() << "authenticating user" << client->username();

		if( VeyonCore::platform().userFunctions().authenticate( client->username(), decryptedPassword ) )
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthState::Successful;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthState::Failed;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}



VncServerClient::AuthState ServerAuthenticationManager::performTokenAuthentication( VncServerClient* client,
																					VariantArrayMessage& message )
{
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		return VncServerClient::AuthState::Token;

	case VncServerClient::AuthState::Token:
	{
		const auto token = AuthenticationCredentials::Token( message.read().toByteArray() );  // Flawfinder: ignore

		if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::Type::Token ) &&
				token == VeyonCore::authenticationCredentials().token() )
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthState::Successful;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthState::Failed;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}
