/*
 * local_system.cpp - namespace localSystem, providing an interface for
 *                    transparent usage of operating-system-specific functions
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtGui/QWidget>
#include <QtNetwork/QTcpServer>


#ifdef BUILD_WIN32

#include <QtCore/QThread>
#include <QtCore/QLibrary>

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <shlobj.h>
#include <psapi.h>
#include <winable.h>

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

	const QString & name( void )
	{
		QMutexLocker m( &m_mutex );
		return( m_name );
	}


private:
	virtual void run( void )
	{
		while( 1 )
		{
		char buf[1024];
		STARTUPINFO si;
		SECURITY_ATTRIBUTES sa;
		SECURITY_DESCRIPTOR sd;	// security information for pipes
		PROCESS_INFORMATION pi;
		HANDLE newstdin, newstdout, read_stdout, write_stdin;
								// pipe handles

		// initialize security descriptor (Windows NT)
		InitializeSecurityDescriptor( &sd,
						SECURITY_DESCRIPTOR_REVISION );
		SetSecurityDescriptorDacl( &sd, true, NULL, FALSE );
		sa.lpSecurityDescriptor = &sd;
		sa.nLength = sizeof( SECURITY_ATTRIBUTES) ;
		sa.bInheritHandle = TRUE;	//allow inheritable handles

		// create stdin pipe
		if( !CreatePipe( &newstdin, &write_stdin, &sa, 0 ) )
		{
			qCritical( "CreatePipe (stdin)" );
			continue;
		}
		// create stdout pipe
		if( !CreatePipe( &read_stdout, &newstdout, &sa, 0 ) )
		{
			qCritical( "CreatePipe (stdout)" );
			CloseHandle( newstdin );
			CloseHandle( write_stdin );
			continue;
		}

		// set startupinfo for the spawned process
		GetStartupInfo( &si );
		si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		si.hStdOutput = newstdout;
		si.hStdError = newstdout;	// set the new handles for the
						// child process
		si.hStdInput = newstdin;
		QString app = QCoreApplication::applicationDirPath() +
					QDir::separator() + "userinfo.exe";
		app.replace( '/', QDir::separator() );

		// spawn the child process
		if( !CreateProcess( app.toAscii().constData(),
							NULL, NULL, NULL, TRUE,
				CREATE_NO_WINDOW, NULL,NULL, &si, &pi ) )
		{
			qCritical( "CreateProcess" );
			CloseHandle( newstdin );
			CloseHandle( newstdout );
			CloseHandle( read_stdout );
			CloseHandle( write_stdin );
			sleep( 5 );
			continue;
		}

		QString s;
		while( 1 )
		{
			DWORD bread, avail;

			//check to see if there is any data to read from stdout
			PeekNamedPipe( read_stdout, buf, 1023, &bread, &avail,
									NULL );
			if( bread != 0 )
			{
				while( avail > 0 )
				{
					memset( buf, 0, sizeof( buf ) );
					ReadFile( read_stdout, buf,
						qMin<long unsigned>( avail,
									1023 ),
							&bread, NULL );
					avail -= bread;
					s += buf;
					s.chop( 2 );
				}
			}

			if( s.count( ':' ) >= 1 )
			{
				QMutexLocker m( &m_mutex );
				m_name = s.section( ':', 0, 0 );
				m_name.remove( "\n" );
				s = s.section( ':', 1 );
			}

			DWORD exit;
			GetExitCodeProcess( pi.hProcess, &exit );
			if( exit != STILL_ACTIVE )
			{
				break;
			}
			sleep( 5 );
		}

		CloseHandle( pi.hThread );
		CloseHandle( pi.hProcess );
		CloseHandle( newstdin );
		CloseHandle( newstdout );
		CloseHandle( read_stdout );
		CloseHandle( write_stdin );

		sleep( 5 );

		} // end while( 1 )

	}

	QString m_name;
	QMutex m_mutex;

}  static * __user_poll_thread = NULL;



// taken from qt-win-opensource-src-4.2.2/src/corelib/io/qsettings.cpp
static QString windowsConfigPath( int _type )
{
	QString result;

	QLibrary library( "shell32" );
	typedef BOOL( WINAPI* GetSpecialFolderPath )( HWND, char *, int, BOOL );
	GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)
				library.resolve( "SHGetSpecialFolderPathA" );
	if( SHGetSpecialFolderPath )
	{
	    char path[MAX_PATH];
	    SHGetSpecialFolderPath( 0, path, _type, FALSE );
	    result = QString::fromLocal8Bit( path );
	}
	return( result );
}


#endif


#ifdef HAVE_PWD_H
#include <pwd.h>
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

static BOOL WINAPI consoleCtrlHandler( DWORD _dwCtrlType )
{
	switch( _dwCtrlType )
	{
		case CTRL_LOGOFF_EVENT: return TRUE;
		default: return FALSE;
	}
}



#include "vncKeymap.h"

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
#else
#define KeySym int
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

static QFile * __debug_out = NULL;

void msgHandler( QtMsgType _type, const char * _msg )
{
	if( __debug_out == NULL )
	{
#ifdef BUILD_WIN32
		const QString log_path = windowsConfigPath( CSIDL_WINDOWS ) +
							QDir::separator();
#else
		const QString log_path = "/var/log/";
#endif
		__debug_out = new QFile( log_path +
#ifdef BUILD_ICA
					"italc_client.log"
#else
					"italc_master.log"
#endif
								);
		__debug_out->open( QFile::WriteOnly | QFile::Append |
							QFile::Unbuffered );
	}

	QString out;
	switch( _type )
	{
		case QtDebugMsg:
			out = QString( "(debug)" );
			break;
		case QtWarningMsg:
			out = QString( "(warning)" );
			break;
		case QtCriticalMsg:
			out = QString( "(critical)" );
			break;
		case QtFatalMsg:
			out = QString( "(fatal)" );
			break;
	}

	out = QString( out + " %1" ).arg( _msg );
	if( out.right( 1 ) != "\n" )
	{
		out += "\n";
	}
	__debug_out->write( out.toAscii() );
}



namespace localSystem
{


void initialize( void )
{
	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "italcsolutions.org" );
	QCoreApplication::setApplicationName( "iTALC" );

	qInstallMsgHandler( msgHandler );

#ifdef BUILD_WIN32
#ifdef BUILD_ICA
	__user_poll_thread = new userPollThread();

	SetConsoleCtrlHandler( consoleCtrlHandler, TRUE );
#endif
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
	const int OUTBUF_SIZE = MAC_SIZE*17;
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
		qWarning( "invalid MAC-address" );
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
		qCritical( "cannot initialize WinSock!" );
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
	if( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char *) &optval,
							sizeof( optval ) ) < 0 )
	{
		qCritical( "can't set sockopt (%d).", errno );
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
	enablePrivilege( SE_SHUTDOWN_NAME, TRUE );
	ExitWindowsEx( EWX_POWEROFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
/*	InitiateSystemShutdown( NULL,	// local machine
				NULL,	// message for shutdown-box
				0,	// no timeout or possibility to abort
					// system-shutdown
				TRUE,	// force closing all apps
				FALSE	// do not reboot
				);*/
	enablePrivilege( SE_SHUTDOWN_NAME, FALSE );
#else
	QProcess::startDetached( "halt" );
#endif
}




void reboot( void )
{
#ifdef BUILD_WIN32
	enablePrivilege( SE_SHUTDOWN_NAME, TRUE );
	ExitWindowsEx( EWX_REBOOT | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
/*	InitiateSystemShutdown( NULL,	// local machine
				NULL,	// message for shutdown-box
				0,	// no timeout or possibility to abort
					// system-shutdown
				TRUE,	// force closing all apps
				TRUE	// reboot
				);*/
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


void logonUser( const QString & _uname, const QString & _passwd,
						const QString & _domain )
{
#ifdef BUILD_WIN32

	// first check for process "explorer.exe" - if we find it, a user
	// is logged in and we do not send our key-sequences as it probably
	// disturbes user
	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		return;
	}

	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	bool user_logged_on = FALSE;
	for( DWORD i = 0; i < cProcesses; i++ )
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
							FALSE, aProcesses[i] );
		HMODULE hMod;
		if( hProcess == NULL ||
			!EnumProcessModules( hProcess, &hMod, sizeof( hMod ),
								&cbNeeded ) )
	        {
			continue;
		}
		TCHAR szProcessName[MAX_PATH];
		GetModuleBaseName( hProcess, hMod, szProcessName, 
                             		  sizeof(szProcessName)/sizeof(TCHAR) );
		for( TCHAR * ptr = szProcessName; *ptr; ++ptr )
		{
			*ptr = tolower( *ptr );
		}
		if( strcmp( szProcessName, "explorer.exe" ) == 0 )
		{
			user_logged_on = TRUE;
			break;
		}
	}

	if( user_logged_on )
	{
		return;
	}

	// disable caps lock
	if( GetKeyState( VK_CAPITAL ) & 1 )
	{
		INPUT input[2];
		ZeroMemory( input, sizeof( input ) );        
		input[0].type = input[1].type = INPUT_KEYBOARD;
		input[0].ki.wVk = input[1].ki.wVk = VK_CAPITAL;        
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput( 2, input, sizeof( INPUT ) );
	}

	pressAndReleaseKey( XK_Escape );
	pressAndReleaseKey( XK_Escape );
	sleep( 50 );

	// send Secure Attention Sequence (SAS) for making sure we can enter
	// username and password
	pressKey( XK_Alt_L, TRUE );
	pressKey( XK_Control_L, TRUE );
	pressAndReleaseKey( XK_Delete );
	pressKey( XK_Control_L, FALSE );
	pressKey( XK_Alt_L, FALSE );

	const ushort * accels = QObject::tr(
		"UPL (note for translators: the first three characters of "
		"this string are the accellerators (underlined characters) "
		"of the three input-fields in logon-dialog of windows - "
		"you only need to \"translate\" these characters, this note "
						"can be ommited)" ).utf16();

	/* Need to handle 2 cases here; if an interactive login message is
         * defined in policy, this window will be displayed with an "OK" button;
         * if not the login window will be displayed. Sending a space will
         * dismiss the message, but will also add a space to the currently
         * selected field if the login windows is active. The solution is:
         *  1. send the "username" field accelerator (which won't do anything
         *     to the message window, but will select the "username" field of
         *     the login window if it is active)
         *  2. Send a space keypress to dismiss the message. (adds a space
         *     to username field of login window if active)
         *  3. Send the "username" field accelerator again; which will select
         *     the username field for the case where the message was displayed
         *  4. Send a backspace keypress to remove any space that was added to
         *     the username field if there is no message.
         */
	pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[0] );
	pressKey( XK_Alt_L, FALSE );

	pressAndReleaseKey( XK_space );

	pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[0] );
	pressKey( XK_Alt_L, FALSE );

	pressAndReleaseKey( XK_BackSpace );

	sleep( 1000 );
#endif

	for( int i = 0; i < _uname.size(); ++i )
	{
		pressAndReleaseKey( _uname.utf16()[i] );
	}

#ifdef BUILD_WIN32
	pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[1] );
	pressKey( XK_Alt_L, FALSE );
	sleep( 1000 );
#else
	pressAndReleaseKey( XK_Tab );
#endif

	for( int i = 0; i < _passwd.size(); ++i )
	{
		pressAndReleaseKey( _passwd.utf16()[i] );
	}

#ifdef BUILD_WIN32
	if( !_domain.isEmpty() )
	{
		pressKey( XK_Alt_L, TRUE );
		pressAndReleaseKey( accels[2] );
		pressKey( XK_Alt_L, FALSE );
		sleep( 1000 );
		for( int i = 0; i < _domain.size(); ++i )
		{
			pressAndReleaseKey( _domain.utf16()[i] );
		}
		sleep( 200 );
	}
#endif

	pressAndReleaseKey( XK_Return );
}

#endif



void logoutUser( void )
{
#ifdef BUILD_WIN32
	ExitWindowsEx( EWX_LOGOFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
#else
	QProcess::startDetached( "/etc/init.d/kdm restart" );
	QProcess::startDetached( "/etc/init.d/gdm restart" );
	QProcess::startDetached( "/etc/init.d/xdm restart" );
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
		qWarning( "invalid role" );
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
	else
	{
		if( _only_path && val.right( 4 ) == "\\key" )
		{
			return( val.left( val.size() - 4 ) );
		}
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




void setKeyPath( QString _path, const ISD::userRoles _role,
							const QString _group )
{
	_path.replace( QString( QDir::separator() ) + QDir::separator(),
							QDir::separator() );

	QSettings settings( QSettings::SystemScope, "iTALC Solutions",
								"iTALC" );
	if( _role <= ISD::RoleNone || _role >= ISD::RoleCount )
	{
		qWarning( "invalid role" );
		return;
	}
	settings.setValue( "keypaths" + _group + "/" +
						userRoleNames[_role], _path );
}


void setPrivateKeyPath( const QString & _path, const ISD::userRoles _role )
{
	setKeyPath( _path, _role, "private" );
}




void setPublicKeyPath( const QString & _path, const ISD::userRoles _role )
{
	setKeyPath( _path, _role, "public" );
}




QString snapshotDir( void )
{
	QSettings settings;
	return( settings.value( "paths/snapshots",
#ifdef BUILD_WIN32
				windowsConfigPath( CSIDL_PERSONAL ) +
					QDir::separator() +
					QObject::tr( "iTALC-snapshots" )
#else
				personalConfigDir() + "snapshots"
#endif
					).toString() + QDir::separator() );
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
#ifdef BUILD_WIN32
				windowsConfigPath( CSIDL_APPDATA ) +
						QDir::separator() + "iTALC"
#else
				QDir::homePath() + QDir::separator() +
								".italc"
#endif
				+ QDir::separator()
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




QString globalStartmenuDir( void )
{
#ifdef BUILD_WIN32
	return( windowsConfigPath( CSIDL_COMMON_STARTMENU ) +
							QDir::separator() );
#else
	return( "/usr/share/applnk/Applications/" );
#endif
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



void activateWindow( QWidget * _w )
{
	_w->activateWindow();
	_w->raise();
#ifdef BUILD_WIN32
	SetWindowPos( _w->winId(), HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
	SetWindowPos( _w->winId(), HWND_NOTOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
#endif
}


} // end of namespace localSystem

