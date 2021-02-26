/*
 * VeyonConnection.cpp - implementation of VeyonConnection
 *
 * Copyright (c) 2008-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "rfb/rfbclient.h"

#include "AuthenticationProxy.h"
#include "CryptoCore.h"
#include "PlatformUserFunctions.h"
#include "SocketDevice.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncFeatureMessageEvent.h"


static rfbClientProtocolExtension* __veyonProtocolExt = nullptr;
static const uint32_t __veyonSecurityTypes[2] = { rfbSecTypeVeyon, 0 };


rfbBool handleVeyonMessage( rfbClient* client, rfbServerToClientMsg* msg )
{
	auto connection = reinterpret_cast<VeyonConnection *>( VncConnection::clientData( client, VeyonConnection::VeyonConnectionTag ) );
	if( connection )
	{
		return connection->handleServerMessage( client, msg->type );
	}

	return false;
}



VeyonConnection::VeyonConnection( VncConnection* vncConnection ):
	m_vncConnection( vncConnection ),
	m_veyonAuthType( RfbVeyonAuth::Logon ),
	m_user(),
	m_userHomeDir()
{
	if( __veyonProtocolExt == nullptr )
	{
		__veyonProtocolExt = new rfbClientProtocolExtension;
		__veyonProtocolExt->encodings = nullptr;
		__veyonProtocolExt->handleEncoding = nullptr;
		__veyonProtocolExt->handleMessage = handleVeyonMessage;
		__veyonProtocolExt->securityTypes = __veyonSecurityTypes;
		__veyonProtocolExt->handleAuthentication = handleSecTypeVeyon;

		rfbClientRegisterExtension( __veyonProtocolExt );
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication )
	{
		m_veyonAuthType = RfbVeyonAuth::KeyFile;
	}

	connect( m_vncConnection, &VncConnection::connectionPrepared, this, &VeyonConnection::registerConnection, Qt::DirectConnection );
	connect( m_vncConnection, &VncConnection::destroyed, this, &VeyonConnection::deleteLater );
}



VeyonConnection::~VeyonConnection()
{
	unregisterConnection();

	delete m_authenticationProxy;
}



void VeyonConnection::sendFeatureMessage( const FeatureMessage& featureMessage, bool wake )
{
	if( m_vncConnection.isNull() )
	{
		vCritical() << "cannot enqueue event as VNC connection is invalid";
		return;
	}

	m_vncConnection->enqueueEvent( new VncFeatureMessageEvent( featureMessage ), wake );
}



bool VeyonConnection::handleServerMessage( rfbClient* client, uint8_t msg )
{
	if( msg == FeatureMessage::RfbMessageType )
	{
		SocketDevice socketDev( VncConnection::libvncClientDispatcher, client );
		FeatureMessage featureMessage;
		if( featureMessage.receive( &socketDev ) == false )
		{
			vDebug() << "could not receive feature message";

			return false;
		}

		vDebug() << "received feature message" << featureMessage.command()
			   << "with arguments" << featureMessage.arguments();

		Q_EMIT featureMessageReceived( featureMessage );

		return true;
	}
	else
	{
		vCritical() << "unknown message type" << static_cast<int>( msg )
					<< "from server. Closing connection. Will re-open it later.";
	}

	return false;
}



void VeyonConnection::registerConnection()
{
	if( m_vncConnection.isNull() == false )
	{
		m_vncConnection->setClientData( VeyonConnectionTag, this );
	}
}



void VeyonConnection::unregisterConnection()
{
	if( m_vncConnection.isNull() == false )
	{
		m_vncConnection->setClientData( VeyonConnectionTag, nullptr );
	}
}



int8_t VeyonConnection::handleSecTypeVeyon( rfbClient* client, uint32_t authScheme )
{
	if( authScheme != rfbSecTypeVeyon )
	{
		return false;
	}

	hookPrepareAuthentication( client );

	auto connection = static_cast<VeyonConnection *>( VncConnection::clientData( client, VeyonConnectionTag ) );
	if( connection == nullptr )
	{
		return false;
	}

	SocketDevice socketDevice( VncConnection::libvncClientDispatcher, client );
	VariantArrayMessage message( &socketDevice );
	message.receive();

	int authTypeCount = message.read().toInt();

	QList<RfbVeyonAuth::Type> authTypes;
	authTypes.reserve( authTypeCount );

	for( int i = 0; i < authTypeCount; ++i )
	{
		authTypes.append( QVariantHelper<RfbVeyonAuth::Type>::value( message.read() ) );
	}

	auto proxy = connection->m_authenticationProxy;

	if( proxy )
	{
		proxy->setAuthenticationTypes( authTypes );
	}

	vDebug() << QThread::currentThreadId() << "received authentication types:" << authTypes;

	RfbVeyonAuth::Type chosenAuthType = RfbVeyonAuth::Token;
	if( authTypes.count() > 0 )
	{
		chosenAuthType = authTypes.first();

		// look whether the VncConnection recommends a specific
		// authentication type (e.g. VeyonAuthHostBased when running as
		// demo client)

		for( auto authType : authTypes )
		{
			if( connection->veyonAuthType() == authType )
			{
				chosenAuthType = authType;
			}
		}
	}

	if( proxy )
	{
		chosenAuthType = proxy->initCredentials();
	}

	if( chosenAuthType == RfbVeyonAuth::None )
	{
		return false;
	}

	vDebug() << QThread::currentThreadId() << "chose authentication type:" << authTypes;

	VariantArrayMessage authReplyMessage( &socketDevice );

	authReplyMessage.write( chosenAuthType );

	// send username which is used when displaying an access confirm dialog
	if( connection->authenticationCredentials().hasCredentials( AuthenticationCredentials::Type::UserLogon ) )
	{
		authReplyMessage.write( connection->authenticationCredentials().logonUsername() );
	}
	else
	{
		authReplyMessage.write( VeyonCore::platform().userFunctions().currentUser() );
	}

	authReplyMessage.send();

	VariantArrayMessage authAckMessage( &socketDevice );
	authAckMessage.receive();

	switch( chosenAuthType )
	{
	case RfbVeyonAuth::KeyFile:
		if( connection->authenticationCredentials().hasCredentials( AuthenticationCredentials::Type::PrivateKey ) )
		{
			VariantArrayMessage challengeReceiveMessage( &socketDevice );
			challengeReceiveMessage.receive();
			const auto challenge = challengeReceiveMessage.read().toByteArray();

			if( challenge.size() != CryptoCore::ChallengeSize )
			{
				vCritical() << QThread::currentThreadId() << "challenge size mismatch!";
				return false;
			}

			// create local copy of private key so we can modify it within our own thread
			auto key = connection->authenticationCredentials().privateKey();
			if( key.isNull() || key.canSign() == false )
			{
				vCritical() << QThread::currentThreadId() << "invalid private key!";
				return false;
			}

			const auto signature = key.signMessage( challenge, CryptoCore::DefaultSignatureAlgorithm );

			VariantArrayMessage challengeResponseMessage( &socketDevice );
			challengeResponseMessage.write( connection->authenticationCredentials().authenticationKeyName() );
			challengeResponseMessage.write( signature );
			challengeResponseMessage.send();
		}
		break;

	case RfbVeyonAuth::Logon:
	{
		VariantArrayMessage publicKeyMessage( &socketDevice );
		publicKeyMessage.receive();

		CryptoCore::PublicKey publicKey = CryptoCore::PublicKey::fromPEM( publicKeyMessage.read().toString() );

		if( publicKey.canEncrypt() == false )
		{
			vCritical() << QThread::currentThreadId() << "can't encrypt with given public key!";
			return false;
		}

		CryptoCore::SecureArray plainTextPassword( connection->authenticationCredentials().logonPassword() );
		CryptoCore::SecureArray encryptedPassword = publicKey.encrypt( plainTextPassword, CryptoCore::DefaultEncryptionAlgorithm );
		if( encryptedPassword.isEmpty() )
		{
			vCritical() << QThread::currentThreadId() << "password encryption failed!";
			return false;
		}

		VariantArrayMessage passwordResponse( &socketDevice );
		passwordResponse.write( encryptedPassword.toByteArray() );
		passwordResponse.send();
		break;
	}

	case RfbVeyonAuth::Token:
	{
		VariantArrayMessage tokenAuthMessage( &socketDevice );
		tokenAuthMessage.write( connection->authenticationCredentials().token().toByteArray() );
		tokenAuthMessage.send();
		break;
	}

	default:
		// nothing to do - we just get accepted
		break;
	}

	return true;
}



void VeyonConnection::hookPrepareAuthentication( rfbClient* client )
{
	auto connection = static_cast<VncConnection *>( VncConnection::clientData( client, VncConnection::VncConnectionTag ) );
	if( connection )
	{
		// set our internal flag which indicates that we basically have communication with the client
		// which means that the host is reachable
		connection->setServerReachable();
	}
}



AuthenticationCredentials VeyonConnection::authenticationCredentials() const
{
	if( m_authenticationProxy )
	{
		return m_authenticationProxy->credentials();
	}

	return VeyonCore::authenticationCredentials();
}
