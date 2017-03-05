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


class ItalcMessageEvent : public MessageEvent
{
public:
	ItalcMessageEvent( const ItalcCore::Msg &m ) :
		m_msg( m )
	{
	}

	virtual void fire( rfbClient *client )
	{
		SocketDevice socketDev( libvncClientDispatcher, client );
		m_msg.setIoDevice( &socketDev );
		qDebug() << "ItalcMessageEvent::fire(): sending message" << m_msg.cmd()
					<< "with arguments" << m_msg.args();
		m_msg.send();
	}


private:
	ItalcCore::Msg m_msg;

} ;


class FeatureMessageEvent : public MessageEvent
{
public:
	FeatureMessageEvent( const FeatureMessage& featureMessage ) :
		m_featureMessage( featureMessage )
	{
	}

	virtual void fire( rfbClient *client )
	{
		qDebug() << "FeatureMessageEvent::fire(): sending message" << m_featureMessage.featureUid()
				 << "with arguments" << m_featureMessage.arguments();

		SocketDevice socketDevice( libvncClientDispatcher, client );
		char messageType = rfbItalcFeatureMessage;
		socketDevice.write( &messageType, sizeof(messageType) );

		m_featureMessage.send( &socketDevice );
	}


private:
	FeatureMessage m_featureMessage;

} ;



static rfbClientProtocolExtension * __italcProtocolExt = NULL;
static void * ItalcCoreConnectionTag = (void *) PortOffsetVncServer; // an unique ID



ItalcCoreConnection::ItalcCoreConnection( ItalcVncConnection *vncConn ):
	m_vncConn( vncConn ),
	m_user(),
	m_userHomeDir(),
	m_slaveStateFlags( 0 )
{
	if( __italcProtocolExt == NULL )
	{
		__italcProtocolExt = new rfbClientProtocolExtension;
		__italcProtocolExt->encodings = NULL;
		__italcProtocolExt->handleEncoding = NULL;
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




rfbBool ItalcCoreConnection::handleItalcMessage( rfbClient *cl,
						rfbServerToClientMsg * msg )
{
	ItalcCoreConnection * icc = (ItalcCoreConnection *)
				rfbClientGetClientData( cl, ItalcCoreConnectionTag );
	if( icc )
	{
		return icc->handleServerMessage( cl, msg->type );
	}

	return false;
}




bool ItalcCoreConnection::handleServerMessage( rfbClient *cl, uint8_t msg )
{
	if( msg == rfbItalcCoreResponse )
	{
		SocketDevice socketDev( libvncClientDispatcher, cl );
		ItalcCore::Msg m( &socketDev );

		m.receive();
		qDebug() << "ItalcCoreConnection: received message" << m.cmd()
					<< "with arguments" << m.args();

		if( m.cmd() == ItalcCore::UserInformation )
		{
			m_user = m.arg( "username" );
			m_userHomeDir = m.arg( "homedir" );
			emit receivedUserInfo();
		}
		else
		{
			qCritical() << "ItalcCoreConnection::"
				"handleServerMessage(): unknown server "
				"response" << m.cmd();
			return false;
		}
	}
	else
	{
		qCritical( "ItalcCoreConnection::handleServerMessage(): "
				"unknown message type %d from server. Closing "
				"connection. Will re-open it later.", msg );
		return false;
	}

	return true;
}




void ItalcCoreConnection::sendGetUserInformationRequest()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::GetUserInformation ) );
}




void ItalcCoreConnection::execCmds( const QString &cmd )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::ExecCmds ).
						addArg( "cmds", cmd ) );
}



void ItalcCoreConnection::logonUser( const QString &uname,
						const QString &pw,
						const QString &domain )
{
/*	enqueueMessage( ItalcCore::Msg( ItalcCore::LogonUserCmd ).
						addArg( "uname", uname ).
						addArg( "passwd", pw ).
						addArg( "domain", domain ) );*/
}




void ItalcCoreConnection::logoutUser()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::LogoutUser ) );
}



void ItalcCoreConnection::setRole( const ItalcCore::UserRole role )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::SetRole ).
						addArg( "role", role ) );
}



void ItalcCoreConnection::enqueueMessage( const ItalcCore::Msg &msg )
{
	ItalcCore::Msg m( msg );
	if (!m_vncConn)
	{
		ilog(Error, "ItalcCoreConnection: cannot call enqueueEvent - m_vncConn is NULL");
		return;
	}
	m_vncConn->enqueueEvent( new ItalcMessageEvent( m ) );
}
