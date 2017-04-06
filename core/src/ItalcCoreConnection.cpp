/*
 * ItalcCoreConnection.cpp - implementation of ItalcCoreConnection
 *
 * Copyright (c) 2008-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "FeatureMessage.h"
#include "ItalcCoreConnection.h"
#include "Logger.h"
#include "SocketDevice.h"

extern "C"
{
	#include <rfb/rfbclient.h>
}


class FeatureMessageEvent : public MessageEvent
{
public:
	FeatureMessageEvent( const FeatureMessage& featureMessage ) :
		m_featureMessage( featureMessage )
	{
	}

	void fire( rfbClient *client ) override
	{
		qDebug() << "FeatureMessageEvent::fire(): sending message" << m_featureMessage.featureUid()
				 << "with arguments" << m_featureMessage.arguments();

		SocketDevice socketDevice( ItalcVncConnection::libvncClientDispatcher, client );
		char messageType = rfbItalcFeatureMessage;
		socketDevice.write( &messageType, sizeof(messageType) );

		m_featureMessage.send( &socketDevice );
	}


private:
	FeatureMessage m_featureMessage;

} ;



static rfbClientProtocolExtension * __italcProtocolExt = nullptr;
static void * ItalcCoreConnectionTag = (void *) PortOffsetVncServer; // an unique ID



ItalcCoreConnection::ItalcCoreConnection( ItalcVncConnection *vncConn ):
	m_vncConn( vncConn ),
	m_user(),
	m_userHomeDir()
{
	if( __italcProtocolExt == nullptr )
	{
		__italcProtocolExt = new rfbClientProtocolExtension;
		__italcProtocolExt->encodings = nullptr;
		__italcProtocolExt->handleEncoding = nullptr;
		__italcProtocolExt->handleMessage = handleItalcMessage;

		rfbClientRegisterExtension( __italcProtocolExt );
	}

	if (m_vncConn) {
		connect( m_vncConn, SIGNAL( newClient( rfbClient * ) ),
				this, SLOT( initNewClient( rfbClient * ) ),
				Qt::DirectConnection );
	}
}




ItalcCoreConnection::~ItalcCoreConnection()
{
}



void ItalcCoreConnection::sendFeatureMessage( const FeatureMessage& featureMessage )
{
	if( !m_vncConn )
	{
		ilog( Error, "ItalcCoreConnection::sendFeatureMessage(): cannot call enqueueEvent - m_vncConn is NULL" );
		return;
	}

	m_vncConn->enqueueEvent( new FeatureMessageEvent( featureMessage ) );
}



void ItalcCoreConnection::initNewClient( rfbClient *cl )
{
	rfbClientSetClientData( cl, ItalcCoreConnectionTag, this );
}




rfbBool ItalcCoreConnection::handleItalcMessage( rfbClient* client, rfbServerToClientMsg* msg )
{
	auto coreConnection = (ItalcCoreConnection *) rfbClientGetClientData( client, ItalcCoreConnectionTag );
	if( coreConnection )
	{
		return coreConnection->handleServerMessage( client, msg->type );
	}

	return false;
}




bool ItalcCoreConnection::handleServerMessage( rfbClient *client, uint8_t msg )
{
	if( msg == rfbItalcFeatureMessage )
	{
		SocketDevice socketDev( ItalcVncConnection::libvncClientDispatcher, client );
		FeatureMessage featureMessage( &socketDev );
		featureMessage.receive();

		qDebug() << "ItalcCoreConnection: received feature message"
				 << featureMessage.command()
				 << "with arguments" << featureMessage.arguments();

		emit featureMessageReceived(  featureMessage );

		return true;
	}
	else
	{
		qCritical( "ItalcCoreConnection::handleServerMessage(): "
				"unknown message type %d from server. Closing "
				"connection. Will re-open it later.", (int) msg );
		return false;
	}

	return true;
}
