/*
 * ServerAuthenticationManager.cpp - implementation of ServerAuthenticationManager
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <QtCrypto>
#include <QHostAddress>

#include "ServerAuthenticationManager.h"
#include "AuthenticationCredentials.h"
#include "DsaKey.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "LogonAuthentication.h"
#include "VariantArrayMessage.h"


ServerAuthenticationManager::ServerAuthenticationManager( QObject* parent ) :
	QObject( parent ),
	m_supportedAuthTypes( { RfbItalcAuth::HostWhiteList } ),
	m_allowedIPs(),
	m_failedAuthHosts()
{
	if( ItalcCore::config->isKeyAuthenticationEnabled() )
	{
		m_supportedAuthTypes.append( RfbItalcAuth::DSA );
	}

	if( ItalcCore::config->isLogonAuthenticationEnabled() )
	{
		m_supportedAuthTypes.append( RfbItalcAuth::Logon );
	}

	if( ItalcCore::authenticationCredentials->hasCredentials( AuthenticationCredentials::Token ) )
	{
		m_supportedAuthTypes.append( RfbItalcAuth::Token );
	}

}



void ServerAuthenticationManager::processAuthenticationMessage( Client* client,
																VariantArrayMessage& message )
{
	qDebug() << "ServerAuthenticationManager::authenticateClient():"
			 << "state" << client->state()
			 << "type" << client->authType()
			 << "host" << client->hostAddress()
			 << "user" << client->username();

	switch( client->authType() )
	{
	// no authentication
	case RfbItalcAuth::None:
		client->setState( Client::FinishedSuccess );
		break;

		// host has to be in list of allowed hosts
	case RfbItalcAuth::HostWhiteList:
		client->setState( performHostWhitelistAuth( client, message ) );
		break;

		// authentication via DSA-challenge/-response
	case RfbItalcAuth::DSA:
		client->setState( performKeyAuthentication( client, message ) );
		break;

	case RfbItalcAuth::Logon:
		client->setState( performLogonAuthentication( client, message ) );
		break;

	case RfbItalcAuth::Token:
		client->setState( performTokenAuthentication( client, message ) );
		break;

	default:
		break;
	}

	if( client->state() == Client::FinishedFail )
	{
		emit authenticationError( client->hostAddress(), client->username() );
	}
}



void ServerAuthenticationManager::setAllowedIPs(const QStringList &allowedIPs)
{
	QMutexLocker l( &m_dataMutex );
	m_allowedIPs = allowedIPs;
}



ServerAuthenticationManager::Client::State ServerAuthenticationManager::performKeyAuthentication( Client* client,
																								  VariantArrayMessage& message )
{
	switch( client->state() )
	{
	case Client::Init:
		client->setChallenge( DsaKey::generateChallenge() );
		if( VariantArrayMessage( message.ioDevice() ).write( client->challenge() ).send() )
		{
			return Client::Challenge;
		}
		else
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): failed to send challenge" );
			return Client::FinishedFail;
		}
		break;

	case Client::Challenge:
	{
		// get user role
		const ItalcCore::UserRoles urole = static_cast<ItalcCore::UserRoles>( message.read().toInt() );

		// now try to verify received signed data using public key of the user
		// under which the client claims to run
		const QByteArray sig = message.read().toByteArray();

		qDebug() << "Loading public key" << LocalSystem::Path::publicKeyPath( urole ) << "for role" << urole;
		// (publicKeyPath does range-checking of urole)
		PublicDSAKey pubKey( LocalSystem::Path::publicKeyPath( urole ) );

		if( pubKey.verifySignature( client->challenge(), sig ) )
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): SUCCESS" );
			return Client::FinishedSuccess;
		}
		else
		{
			qDebug( "ServerAuthenticationManager::performKeyAuthentication(): FAIL" );
			return Client::FinishedFail;
		}
		break;
	}

	default:
		break;
	}

	return Client::FinishedFail;
}



ServerAuthenticationManager::Client::State ServerAuthenticationManager::performLogonAuthentication( Client* client,
																									VariantArrayMessage& message )
{
	switch( client->state() )
	{
	case Client::Init:
	{
		QCA::PrivateKey privateKey = QCA::KeyGenerator().createRSA( 1024 );

		client->setPrivateKey( privateKey.toPEM() );

		QCA::PublicKey publicKey = privateKey.toPublicKey();

		if( VariantArrayMessage( message.ioDevice() ).write( publicKey.toPEM() ).send() )
		{
			return Client::Password;
		}
		else
		{
			qDebug( "ServerAuthenticationManager::performLogonAuthentication(): failed to send public key" );
			return Client::FinishedFail;
		}
	}

	case Client::Password:
	{
		QCA::PrivateKey privateKey = QCA::PrivateKey::fromPEM( client->privateKey() );

		QCA::SecureArray encryptedPassword( message.read().toByteArray() );

		QCA::SecureArray decryptedPassword;

		if( privateKey.decrypt( encryptedPassword, &decryptedPassword, QCA::EME_PKCS1_OAEP ) == false )
		{
			qWarning( "ServerAuthenticationManager::performLogonAuthentication(): failed to decrypt password" );
			return Client::FinishedFail;
		}

		AuthenticationCredentials credentials;
		credentials.setLogonUsername( client->username() );
		credentials.setLogonPassword( QString::fromUtf8( decryptedPassword.toByteArray() ) );

		if( LogonAuthentication::authenticateUser( credentials ) )
		{
			qDebug( "ServerAuthenticationManager::performLogonAuthentication(): SUCCESS" );
			return Client::FinishedSuccess;
		}

		qDebug( "ServerAuthenticationManager::performLogonAuthentication(): FAIL" );
		return Client::FinishedFail;
	}

	default:
		break;
	}

	return Client::FinishedFail;
}


ServerAuthenticationManager::Client::State ServerAuthenticationManager::performHostWhitelistAuth( Client* client,
																								  VariantArrayMessage& message )
{
	QMutexLocker l( &m_dataMutex );

	if( m_allowedIPs.isEmpty() )
	{
		qWarning( "ServerAuthenticationManager: empty list of allowed IPs" );
		return Client::FinishedFail;
	}

	if( m_allowedIPs.contains( client->hostAddress() ) )
	{
		qDebug( "ServerAuthenticationManager::performHostWhitelistAuth(): SUCCESS" );
		return Client::FinishedSuccess;
	}

	qWarning( "ServerAuthenticationManager::performHostWhitelistAuth(): FAIL" );

	// authentication failed
	return Client::FinishedFail;
}



ServerAuthenticationManager::Client::State ServerAuthenticationManager::performTokenAuthentication( Client* client,
																									VariantArrayMessage& message )
{
	Q_UNUSED(client);

	if( ItalcCore::authenticationCredentials->hasCredentials( AuthenticationCredentials::Token ) &&
			message.read().toString() == ItalcCore::authenticationCredentials->token() )
	{
		qDebug( "ServerAuthenticationManager::performTokenAuthentication(): SUCCESS" );

		return Client::FinishedSuccess;
	}

	qDebug( "ServerAuthenticationManager::performTokenAuthentication(): FAIL" );

	return Client::FinishedFail;
}
