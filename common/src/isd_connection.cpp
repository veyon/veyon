/*
 * isd_connection.cpp - client-implementation for ISD (iTALC Service Daemon)
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QFileInfo>


#include "isd_connection.h"
#include "dsa_key.h"
#include "local_system.h"

#ifdef BUILD_ICA
#include "isd_server.h"
#endif


static privateDSAKey * privDSAKey = NULL;

ISD::userRoles __role = ISD::RoleOther;


bool isdConnection::initAuthentication( void )
{
	if( privDSAKey != NULL )
	{
		printf( "private key already initialized" );
		return( TRUE );
	}

/*	QString priv_key_file = QDir::homePath() + QDir::separator() +
						localSystem::privateKeysPath() +
						QDir::separator() + "id_dsa";
	if( QDir::separator() != '/' )
	{
		priv_key_file = priv_key_file.replace( '/', QDir::separator() ).
				replace( QString( QDir::separator() ) +
							QDir::separator(),
							QDir::separator() );
	}*/

	const QString priv_key_file = localSystem::privateKeyPath( __role );

	privDSAKey = new privateDSAKey( priv_key_file );

	// key valid (i.e. could be loaded)?
	if( !privDSAKey->isValid() )
	{
		// no, then create a new one
		delete privDSAKey;
		// generate 1024bit RSA-key and save it
		privDSAKey = new privateDSAKey( 1024 );
		if( !privDSAKey->isValid() )
		{
			printf( "key generation failed!\n" );
			return( FALSE );
		}
		privDSAKey->save( priv_key_file );

		// derive public key from private key and save it
		publicDSAKey( *privDSAKey ).save(
					localSystem::publicKeyPath( __role ) );

		return( FALSE );
	}
	return( TRUE );
}




// normal ctor
isdConnection::isdConnection( const QString & _host, QObject * _parent ) :
	QObject( _parent ),
	m_socket( NULL ),
	m_state( Disconnected ),
	m_socketDev( qtcpsocketDispatcher, NULL ),
	m_host( _host ),
	m_port( PortOffsetISD ),
	m_demoServerPort( 0 ),
	m_user( "" )
{
	if( m_host.contains( ':' ) )
	{
		m_port = m_host.section( ':', 1, 1 ).toInt();
		m_host = m_host.section( ':', 0, 0 );
	}
}




isdConnection::~isdConnection()
{
	close();
}




isdConnection::states isdConnection::open( void )
{
	if( m_state != Disconnected )
	{
		close();
	}

	m_state = Connecting;

	// can use QTcpSocket's only in the same thread, therefore we must not
	// create it in constructor as it is called by another thread than
	// the thread actually using isdConnection
	if( m_socket == NULL )
	{
		m_socket = new QTcpSocket;
		m_socketDev.setUser( m_socket );
	}

	m_socket->connectToHost( m_host, m_port );
	if( m_socket->error() == QTcpSocket::HostNotFoundError ||
			m_socket->error() == QTcpSocket::NetworkError )
	{
		return( m_state = HostUnreachable );
	}

	m_socket->waitForConnected( 1000 );

	if( m_socket->state() != QTcpSocket::ConnectedState )
	{
/*		printf( "Unable to connect to VNC server on client %s\n",
					m_host.toAscii().constData() );*/
		
		if( m_socket->error() == QTcpSocket::ConnectionRefusedError )
		{
			return( m_state = ConnectionRefused );
		}
		return( m_state = HostUnreachable );
	}

	protocolInitialization();

	if( m_state == Connecting || m_state == Connected )
	{
		return( m_state = Connected );
	}

	// something went wrong during initialization
	close();

	return( m_state );
}




void isdConnection::close( void )
{
	m_state = Disconnected;
	if( m_socket != NULL )
	{
		//m_socket->disconnectFromHost();
		m_socket->abort();
	}

	m_user = "";
}




isdConnection::states isdConnection::reset( const QString & _host )
{
	close();

	if( _host != "" )
	{
		m_host = _host;
		if( m_host.contains( ':' ) )
		{
			m_port = m_host.section( ':', 1, 1 ).toInt();
			m_host = m_host.section( ':', 0, 0 );
		}
	}

	return( open() );
}




bool isdConnection::readFromServer( char * _out, const unsigned int _n )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = ConnectionFailed;
		return( FALSE );
	}

	unsigned int bytes_done = 0;

	while( bytes_done < _n )
	{
		int bytes_read = m_socket->read( _out + bytes_done,
							_n - bytes_done);
		if( bytes_read < 0 )
		{
			printf( "VNC server closed connection: %d\n",
							m_socket->error() );
			close();
			return( FALSE );
		}
		else if( bytes_read == 0 )
		{
			// could not read data because we're not connected
			// anymore?
			if( m_socket->state() != QTcpSocket::ConnectedState )
			{
				printf( "connection failed: %d\n",
							m_socket->state() );
				m_state = ConnectionFailed;
				return( FALSE );
			}
			m_socket->waitForReadyRead( 50 );
		}
		bytes_done += bytes_read;
	}

	return( TRUE );
}




bool isdConnection::writeToServer( const char * _buf, const unsigned int _n )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = ConnectionFailed;
		return( FALSE );
	}

	unsigned int bytes_done = 0;

	while( bytes_done < _n )
	{
		int bytes_written = m_socket->write( _buf + bytes_done,
							_n - bytes_done );
		if( bytes_written < 0 )
		{
			printf( "writeToServer: write(..) failed\n" );
			close();
			return( FALSE );
		}
		bytes_done += bytes_written;
	}

	return( m_socket->waitForBytesWritten( 50 ) );
}




long isdConnection::readCompactLen( void )
{
	Q_UINT8 b;

	if( !readFromServer( (char *)&b, sizeof( b ) ) )
	{
		return( -1 );
	}

	long len = (int)b & 0x7F;
	if( b & 0x80 )
	{
		if( !readFromServer( (char *)&b, sizeof( b ) ) )
		{
			return( -1 );
		}
		len |= ( (int)b & 0x7f ) << 7;
		if( b & 0x80 )
		{
			if( !readFromServer( (char *)&b, 1 ) )
			{
				return( -1 );
			}
			len |= ( (int)b & 0xff ) << 14;
		}
	}

	return( len );
}




isdConnection::states isdConnection::protocolInitialization( void )
{
	isdProtocolVersionMsg protocol_version;

	if( !readFromServer( protocol_version, sz_isdProtocolVersionMsg ) )
	{
		return( m_state = ConnectionFailed );
	}

	protocol_version[sz_isdProtocolVersionMsg] = 0;

	int major, minor;
	if( sscanf( protocol_version, isdProtocolVersionFormat, &major,
							&minor ) != 2 )
	{
		printf( "Not a valid iTALC Service Daemon\n" );
		return( m_state = InvalidServer );
	}

	if( !writeToServer( protocol_version, sz_isdProtocolVersionMsg ) )
	{
		return( m_state = ConnectionFailed );
	}

	return( authAgainstServer() );
}




isdConnection::states isdConnection::authAgainstServer(
					const italcAuthTypes _try_auth_type )
{
	Q_UINT8 num_sec_types;
	if( !readFromServer( (char *)&num_sec_types, sizeof( num_sec_types ) ) )
	{
		return( m_state = ConnectionFailed );
	}

	Q_UINT8 * sec_type_list = new Q_UINT8[num_sec_types];
	if( !readFromServer( (char *)sec_type_list, num_sec_types *
						sizeof( *sec_type_list ) ) )
	{
		delete[] sec_type_list;
		return( m_state = ConnectionFailed );
	}

	for( Q_UINT8 i = 0; i < num_sec_types; ++i )
	{
		if( sec_type_list[i] == rfbNoAuth )
		{
			printf("no auth\n" );
			if( !writeToServer( (char *)&sec_type_list[i],
						sizeof( sec_type_list[i] ) ) )
			{
				m_state = ConnectionFailed;
			}
			delete[] sec_type_list;
			return( m_state );
		}
		else if( sec_type_list[i] == rfbSecTypeItalc )
		{
			printf("italcauth\n");
			if( !writeToServer( (char *)&sec_type_list[i],
						sizeof( sec_type_list[i] ) ) )
			{
				return( m_state = ConnectionFailed );
			}
			int iat = m_socketDev.read().toInt();
			if( _try_auth_type == ItalcAuthChallengeViaAuthFile ||
			_try_auth_type == ItalcAuthAppInternalChallenge )
			{
				iat = _try_auth_type;
			}
			m_socketDev.write( QVariant( iat ) );

			if( iat == ItalcAuthDSA || iat == ItalcAuthLocalDSA )
			{
				QByteArray chall =
					m_socketDev.read().toByteArray();
/*				m_socketDev.write( localSystem::currentUser().
							replace( ' ', '_' ) );*/
				m_socketDev.write( QVariant( (int) __role ) );
				if( !privDSAKey )
				{
					isdConnection::initAuthentication();
				}
				m_socketDev.write( privDSAKey->sign( chall ) );
			}
#ifdef BUILD_ICA
			else if( iat == ItalcAuthAppInternalChallenge )
			{
				// wait for signal
				m_socketDev.read();
				m_socketDev.write( QVariant(
					isdServer::s_appInternalChallenge ) );
			}
#endif
			else if( iat == ItalcAuthChallengeViaAuthFile )
			{
				QFile file( m_socketDev.read().toString() );
				file.open( QFile::ReadOnly );
				m_socketDev.write( QVariant(
							file.readAll() ) );
			}
			else if( iat == ItalcAuthHostBased )
			{
			}
			else if( iat == ItalcAuthNone )
			{
			}
			else
			{
				printf( "unhandled italc-auth-mechanism!\n" );
			}
			break;
		}
		// even last security type not handled?
		else if( i == num_sec_types - 1 )
		{
			printf( "unknown sec-type for authentication: "
					"%d\n", (int) sec_type_list[i] );
			m_state = AuthFailed;
		}
	}

	delete[] sec_type_list;

	if( m_state == Connecting )
	{
		const uint res = m_socketDev.read().toUInt();
		if( res != ItalcAuthOK )
		{
			return( m_state = AuthFailed );
		}
	}

	return( m_state );
}




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
			case ISD::UserInformation:
			{
				ISD::msg m( &m_socketDev );
				m.receive();
				m_user = m.arg( "username" ).toString();
				m_userHomeDir = m.arg( "homedir" ).toString();
				break;
			}

/*			case ISD::DemoServer_PortInfo:
			{
				ISD::msg m( &m_socketDev );
				m.receive();
				m_demoServerPort =
					m.arg( "demoserverport" ).toInt();
				break;
			}*/

			default:
				printf( "Unknown server response %d\n",
								(int) cmd );
				return( FALSE );
		}
	}
	else
	{
		printf( "Unknown message type %d from server. Closing "
			"connection. Will re-open it later.\n", _msg );
		close();
		return( FALSE );
	}

	return( TRUE );
}




bool isdConnection::sendGetUserInformationRequest( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::GetUserInformation ).send() );
}




bool isdConnection::execCmds( const QString & _cmd )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::GetUserInformation ).
					addArg( "cmds", _cmd ).send() );
}




bool isdConnection::startDemo( const QString & _master_ip, bool _full_screen )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, _full_screen ?
			ISD::StartFullScreenDemo : ISD::StartWindowDemo ).
					addArg( "ip", _master_ip ).send() );
}




bool isdConnection::stopDemo( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::StopDemo ).send() );
}




bool isdConnection::lockDisplay( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::LockDisplay ).send() );
}




bool isdConnection::unlockDisplay( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::UnlockDisplay ).send() );
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
	return( ISD::msg( &m_socketDev, ISD::LogonUser ).
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
	return( ISD::msg( &m_socketDev, ISD::LogoutUser ).send() );
}




bool isdConnection::displayTextMessage( const QString & _msg )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::DisplayTextMessage ).
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




bool isdConnection::remoteControlDisplay( const QString & _ip, bool _view_only )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	if( _view_only )
	{
		return( ISD::msg( &m_socketDev, ISD::ViewRemoteDisplay ).
						addArg( "ip", _ip ).send() );
	}
	return( ISD::msg( &m_socketDev, ISD::RemoteControlDisplay ).
						addArg( "ip", _ip ).send() );
}




bool isdConnection::wakeOtherComputer( const QString & _mac )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::WakeOtherComputer ).
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
	return( ISD::msg( &m_socketDev, ISD::PowerDownComputer).send() );
}




bool isdConnection::restartComputer( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::RestartComputer).send() );
}




bool isdConnection::setRole( const ISD::userRoles _role )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::SetRole ).
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
	return( ISD::msg( &m_socketDev, ISD::DemoServer_Run ).
					addArg( "port", _port ).
					addArg( "quality", _quality ).send() );
}




bool isdConnection::demoServerStop( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::DemoServer_Stop ).send() );
}




bool isdConnection::demoServerAllowClient( const QString & _client )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::DemoServer_AllowClient ).
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
	return( ISD::msg( &m_socketDev, ISD::DemoServer_DenyClient ).
					addArg( "client", _client ).send() );
}




bool isdConnection::handleServerMessages( void )
{
	while( hasData() )
	{

		rfbServerToClientMsg msg;
		if( !readFromServer( (char *) &msg, sizeof( Q_UINT8 ) ) )
		{
			printf( "Reading message-type failed\n" );
			return( FALSE );
		}

		if( handleServerMessage( msg.type ) == FALSE )
		{
			return( FALSE );
		}

	}

	return( TRUE );
}



#include "isd_connection.moc"

