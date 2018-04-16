/*
 * VeyonCoreConnection.cpp - implementation of VeyonCoreConnection
 *
 * Copyright (c) 2008-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "FeatureMessage.h"
#include "VeyonCoreConnection.h"
#include "Logger.h"
#include "SocketDevice.h"

extern "C"
{
	#include <rfb/rfbclient.h>
}

// clazy:excludeall=copyable-polymorphic

class FeatureMessageEvent : public MessageEvent
{
public:
	FeatureMessageEvent( const FeatureMessage& featureMessage ) :
		m_featureMessage( featureMessage )
	{
	}

	void fire( rfbClient* client ) override
	{
		qDebug() << "FeatureMessageEvent::fire(): sending message" << m_featureMessage.featureUid()
				 << "command" << m_featureMessage.command()
				 << "arguments" << m_featureMessage.arguments();

		SocketDevice socketDevice( VeyonVncConnection::libvncClientDispatcher, client );
		char messageType = rfbVeyonFeatureMessage;
		socketDevice.write( &messageType, sizeof(messageType) );

		m_featureMessage.send( &socketDevice );
	}


private:
	FeatureMessage m_featureMessage;

} ;



static rfbClientProtocolExtension * __veyonProtocolExt = nullptr;
static void* VeyonCoreConnectionTag = reinterpret_cast<void *>( PortOffsetVncServer ); // an unique ID



VeyonCoreConnection::VeyonCoreConnection( VeyonVncConnection *vncConn ):
	m_vncConn( vncConn ),
	m_user(),
	m_userHomeDir()
{
	if( __veyonProtocolExt == nullptr )
	{
		__veyonProtocolExt = new rfbClientProtocolExtension;
		__veyonProtocolExt->encodings = nullptr;
		__veyonProtocolExt->handleEncoding = nullptr;
		__veyonProtocolExt->handleMessage = handleVeyonMessage;

		rfbClientRegisterExtension( __veyonProtocolExt );
	}

	if (m_vncConn) {
		connect( m_vncConn, &VeyonVncConnection::newClient,
				this, &VeyonCoreConnection::initNewClient,
				Qt::DirectConnection );
	}
}




VeyonCoreConnection::~VeyonCoreConnection()
{
}



void VeyonCoreConnection::sendFeatureMessage( const FeatureMessage& featureMessage )
{
	if( !m_vncConn )
	{
		ilog( Error, "VeyonCoreConnection::sendFeatureMessage(): cannot call enqueueEvent - m_vncConn is NULL" );
		return;
	}

	m_vncConn->enqueueEvent( new FeatureMessageEvent( featureMessage ) );
}



void VeyonCoreConnection::initNewClient( rfbClient *cl )
{
	rfbClientSetClientData( cl, VeyonCoreConnectionTag, this );
}




rfbBool VeyonCoreConnection::handleVeyonMessage( rfbClient* client, rfbServerToClientMsg* msg )
{
	auto coreConnection = reinterpret_cast<VeyonCoreConnection *>( rfbClientGetClientData( client, VeyonCoreConnectionTag ) );
	if( coreConnection )
	{
		return coreConnection->handleServerMessage( client, msg->type );
	}

	return false;
}




bool VeyonCoreConnection::handleServerMessage( rfbClient* client, uint8_t msg )
{
	if( msg == rfbVeyonFeatureMessage )
	{
		SocketDevice socketDev( VeyonVncConnection::libvncClientDispatcher, client );
		FeatureMessage featureMessage( &socketDev );
		if( featureMessage.receive() == false )
		{
			qDebug( "VeyonCoreConnection: could not receive feature message" );

			return false;
		}

		qDebug() << "VeyonCoreConnection: received feature message"
				 << featureMessage.command()
				 << "with arguments" << featureMessage.arguments();

		emit featureMessageReceived(  featureMessage );

		return true;
	}
	else
	{
		qCritical( "VeyonCoreConnection::handleServerMessage(): "
				"unknown message type %d from server. Closing "
				"connection. Will re-open it later.", static_cast<int>( msg ) );
	}

	return false;
}
