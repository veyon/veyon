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
		return connection->handleServerMessage( client, msg->type );
	}

	return false;
}



VeyonConnection::VeyonConnection( VncConnection* vncConnection ):
	m_vncConnection( vncConnection ),
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
	connect( m_vncConnection, &VncConnection::destroyed, this, &VeyonConnection::deleteLater );
}



VeyonConnection::~VeyonConnection()
{
	unregisterConnection();
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

	vCritical() << "unknown message type" << int( msg )
				<< "from server. Closing connection. Will re-open it later.";

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

	PluginUidList authTypes;
	authTypes.reserve( authTypeCount );

	for( int i = 0; i < authTypeCount; ++i )
	{
		authTypes.append( message.read().toUuid() );
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

	vDebug() << QThread::currentThreadId() << "chose authentication type:" << chosenAuthPlugin;

	VariantArrayMessage authReplyMessage( &socketDevice );

	authReplyMessage.write( chosenAuthPlugin );

	// send username which is used when displaying an access confirm dialog
	authReplyMessage.write( VeyonCore::platform().userFunctions().currentUser() );
	authReplyMessage.send();

	VariantArrayMessage authAckMessage( &socketDevice );
	authAckMessage.receive();

	return plugins[chosenAuthPlugin]->authenticate( &socketDevice );
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
