/*
 * isd_connection.cpp - client-implementation for ISD (iTALC Service Daemon)
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*#define NO_QTCPSOCKET_CONNECT*/


#ifdef NO_QTCPSOCKET_CONNECT

#include <stdio.h>

#ifdef BUILD_WIN32

#include <io.h>
#include <winsock.h>
#define lasterror WSAGetLastError()

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define lasterror errno

#endif /* BUILD_WIN32 */

#include <sys/types.h>

#endif /* NO_QTCPSOCKET_CONNECT */


#include <QtCore/QDir>
#include <QtCore/QFileInfo>


#include "isd_connection.h"
#include "dsa_key.h"
#include "local_system.h"


static privateDSAKey * privDSAKey = NULL;

ISD::userRoles __role = ISD::RoleOther;

QByteArray __appInternalChallenge;


bool isdConnection::initAuthentication( void )
{
	if( privDSAKey != NULL )
	{
		qWarning( "isdConnection::initAuthentication(): private key "
							"already initialized" );
		delete privDSAKey;
	}

	const QString priv_key_file = localSystem::privateKeyPath( __role );

	privDSAKey = new privateDSAKey( priv_key_file );

	return( privDSAKey->isValid() );

/*	// key valid (i.e. could be loaded)?
	if( !privDSAKey->isValid() )
	{
		// no, then create a new one
		delete privDSAKey;
		// generate 1024bit RSA-key and save it
		privDSAKey = new privateDSAKey( 1024 );
		if( !privDSAKey->isValid() )
		{
			qCritical( "isdConnection::initAuthentication(): "
						"key generation failed!" );
			return( FALSE );
		}
		privDSAKey->save( priv_key_file );

		// derive public key from private key and save it
		publicDSAKey( *privDSAKey ).save(
					localSystem::publicKeyPath( __role ) );

		return( FALSE );
	}
	return( TRUE );*/
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




#ifdef NO_QTCPSOCKET_CONNECT

bool connectToHost( const QString & _host, int _port, QTcpSocket * _sock )
{
	static bool initialized = FALSE;
	if( initialized == FALSE )
	{
		initialized = TRUE;
#ifdef BUILD_WIN32
		// Initialise WinPoxySockets on Win32
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD( 2, 0 );
		if( WSAStartup( wVersionRequested, &wsaData ) != 0 )
		{
			return( FALSE );
		}
#else
		// Disable the nasty read/write failure signals on UNIX
		signal( SIGPIPE, SIG_IGN );
#endif
	}
	const int one = 1;
	int sock = socket( AF_INET, SOCK_STREAM, 0 );
	// Create the socket
	if( sock < 0 )
	{
		return( FALSE );
	}

	// Set the socket options:
	if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one,
							sizeof( one ) ) )
	{
		return( FALSE );
	}
	if( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &one,
							sizeof( one ) ) )
	{
		return FALSE;
	}

	// Create an address structure and clear it
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	// Fill in the address if possible
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr( _host.toAscii().constData() );

	// Was the string a valid IP address?
	if( (signed int) addr.sin_addr.s_addr == -1 )
	{
		// No, so get the actual IP address of the host name specified
		struct hostent *pHost;
		pHost = gethostbyname( _host.toAscii().constData() );
		if( pHost != NULL )
		{
			if( pHost->h_addr == NULL )
			{
				return( FALSE );
			}
			addr.sin_addr.s_addr =
				((struct in_addr *)pHost->h_addr)->s_addr;
		}
		else
		{
			return( FALSE );
		}
	}

	// Put the socket into non-blocking mode
#ifdef __WIN32__
	u_long arg = 1;
	if( ioctlsocket( sock, FIONBIO, &arg ) != 0 )
	{
		return( FALSE );
	}
#else
	if( fcntl( sock, F_SETFL, O_NONBLOCK ) != 0 )
	{
		return( FALSE );
	}
#endif

	// Set the port number in the correct format
	addr.sin_port = htons( _port );

	// Actually connect the socket
	int res = connect( sock, (struct sockaddr *) &addr, sizeof( addr ) );
	if( res < 0 )
	{
		if( lasterror ==
	#ifdef BUILD_WIN32
				WSAEWOULDBLOCK
	#else
				EINPROGRESS
	#endif
						)
		{
			do
			{
				fd_set s;
				timeval tv;
				tv.tv_sec = 3;
				tv.tv_usec = 0;
				FD_ZERO( &s );
				FD_SET( sock, &s );
				res = select( sock+1, NULL, &s, NULL, &tv );
				if( res < 0 && lasterror != EINTR )
				{
					return( FALSE );
				}
				else if( res > 0 )
				{
					socklen_t lon = sizeof( int );
					int valopt;
					if( getsockopt( sock, SOL_SOCKET,
							SO_ERROR,
							(char *) &valopt,
							&lon ) < 0 )
					{
						return( FALSE );
					}
					if( valopt )
					{
						return( FALSE );
					}
					break;
				}
				else	// timeout
				{
					return( FALSE );
				}
			} while( 1 );
		}
		else
		{
			return( FALSE );
		}
	}

	_sock->setSocketDescriptor( sock );

	return( TRUE );
}
#endif



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

#ifdef NO_QTCPSOCKET_CONNECT
	if( !connectToHost( m_host, m_port, m_socket ) )
#else
	m_socket->connectToHost( m_host, m_port );
	if( m_socket->error() == QTcpSocket::HostNotFoundError ||
			m_socket->error() == QTcpSocket::NetworkError )
#endif
	{
		return( m_state = HostUnreachable );
	}

#ifndef NO_QTCPSOCKET_CONNECT
	m_socket->waitForConnected( 3000 );
#endif

	if( m_socket->state() != QTcpSocket::ConnectedState )
	{
		qDebug( "isdConnection::open(): unable to connect to server "
                               "on client %s:%i", m_host.toAscii().constData(),
								m_port );
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
		delete m_socket;
		m_socket = NULL;
	}

	m_user = "";
}




isdConnection::states isdConnection::reset( const QString & _host, int * _cnt )
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

	const states s = open();
	if( s == Connected && _cnt != NULL )
	{
		*_cnt = 0;
	}
	return( s );
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
	int tries = 0;

	while( bytes_done < _n )
	{
		int bytes_read = m_socket->read( _out + bytes_done,
							_n - bytes_done );
		if( bytes_read < 0 )
		{
			qWarning( "isdConnection::readFromServer(): "
					"server closed connection: %d",
							m_socket->error() );
			close();
			return( FALSE );
		}
		else if( bytes_read == 0 )
		{
			// could not read data because we're not connected
			// anymore?
			if( m_socket->state() != QTcpSocket::ConnectedState ||
								++tries > 60 )
			{
				qWarning( "isdConnection::readFromServer(): "
						"connection failed: %d",
							m_socket->state() );
				m_state = ConnectionFailed;
				return( FALSE );
			}
			m_socket->waitForReadyRead( 50 );
		}
		else
		{
			bytes_done += bytes_read;
			tries /= 2;
		}
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
			qCritical( "isdConnection::writeToServer(...): "
							"write(..) failed" );
			close();
			return( FALSE );
		}
		bytes_done += bytes_written;
	}

	return( m_socket->waitForBytesWritten( 100 ) );
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
		qCritical( "isdConnection::protocolInitialization(): "
					"not a valid iTALC Service Daemon" );
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
	Q_UINT8 num_sec_types = 0;
	if( !readFromServer( (char *)&num_sec_types, 1 )
							|| num_sec_types < 1 )
	{
		return( m_state = ConnectionFailed );
	}

	bool auth_handled = FALSE;
	bool italc_auth = FALSE;
	for( Q_UINT8 i = 0; i < num_sec_types; ++i )
	{
		Q_UINT8 auth_type = 0;
		if( !readFromServer( (char *)&auth_type, 1 ) )
		{
			return( m_state = ConnectionFailed );
		}

		if( auth_handled )
		{
			continue;
		}

		if( auth_type == rfbNoAuth )
		{
			qDebug( "no auth" );
			if( !writeToServer( (char *)&auth_type, 1 ) )
			{
				return( m_state = ConnectionFailed );
			}
			auth_handled = TRUE;
			continue;
		}
		else if( auth_type == rfbSecTypeItalc )
		{
			italc_auth = TRUE;
			qDebug( "italcauth" );
			if( !writeToServer( (char *)&auth_type, 1 ) )
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
				m_socketDev.write( QVariant( (int) __role ) );
				if( !privDSAKey )
				{
					isdConnection::initAuthentication();
				}
				m_socketDev.write( privDSAKey->sign( chall ) );
			}
			else if( iat == ItalcAuthAppInternalChallenge )
			{
				// wait for signal
				m_socketDev.read();
				m_socketDev.write( QVariant(
					__appInternalChallenge ) );
			}
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
	qCritical( "isdConnection::authAgainstServer(): "
					"unhandled italc-auth-mechanism!" );
			}
			auth_handled = TRUE;
		}
		// even last security type not handled?
		else if( i == num_sec_types - 1 )
		{
			qCritical( "isdConnection::authAgainstServer(): "
					"unknown sec-type for authentication: "
						"%d", (int) auth_type );
			m_state = AuthFailed;
		}
	}

	if( m_state == Connecting )
	{
		if( italc_auth )
		{
			const uint res = m_socketDev.read().toUInt();
			if( res != ItalcAuthOK )
			{
				return( m_state = AuthFailed );
			}
		}
		else
		{
			uint32_t res = 0;
			if( !readFromServer( (char *)&res, 4 ) )
			{
				return( m_state = ConnectionFailed );
			}
			if( res != rfbVncAuthOK )
			{
				return( m_state = AuthFailed );
			}
			
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
	return( ISD::msg( &m_socketDev, ISD::ExecCmds ).
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
	return( ISD::msg( &m_socketDev, ISD::LogonUserCmd ).
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



/*
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
*/



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
	return( ISD::msg( &m_socketDev, ISD::PowerDownComputer ).send() );
}




bool isdConnection::restartComputer( void )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::RestartComputer ).send() );
}




bool isdConnection::disableLocalInputs( bool _disabled )
{
	if( m_socket == NULL ||
			m_socket->state() != QTcpSocket::ConnectedState )
	{
		m_state = Disconnected;
		return( FALSE );
	}
	return( ISD::msg( &m_socketDev, ISD::DisableLocalInputs ).
				addArg( "disabled", _disabled ).send() );
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



#include "isd_connection.moc"

