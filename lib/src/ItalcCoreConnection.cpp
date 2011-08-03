/*
 * ItalcCoreConnection.cpp - implementation of ItalcCoreConnection
 *
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "ItalcCoreConnection.h"
#include "Logger.h"
#include "SocketDevice.h"


class ItalcMessageEvent : public ClientEvent
{
public:
	ItalcMessageEvent( const ItalcCore::Msg &m ) :
		m_msg( m )
	{
	}

	virtual void fire( rfbClient *client )
	{
		SocketDevice socketDev( libvncClientDispatcher, client );
		m_msg.setSocketDevice( &socketDev );
		qDebug() << "ItalcMessageEvent::fire(): sending message" << m_msg.cmd()
					<< "with arguments" << m_msg.args();
		m_msg.send();
	}


private:
	ItalcCore::Msg m_msg;

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

	connect( m_vncConn, SIGNAL( newClient( rfbClient * ) ),
			this, SLOT( initNewClient( rfbClient * ) ),
							Qt::DirectConnection );
}




ItalcCoreConnection::~ItalcCoreConnection()
{
	if( m_vncConn )
	{
		m_vncConn->stop();
		m_vncConn = NULL;
	}
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
		}
		else if( m.cmd() == ItalcCore::ReportSlaveStateFlags )
		{
			m_slaveStateFlags = m.arg( "slavestateflags" ).toInt();
		}
		// TODO: plugin hook
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




void ItalcCoreConnection::startDemo( const QString &host, int port,
										bool fullscreen )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::StartDemo ).
					addArg( "host", host ).
					addArg( "port", port ).
					addArg( "fullscreen", fullscreen ) );
}




void ItalcCoreConnection::stopDemo()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::StopDemo ) );
}




void ItalcCoreConnection::lockScreen()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::LockScreen ) );
}




void ItalcCoreConnection::unlockScreen()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::UnlockScreen ) );
}




void ItalcCoreConnection::lockInput()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::LockInput ) );
}




void ItalcCoreConnection::unlockInput()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::UnlockInput ) );
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




void ItalcCoreConnection::displayTextMessage( const QString &msg )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::DisplayTextMessage ).
						addArg( "text", msg ) );
}




void ItalcCoreConnection::powerOnComputer( const QString &mac )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::PowerOnComputer ).
						addArg( "mac",mac ) );
}




void ItalcCoreConnection::powerDownComputer()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::PowerDownComputer ) );
}




void ItalcCoreConnection::restartComputer()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::RestartComputer ) );
}




void ItalcCoreConnection::disableLocalInputs( bool disabled )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::DisableLocalInputs ).
					addArg( "disabled", disabled ) );
}




void ItalcCoreConnection::setRole( const ItalcCore::UserRole role )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::SetRole ).
						addArg( "role", role ) );
}




void ItalcCoreConnection::startDemoServer( int sourcePort, int destinationPort )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::StartDemoServer ).
						addArg( "sourceport", sourcePort ).
						addArg( "destinationport", destinationPort ) );
}




void ItalcCoreConnection::stopDemoServer()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::StopDemoServer ) );
}




void ItalcCoreConnection::demoServerAllowHost( const QString &host )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::DemoServerAllowHost ).
						addArg( "host", host ) );
}




void ItalcCoreConnection::demoServerUnallowHost( const QString &host )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::DemoServerUnallowHost ).
						addArg( "host", host ) );
}




void ItalcCoreConnection::reportSlaveStateFlags()
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::ReportSlaveStateFlags ) );
}



void ItalcCoreConnection::enqueueMessage( const ItalcCore::Msg &msg )
{
	ItalcCore::Msg m( msg );
	m_vncConn->enqueueEvent( new ItalcMessageEvent( m ) );
}


