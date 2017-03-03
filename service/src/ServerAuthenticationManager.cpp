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

#include <QHostAddress>
#include <QTcpSocket>

#include "ServerAuthenticationManager.h"
#include "AccessControlProvider.h"
#include "AuthenticationCredentials.h"
#include "DesktopAccessPermission.h"
#include "DsaKey.h"
#include "LocalSystem.h"
#include "LogonAuthentication.h"
#include "VariantStream.h"

#include "rfb/dh.h"

extern "C"
{
#include "common/d3des.h"
}


ServerAuthenticationManager::ServerAuthenticationManager( FeatureWorkerManager& featureWorkerManager,
														  DesktopAccessDialog& desktopAccessDialog ) :
	QObject(),
	m_featureWorkerManager( featureWorkerManager ),
	m_desktopAccessDialog( desktopAccessDialog ),
	m_allowedIPs(),
	m_failedAuthHosts()
{
}


bool ServerAuthenticationManager::authenticateClient( QTcpSocket* socket, RfbItalcAuth::Type authType )
{
	QString username = VariantStream( socket ).read().toString();

	const QString hostAddress = socket->peerAddress().toString();

	qDebug() << "ServerAuthenticationManager::authenticateClient():"
			 << "type" << authType
			 << "host" << hostAddress
			 << "user" << username;

	switch( authType )
	{
	// no authentication
	case RfbItalcAuth::None:
		return true;
		break;

		// host has to be in list of allowed hosts
	case RfbItalcAuth::HostWhiteList:
		if( performHostWhitelistAuth( socket ) )
		{
			return true;
		}
		else
		{
			return false;
		}
		break;

		// authentication via DSA-challenge/-response
	case RfbItalcAuth::DSA:
		if( performKeyAuthentication( socket ) &&
				performAccessControl( username, hostAddress, DesktopAccessPermission::KeyAuthentication ) )
		{
			return true;
		}
		break;

	case RfbItalcAuth::Logon:
		if( performLogonAuthentication( socket ) &&
				performAccessControl( username, hostAddress, DesktopAccessPermission::LogonAuthentication ) )
		{
			return true;
		}
		break;

	case RfbItalcAuth::Token:
		if( performTokenAuthentication( socket ) )
		{
			return true;
		}
		break;

	default:
		break;
	}

	qWarning() << "ServerAuthenticationManager::authenticateClient(): failed authenticating client" << hostAddress << username;

	emit authenticationError( hostAddress, username );

	return false;
}



void ServerAuthenticationManager::setAllowedIPs(const QStringList &allowedIPs)
{
	QMutexLocker l( &m_dataMutex );
	m_allowedIPs = allowedIPs;
}



bool ServerAuthenticationManager::performAccessControl( const QString &username, const QString &host,
														DesktopAccessPermission::AuthenticationMethod authenticationMethod )
{
	auto accessResult = AccessControlProvider().checkAccess( username, host );

	DesktopAccessPermission desktopAccessPermission( authenticationMethod );

	switch( accessResult )
	{
	case AccessControlProvider::AccessToBeConfirmed:
		return desktopAccessPermission.ask( m_featureWorkerManager, m_desktopAccessDialog, username, host );

	case AccessControlProvider::AccessAllow:
		if( desktopAccessPermission.authenticationMethodRequiresConfirmation() )
		{
			return desktopAccessPermission.ask( m_featureWorkerManager, m_desktopAccessDialog, username, host );
		}
		return true;

	default:
		break;
	}

	return false;
}



bool ServerAuthenticationManager::performKeyAuthentication( QTcpSocket* socket )
{
	VariantStream stream( socket );

	// generate data to sign and send to client
	const QByteArray chall = DsaKey::generateChallenge();
	stream.write( chall );

	// get user role
	const ItalcCore::UserRoles urole = static_cast<ItalcCore::UserRoles>( stream.read().toInt() );

	// now try to verify received signed data using public key of the user
	// under which the client claims to run
	const QByteArray sig = stream.read().toByteArray();

	qDebug() << "Loading public key" << LocalSystem::Path::publicKeyPath( urole )
			 << "for role" << urole;
	// (publicKeyPath does range-checking of urole)
	PublicDSAKey pubKey( LocalSystem::Path::publicKeyPath( urole ) );

	if( pubKey.verifySignature( chall, sig ) )
	{
		qDebug( "ServerAuthenticationManager::performKeyAuthentication(): SUCCESS" );

		return true;
	}

	qDebug( "ServerAuthenticationManager::performKeyAuthentication(): FAIL" );
	return false;
}


#ifndef ITALC_BUILD_WIN32

static void vncDecryptBytes(unsigned char *where, const int length, const unsigned char *key)
{
	int i, j;
	rfbDesKey((unsigned char*) key, DE1);
	for (i = length - 8; i > 0; i -= 8) {
		rfbDes(where + i, where + i);
		for (j = 0; j < 8; j++)
			where[i + j] ^= where[i + j - 8];
	}
	/* i = 0 */
	rfbDes(where, where);
	for (i = 0; i < 8; i++)
		where[i] ^= key[i];
}

#endif


bool ServerAuthenticationManager::performLogonAuthentication( QTcpSocket* socket )
{
#ifndef ITALC_BUILD_WIN32

	DiffieHellman dh;
	char gen[8], mod[8], pub[8], resp[8];
	char user[256], passwd[64];
	unsigned char key[8];

	dh.createKeys();
	int64ToBytes( dh.getValue(DH_GEN), gen );
	int64ToBytes( dh.getValue(DH_MOD), mod );
	int64ToBytes( dh.createInterKey(), pub );
	if( socket->write( gen, sizeof(gen) ) != sizeof(gen) ) return false;
	if( socket->write( mod, sizeof(mod) ) != sizeof(mod) ) return false;
	if( socket->write( pub, sizeof(pub) ) != sizeof(pub) ) return false;
	socket->flush();

	if( socket->read( resp, sizeof(resp) ) != sizeof(resp) ) return false;
	if( socket->read( user, sizeof(user) ) != sizeof(resp) ) return false;
	if( socket->read( passwd, sizeof(passwd) ) != sizeof(resp) ) return false;

	int64ToBytes( dh.createEncryptionKey( bytesToInt64( resp ) ), (char*) key );
	vncDecryptBytes( (unsigned char*) user, sizeof(user), key ); user[255] = '\0';
	vncDecryptBytes( (unsigned char*) passwd, sizeof(passwd), key ); passwd[63] = '\0';

	AuthenticationCredentials credentials;
	credentials.setLogonUsername( user );
	credentials.setLogonPassword( passwd );

	return LogonAuthentication::authenticateUser( credentials );
#else
	// TODO
	return false;
#endif
}



bool ServerAuthenticationManager::performHostWhitelistAuth( QTcpSocket* socket )
{
	QMutexLocker l( &m_dataMutex );

	if( m_allowedIPs.isEmpty() )
	{
		qWarning( "ServerAuthenticationManager: empty list of allowed IPs" );
		return false;
	}

	const QString host = socket->peerAddress().toString();

	if( m_allowedIPs.contains( host ) )
	{
		qDebug( "ServerAuthenticationManager::performHostWhitelistAuth(): SUCCESS" );
		return true;
	}

	qWarning( "ServerAuthenticationManager::performHostWhitelistAuth(): FAIL" );

	// authentication failed
	return false;
}



bool ServerAuthenticationManager::performTokenAuthentication( QTcpSocket* socket )
{

	if( ItalcCore::authenticationCredentials->hasCredentials( AuthenticationCredentials::Token ) &&
			VariantStream( socket ).read().toString() == ItalcCore::authenticationCredentials->token() )
	{
		qDebug( "ServerAuthenticationManager::performTokenAuthentication(): SUCCESS" );

		return true;
	}

	qDebug( "ServerAuthenticationManager::performTokenAuthentication(): FAIL" );

	return false;
}
