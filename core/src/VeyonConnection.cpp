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

#include "AuthenticationManager.h"
#include "PlatformUserFunctions.h"
#include "SocketDevice.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncFeatureMessageEvent.h"


static rfbClientProtocolExtension* __veyonProtocolExt = nullptr;
static constexpr std::array<uint32_t, 2> __veyonSecurityTypes = { VeyonCore::RfbSecurityTypeVeyon, 0 };


rfbBool handleVeyonMessage( rfbClient* client, rfbServerToClientMsg* msg )
{
	auto connection = reinterpret_cast<VeyonConnection *>( VncConnection::clientData( client, VeyonConnection::VeyonConnectionTag ) );
	if( connection )
	{
		return connection->handleServerMessage( client, msg->type ) ? TRUE : FALSE;
	}

	return false;
}



VeyonConnection::VeyonConnection():
	m_user(),
	m_userHomeDir()
{
	if( __veyonProtocolExt == nullptr )
	{
		__veyonProtocolExt = new rfbClientProtocolExtension;
		__veyonProtocolExt->encodings = nullptr;
		__veyonProtocolExt->handleEncoding = nullptr;
		__veyonProtocolExt->handleMessage = handleVeyonMessage;
		__veyonProtocolExt->securityTypes = __veyonSecurityTypes.data();
		__veyonProtocolExt->handleAuthentication = handleSecTypeVeyon;

		rfbClientRegisterExtension( __veyonProtocolExt );
	}

	connect( m_vncConnection, &VncConnection::connectionPrepared, this, &VeyonConnection::registerConnection, Qt::DirectConnection );
	connect( m_vncConnection, &VncConnection::destroyed, VeyonCore::instance(), [this]() {
		delete this;
	} );
}



void VeyonConnection::stopAndDeleteLater()
{
	unregisterConnection();

	if( m_vncConnection )
	{
		m_vncConnection->stopAndDeleteLater();
		m_vncConnection = nullptr;
	}
}



void VeyonConnection::sendFeatureMessage(const FeatureMessage& featureMessage)
{
	if( m_vncConnection )
	{
		m_vncConnection->enqueueEvent(new VncFeatureMessageEvent(featureMessage));
	}
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

		vDebug() << featureMessage;

		Q_EMIT featureMessageReceived( featureMessage );

		return true;
	}

	vCritical() << "unknown message type" << int( msg )
				<< "from server. Closing connection. Will re-open it later.";

	return false;
}



void VeyonConnection::registerConnection()
{
	if( m_vncConnection )
	{
		m_vncConnection->setClientData( VeyonConnectionTag, this );
	}
}



void VeyonConnection::unregisterConnection()
{
	if( m_vncConnection )
	{
		m_vncConnection->setClientData( VeyonConnectionTag, nullptr );
	}
}



int8_t VeyonConnection::handleSecTypeVeyon( rfbClient* client, uint32_t authScheme )
{
	if( authScheme != VeyonCore::RfbSecurityTypeVeyon )
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
	if( message.receive() == false )
	{
		vDebug() << QThread::currentThreadId() << "invalid authentication message received";
		return false;
	}

	const auto authTypeCount = message.read().toInt();

	if( authTypeCount == 0 )
	{
		vDebug() << QThread::currentThreadId() << "no auth types received";
		return false;
	}

	auto legacyAuth = false;
	PluginUidList authTypes;
	authTypes.reserve( authTypeCount );

	for( int i = 0; i < authTypeCount; ++i )
	{
		if( message.atEnd() )
		{
			vDebug() << QThread::currentThreadId() << "less auth types received than announced";
			return false;
		}
		const auto authType = message.read();
		if( authType.userType() == QMetaType::QUuid )
		{
			authTypes.append( authType.toUuid() );
		}
		const auto legacyAuthType = VeyonCore::authenticationManager().fromLegacyAuthType(
			authType.value<AuthenticationManager::LegacyAuthType>() );
		if( legacyAuthType.isNull() == false )
		{
			authTypes.append( legacyAuthType );
			legacyAuth = true;
		}
	}

	vDebug() << QThread::currentThreadId() << "received authentication types:" << authTypes;

	auto chosenAuthPlugin = Plugin::Uid();

	const auto& plugins = VeyonCore::authenticationManager().plugins();
	for( auto it = plugins.constBegin(), end = plugins.constEnd(); it != end; ++it )
	{
		if( authTypes.contains( it.key() ) && it.value()->hasCredentials() )
		{
			chosenAuthPlugin = it.key();
		}
	}

	if( chosenAuthPlugin.isNull() )
	{
		vWarning() << QThread::currentThreadId() << "authentication plugins not supported "
					  "or missing credentials";
		return false;
	}

	VariantArrayMessage authReplyMessage( &socketDevice );

	if( legacyAuth )
	{
		const auto legacyAuthType = VeyonCore::authenticationManager().toLegacyAuthType( chosenAuthPlugin );
		vDebug() << QThread::currentThreadId() << "chose legacy authentication type:" << legacyAuthType;
		authReplyMessage.write( legacyAuthType );
	}
	else
	{
		vDebug() << QThread::currentThreadId() << "chose authentication method:" << chosenAuthPlugin;
		authReplyMessage.write( chosenAuthPlugin );
	}
	// send username which is used when displaying an access confirm dialog
	authReplyMessage.write( VeyonCore::platform().userFunctions().currentUser() );
	authReplyMessage.send();

	VariantArrayMessage authAckMessage( &socketDevice );
	authAckMessage.receive();

	return plugins[chosenAuthPlugin]->authenticate( &socketDevice ) ? TRUE : FALSE;
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
