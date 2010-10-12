/*
 * ItalcCoreConnection.cpp - implementation of ItalcCoreConnection
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QDebug>

#include "ItalcCoreConnection.h"


class ItalcMessageEvent : public ClientEvent
{
public:
	ItalcMessageEvent( const ItalcCore::Msg & _m ) :
		m_msg( _m )
	{
	}

	virtual void fire( rfbClient * _c )
	{
		m_msg.send();
	}


private:
	ItalcCore::Msg m_msg;

} ;



static rfbClientProtocolExtension * __italcProtocolExt = NULL;
static void * ItalcCoreConnectionTag = (void *) PortOffsetIVS; // an unique ID



ItalcCoreConnection::ItalcCoreConnection( ItalcVncConnection * _ivc ) :
	m_ivc( _ivc ),
	m_socketDev( libvncClientDispatcher ),
	m_user(),
	m_userHomeDir()
{
	if( __italcProtocolExt == NULL )
	{
		__italcProtocolExt = new rfbClientProtocolExtension;
		__italcProtocolExt->encodings = NULL;
		__italcProtocolExt->handleEncoding = NULL;
		__italcProtocolExt->handleMessage = handleItalcMessage;

		rfbClientRegisterExtension( __italcProtocolExt );
	}

	connect( m_ivc, SIGNAL( newClient( rfbClient * ) ),
			this, SLOT( initNewClient( rfbClient * ) ),
							Qt::DirectConnection );
}




ItalcCoreConnection::~ItalcCoreConnection()
{
	if( m_ivc )
	{
		m_ivc->stop();
		delete m_ivc;
	}
}




void ItalcCoreConnection::initNewClient( rfbClient * _cl )
{
	m_socketDev.setUser( _cl );
	rfbClientSetClientData( _cl, ItalcCoreConnectionTag, this );
}




rfbBool ItalcCoreConnection::handleItalcMessage( rfbClient * cl,
						rfbServerToClientMsg * msg )
{
	ItalcCoreConnection * icc = (ItalcCoreConnection *)
				rfbClientGetClientData( cl, ItalcCoreConnectionTag );
	return icc->handleServerMessage( cl, msg->type );
}




bool ItalcCoreConnection::handleServerMessage( rfbClient *cl, uint8_t _msg )
{
	if( _msg == rfbItalcCoreResponse )
	{
		ItalcCore::Msg m( &m_socketDev );
		m.receive();
		if( m.cmd() == ItalcCore::UserInformation )
		{
			m_user = m.arg( "username" );
			m_userHomeDir = m.arg( "homedir" );
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
				"connection. Will re-open it later.", _msg );
		return false;
	}

	return true;
}




void ItalcCoreConnection::sendGetUserInformationRequest( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::GetUserInformation ) );
}




void ItalcCoreConnection::execCmds( const QString & _cmd )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::ExecCmds ).
						addArg( "cmds", _cmd ) );
}




void ItalcCoreConnection::startDemo( int _port, bool _full_screen )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::StartDemo ).
					addArg( "port", _port ).
					addArg( "fullscreen", _full_screen ) );
}




void ItalcCoreConnection::stopDemo( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::StopDemo ) );
}




void ItalcCoreConnection::lockDisplay( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::LockDisplay ) );
}




void ItalcCoreConnection::unlockDisplay( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::UnlockDisplay ) );
}




void ItalcCoreConnection::logonUser( const QString & _uname,
						const QString & _pw,
						const QString & _domain )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::LogonUserCmd ).
						addArg( "uname", _uname ).
						addArg( "passwd", _pw ).
						addArg( "domain", _domain ) );
}




void ItalcCoreConnection::logoutUser( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::LogoutUser ) );
}




void ItalcCoreConnection::displayTextMessage( const QString & _msg )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::DisplayTextMessage ).
						addArg( "text", _msg ) );
}




void ItalcCoreConnection::sendFile( const QString & _fname )
{
}




void ItalcCoreConnection::collectFiles( const QString & _nfilter )
{
}




void ItalcCoreConnection::powerOnComputer( const QString & _mac )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::PowerOnComputer ).
						addArg( "mac", _mac ) );
}




void ItalcCoreConnection::powerDownComputer( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::PowerDownComputer ) );
}




void ItalcCoreConnection::restartComputer( void )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::RestartComputer ) );
}




void ItalcCoreConnection::disableLocalInputs( bool _disabled )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::DisableLocalInputs ).
					addArg( "disabled", _disabled ) );
}




void ItalcCoreConnection::setRole( const ItalcCore::UserRoles _role )
{
	enqueueMessage( ItalcCore::Msg( ItalcCore::SetRole ).
						addArg( "role", _role ) );
}




void ItalcCoreConnection::enqueueMessage( const ItalcCore::Msg & _msg )
{
	ItalcCore::Msg m( _msg );
	m.setSocketDevice( &m_socketDev );
	m_ivc->enqueueEvent( new ItalcMessageEvent( m ) );
}


