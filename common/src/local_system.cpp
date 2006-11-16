/*
 * local_system.cpp - namespace localSystem, providing an interface for
 *                    transparent usage of operating-system-dependent functions
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


#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtNetwork/QTcpServer>


#ifdef BUILD_WIN32

#include <QtCore/QThread>

#define _WIN32_WINNT 0x0501
#include <windows.h>

#if _WIN32_WINNT >= 0x500
#define SHUTDOWN_FLAGS (EWX_FORCE | EWX_FORCEIFHUNG)
#else
#define SHUTDOWN_FLAGS (EWX_FORCE)
#endif

#if _WIN32_WINNT >= 0x501
#include <reason.h>
#define SHUTDOWN_REASON (SHTDN_REASON_MAJOR_SYSTEM/* SHTDN_REASON_MINOR_ENVIRONMENT*/)
#else
#define SHUTDOWN_REASON 0
#endif



class userPollThread : public QThread
{
public:
	userPollThread() : QThread()
	{
		start( LowPriority );
	}

	const QString & name( void ) const
	{
		QMutexLocker m( &m_mutex );
		return( m_name );
	}

private:
	virtual void run( void )
	{
		while( 1 )
		{
			QProcess p;
			p.start( "userinfo" );
			if( p.waitForFinished() )
			{
				QMutexLocker m( &m_mutex );
				( m_name = p.readAll() ).chop( 2 );
			}
		}
	}

	QString m_name;
	mutable QMutex m_mutex;

}  static * __user_poll_thread = NULL;


#else

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif


#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "local_system.h"


#ifdef BUILD_ICA

#ifdef BUILD_WIN32

#include "vncKeymap.h"

class vncServer;
extern vncServer * __server;

static inline void pressKey( int _key, bool _down )
{
	if( !__server )
	{
		return;
	}
	vncKeymap::keyEvent( _key, _down, __server );
}

#else

#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

#include "rfb/rfb.h"

extern "C"
{
#include "keyboard.h"
}


extern rfbClientPtr __client;

static inline void pressKey( int _key, bool _down )
{
	if( !__client )
	{
		return;
	}
	keyboard( _down, _key, __client );
}


#endif

#endif


namespace localSystem
{


void initialize( void )
{
	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "is.org" );
	QCoreApplication::setApplicationName( "iTALC" );

#ifdef BUILD_WIN32
	__user_poll_thread = new userPollThread();
#endif

}




int freePort( void )
{
	QTcpServer t;
	t.listen( QHostAddress::LocalHost );
	return( t.serverPort() );
}




void sleep( const int _ms )
{
#ifdef BUILD_WIN32
	Sleep( static_cast<unsigned int>( _ms ) );
#else
	struct timespec ts = { _ms / 1000, ( _ms % 1000 ) * 1000 * 1000 } ;
	nanosleep( &ts, NULL );
#endif
}




void execInTerminal( const QString & _cmds )
{
	QProcess::startDetached(
#ifdef BUILD_WIN32
			"cmd " +
#else
			"xterm -e " +
#endif
			_cmds );
}




void broadcastWOLPacket( const QString & _mac )
{
	const int PORT_NUM = 65535;
	const int MAC_SIZE = 6;
	const int OUTBUF_SIZE = MAC_SIZE*7;
	unsigned char mac[MAC_SIZE];
	char out_buf[OUTBUF_SIZE];

	if( sscanf( _mac.toAscii().constData(),
				"%2x:%2x:%2x:%2x:%2x:%2x",
				(unsigned int *) &mac[0],
				(unsigned int *) &mac[1],
				(unsigned int *) &mac[2],
				(unsigned int *) &mac[3],
				(unsigned int *) &mac[4],
				(unsigned int *) &mac[5] ) != MAC_SIZE )
	{
		printf( "Invalid MAC-address\n" );
		return;
	}

	for( int i = 0; i < MAC_SIZE; ++i )
	{
		out_buf[i] = 0xff;	  
	}

	for( int i = 1; i < 17; ++i )
	{
		for(int j = 0; j < MAC_SIZE; ++j )
		{
			out_buf[i*MAC_SIZE+j] = mac[j];
		}
	}

#ifdef BUILD_WIN32
	WSADATA info;
	if( WSAStartup( MAKEWORD( 1, 1 ), &info ) != 0 )
	{
		fprintf( stderr, "Cannot initialize WinSock!\n" );
		return;
	}
#endif

	// UDP-broadcast the MAC-address
	unsigned int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	struct sockaddr_in my_addr;
	my_addr.sin_family      = AF_INET;            // Address family to use
	my_addr.sin_port        = htons( PORT_NUM );    // Port number to use
	my_addr.sin_addr.s_addr = inet_addr( "255.255.255.255" ); // send to
								  // IP_ADDR

	int optval = 1;
	if( setsockopt( sock, SOL_SOCKET, SO_BROADCAST,(char *) &optval,
							sizeof( optval ) ) < 0 )
	{
		fprintf( stderr,"Can't set sockopt (%d).\n", errno );
		return;
	}

	sendto( sock, out_buf, sizeof( out_buf ), 0,
			(struct sockaddr*) &my_addr, sizeof( my_addr ) );
#ifdef BUILD_WIN32
	closesocket( sock );
	WSACleanup();
#else
	close( sock );
#endif


#if 0
#ifdef BUILD_LINUX
	QProcess::startDetached( "etherwake " + _mac );
#endif
#endif
}



void powerDown( void )
{
#ifdef BUILD_WIN32
	//ExitWindowsEx( EWX_POWEROFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
	enablePrivilege( SE_SHUTDOWN_NAME, TRUE );
	InitiateSystemShutdown( NULL,	// local machine
				NULL,	// message for shutdown-box
				0,	// no timeout or possibility to abort
					// system-shutdown
				TRUE,	// force closing all apps
				FALSE	// do not reboot
				);
	enablePrivilege( SE_SHUTDOWN_NAME, FALSE );
#else
	QProcess::startDetached( "halt" );
#endif
}




void reboot( void )
{
#ifdef BUILD_WIN32
	//ExitWindowsEx( EWX_REBOOT | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
	enablePrivilege( SE_SHUTDOWN_NAME, TRUE );
	InitiateSystemShutdown( NULL,	// local machine
				NULL,	// message for shutdown-box
				0,	// no timeout or possibility to abort
					// system-shutdown
				TRUE,	// force closing all apps
				TRUE	// reboot
				);
	enablePrivilege( SE_SHUTDOWN_NAME, FALSE );
#else
	QProcess::startDetached( "reboot" );
#endif
}




#ifdef BUILD_ICA
static inline void pressAndReleaseKey( int _key )
{
	pressKey( _key, TRUE );
	pressKey( _key, FALSE );
}


void logonUser( const QString & _uname, const QString & _passwd )
{
#ifdef BUILD_WIN32
	// send Secure Attention Sequence (SAS) for making sure we can enter
	// username and password
	pressKey( XK_Alt_L, TRUE );
	pressKey( XK_Control_L, TRUE );
	pressAndReleaseKey( XK_Delete );
	pressKey( XK_Control_L, FALSE );
	pressKey( XK_Alt_L, FALSE );
#endif
	for( int i = 0; i < _uname.size(); ++i )
	{
		pressAndReleaseKey( _uname.utf16()[i] );
	}
	pressAndReleaseKey( XK_Tab );
	for( int i = 0; i < _passwd.size(); ++i )
	{
		pressAndReleaseKey( _passwd.utf16()[i] );
	}
	pressAndReleaseKey( XK_Return );
}

#endif



void logoutUser( void )
{
#ifdef BUILD_WIN32
	ExitWindowsEx( EWX_LOGOFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
#else
	QProcess::startDetached( "killall Xorg" );
#endif
}



QString currentUser( void )
{
	QString ret = "unknown";

#ifdef BUILD_WIN32

	if( !__user_poll_thread->name().isEmpty() )
	{
		ret = __user_poll_thread->name();
	}

#else

	char * user_name = getenv( "USER" );
#ifdef HAVE_PWD_H
	struct passwd * pw_entry = NULL;
	if( user_name )
	{
		pw_entry = getpwnam( user_name );
	}
	if( !pw_entry )
	{
		pw_entry = getpwuid( getuid() );
	}
	if( pw_entry )
	{
		return( QString( "%1 (%2)" ).arg( pw_entry->pw_gecos ).
						arg( pw_entry->pw_name ) );
	}
#endif
	if( user_name )
	{
		return user_name;
	}

#endif

	return( ret );
}



static const QString userRoleNames[] =
{
	"none",
	"teacher",
	"admin",
	"supporter",
	"other"
} ;


QString userRoleName( const ISD::userRoles _role )
{
	return( userRoleNames[_role] );
}


inline QString keyPath( const ISD::userRoles _role, const QString _group,
							bool _only_path )
{
	QSettings settings( QSettings::SystemScope, "iTALC Solutions",
								"iTALC" );
	if( _role <= ISD::RoleNone || _role >= ISD::RoleCount )
	{
		printf( "invalid role\n" );
		return( "" );
	}
	const QString fallback_dir =
#ifdef BUILD_LINUX
		"/etc/italc/keys/"
#elif BUILD_WIN32
		"c:\\italc\\keys\\"
#endif
		+ _group + QDir::separator() + userRoleNames[_role] +
						QDir::separator() +
						( _only_path ? "" : "key" );
	const QString val = settings.value( "keypaths" + _group + "/" +
					userRoleNames[_role] ).toString();
	if( val.isEmpty() )
	{
		settings.setValue( "keypaths" + _group + "/" +
					userRoleNames[_role], fallback_dir );
		return( fallback_dir );
	}
	return( val );
}


QString privateKeyPath( const ISD::userRoles _role, bool _only_path )
{
	return( keyPath( _role, "private", _only_path ) );
}


QString publicKeyPath( const ISD::userRoles _role, bool _only_path )
{
	return( keyPath( _role, "public", _only_path ) );
}




QString snapshotDir( void )
{
	QSettings settings;
	return( settings.value( "paths/snapshots", personalConfigDir() +
						"snapshots" ).toString() +
							QDir::separator() );
}




QString globalConfigPath( void )
{
	QSettings settings;
	return( settings.value( "paths/globalconfig", personalConfigDir() +
					"globalconfig.xml" ).toString() );
}




QString personalConfigDir( void )
{
	QSettings settings;
	const QString d = settings.value( "paths/personalconfig" ).toString();
	return( d.isEmpty() ?
				QDir::homePath() + QDir::separator() +
				".italc" + QDir::separator()
			:
				d );
}




QString personalConfigPath( void )
{
	QSettings settings;
	const QString d = settings.value( "paths/personalconfig" ).toString();
	return( d.isEmpty() ?
				personalConfigDir() + "personalconfig.xml"
			:
				d );
}


bool ensurePathExists( const QString & _path )
{
	if( _path.isEmpty() || QDir( _path ).exists() )
	{
		return( TRUE );
	}

	QString p = QDir( _path ).absolutePath();
	if( !QFileInfo( _path ).isDir() )
	{
		p = QFileInfo( _path ).absolutePath();
	}
	QStringList dirs;
	while( !QDir( p ).exists() && !p.isEmpty() )
	{
		dirs.push_front( QDir( p ).dirName() );
		p.chop( dirs.front().size() + 1 );
	}
	if( !p.isEmpty() )
	{
		return( QDir( p ).mkpath( dirs.join( QDir::separator() ) ) );
	}
	return( FALSE );
}



#ifdef BUILD_WIN32
BOOL enablePrivilege( LPCTSTR lpszPrivilegeName, BOOL bEnable )
{
	HANDLE			hToken;
	TOKEN_PRIVILEGES	tp;
	LUID			luid;
	BOOL			ret;

	if( !OpenProcessToken( GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken ) )
	{
		return FALSE;
	}

	if( !LookupPrivilegeValue( NULL, lpszPrivilegeName, &luid ) )
	{
		return FALSE;
	}

	tp.PrivilegeCount           = 1;
	tp.Privileges[0].Luid       = luid;
	tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	ret = AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );

	CloseHandle( hToken );

	return ret;
}
#endif


} // end of namespace localSystem

