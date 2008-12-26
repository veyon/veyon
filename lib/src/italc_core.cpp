/*
 * italc_core.cpp - implementation of iTALC Core
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <italcconfig.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "italc_core.h"
#include "dsa_key.h"
#include "local_system.h"


static privateDSAKey * privDSAKey = NULL;

ItalcCore::UserRoles __role = ItalcCore::RoleOther;

QByteArray __appInternalChallenge;


qint64 libvncClientDispatcher( char * _buf, const qint64 _len,
				const SocketOpCodes _op_code, void * _user )
{
	rfbClient * cl = (rfbClient *) _user;
	switch( _op_code )
	{
		case SocketRead:
			return( ReadFromRFBServer( cl, _buf, _len ) != 0 ?
								_len : 0 );
		case SocketWrite:
			return( WriteToRFBServer( cl, _buf, _len ) != 0 ?
								_len : 0 );
		case SocketGetPeerAddress:
//			strncpy( _buf, cl->host, _len );
			break;
	}
	return( 0 );

}




void ItalcCore::initAuthentication( void )
{
	if( privDSAKey != NULL )
	{
		qWarning( "isdConnection::initAuthentication(): private key "
							"already initialized" );
		delete privDSAKey;
		privDSAKey = NULL;
	}

	const QString priv_key_file = localSystem::privateKeyPath( __role );
	if( priv_key_file == "" )
	{
		return;
	}
	privDSAKey = new privateDSAKey( priv_key_file );

//	return( privDSAKey->isValid() );
}




int handleSecTypeItalc( rfbClient * _cl )
{
	socketDevice socketDev( libvncClientDispatcher, _cl );

	int iat = socketDev.read().toInt();
/*	if( _try_auth_type == ItalcAuthChallengeViaAuthFile ||
	_try_auth_type == ItalcAuthAppInternalChallenge )
	{
		iat = _try_auth_type;
	}*/
	socketDev.write( QVariant( iat ) );

	if( iat == ItalcAuthDSA || iat == ItalcAuthLocalDSA )
	{
		QByteArray chall = socketDev.read().toByteArray();
		socketDev.write( QVariant( (int) __role ) );
		if( !privDSAKey )
		{
			ItalcCore::initAuthentication();
		}
		socketDev.write( privDSAKey->sign( chall ) );
	}
	else if( iat == ItalcAuthAppInternalChallenge )
	{
		// wait for signal
		socketDev.read();
		socketDev.write( QVariant( __appInternalChallenge ) );
	}
	else if( iat == ItalcAuthChallengeViaAuthFile )
	{
		QFile file( socketDev.read().toString() );
		file.open( QFile::ReadOnly );
		socketDev.write( QVariant( file.readAll() ) );
	}
	else if( iat == ItalcAuthHostBased )
	{
	}
	else if( iat == ItalcAuthNone )
	{
	}
	const uint res = socketDev.read().toUInt();
	return( res == ItalcAuthOK );
}



#if 0
bool isdConnection::handleServerMessage( Q_UINT8 _msg )
{
	if( _msg == rfbItalcServiceResponse )
	{
		Q_UINT8 cmd;
		if( !readFromServer( (char *) &cmd, sizeof( cmd ) ) )
		{
			return( FALSE );
		}
		switch( cmd )
		{
			case ItalcCore::UserInformation:
			{
				ItalcCore::msg m( &m_socketDev );
				m.receive();
				m_user = m.arg( "username" ).toString();
				m_userHomeDir = m.arg( "homedir" ).toString();
				break;
			}

/*			case ItalcCore::DemoServer_PortInfo:
			{
				ItalcCore::msg m( &m_socketDev );
				m.receive();
				m_demoServerPort =
					m.arg( "demoserverport" ).toInt();
				break;
			}*/

			default:
	qCritical( "isdConnection::handleServerMessage(): unknown server "
						"response %d", (int) cmd );
				return( FALSE );
		}
	}
	else
	{
		qCritical( "isdConnection::handleServerMessage(): unknown "
				"message type %d from server. Closing "
				"connection. Will re-open it later.", _msg );
		close();
		return( FALSE );
	}

	return( TRUE );
}



bool isdConnection::sendGetUserInformationRequest( void )
{
	return( ItalcCore::msg( &m_socketDev, ItalcCore::GetUserInformation ).send() );
}




bool isdConnection::execCmds( const QString & _cmd )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::ExecCmds ).
					addArg( "cmds", _cmd ).send() );
}




bool isdConnection::startDemo( const QString & _port, bool _full_screen )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, _full_screen ?
			ItalcCore::StartFullScreenDemo : ItalcCore::StartWindowDemo ).
					addArg( "port", _port ).send() );
}




bool isdConnection::stopDemo( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::StopDemo ).send() );
}




bool isdConnection::lockDisplay( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::LockDisplay ).send() );
}




bool isdConnection::unlockDisplay( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::UnlockDisplay ).send() );
}




bool isdConnection::logonUser( const QString & _uname, const QString & _pw,
						const QString & _domain )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::LogonUserCmd ).
				addArg( "uname", _uname ).
				addArg( "passwd", _pw ).
				addArg( "domain", _domain ).send() );
}




bool isdConnection::logoutUser( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::LogoutUser ).send() );
}




bool isdConnection::displayTextMessage( const QString & _msg )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::DisplayTextMessage ).
					addArg( "msg", _msg ).send() );
}




bool isdConnection::sendFile( const QString & _fname )
{
	return( TRUE );
}




bool isdConnection::collectFiles( const QString & _nfilter )
{
	return( TRUE );
}




bool isdConnection::wakeOtherComputer( const QString & _mac )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::WakeOtherComputer ).
					addArg( "mac", _mac ).send() );
}




bool isdConnection::powerDownComputer( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::PowerDownComputer ).send() );
}




bool isdConnection::restartComputer( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::RestartComputer ).send() );
}




bool isdConnection::disableLocalInputs( bool _disabled )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::DisableLocalInputs ).
				addArg( "disabled", _disabled ).send() );
}




bool isdConnection::setRole( const ItalcCore::userRoles _role )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::SetRole ).
					addArg( "role", _role ).send() );
}




bool isdConnection::demoServerRun( int _quality, int _port )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	m_demoServerPort = _port;
	return( ItalcCore::msg( &m_socketDev, ItalcCore::DemoServer_Run ).
					addArg( "port", _port ).
					addArg( "quality", _quality ).send() );
}




bool isdConnection::demoServerAllowClient( const QString & _client )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::DemoServer_AllowClient ).
					addArg( "client", _client ).send() );
}




bool isdConnection::demoServerDenyClient( const QString & _client )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::DemoServer_DenyClient ).
					addArg( "client", _client ).send() );
}




bool isdConnection::hideTrayIcon( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ItalcCore::msg( &m_socketDev, ItalcCore::HideTrayIcon ).send() );
}




bool isdConnection::handleServerMessages( void )
{
	while( hasData() )
	{

		rfbServerToClientMsg msg;
		if( !readFromServer( (char *) &msg, sizeof( Q_UINT8 ) ) )
		{
			qCritical( "isdConnection::handleServerMessage(): "
						"reading message-type failed" );
			return( FALSE );
		}

		if( handleServerMessage( msg.type ) == FALSE )
		{
			return( FALSE );
		}

	}

	return( TRUE );
}
#endif

