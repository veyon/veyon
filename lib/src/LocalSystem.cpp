/*
 * LocalSystem.cpp - namespace LocalSystem, providing an interface for
 *                   transparent usage of operating-system-specific functions
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef ITALC_BUILD_WIN32
#define _WIN32_WINNT 0x0501
#endif

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#define _WIN32_WINNT 0x0501
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QDateTime>
#include <QtGui/QWidget>
#include <QtNetwork/QTcpServer>


#ifdef ITALC_BUILD_WIN32

#include <QtCore/QLibrary>

static const char * tr_accels = QT_TRANSLATE_NOOP(
		"QObject",
		"UPL (note for translators: the first three characters of "
		"this string are the accellerators (underlined characters) "
		"of the three input-fields in logon-dialog of windows - "
		"please keep this note as otherwise there are strange errors "
					"concerning logon-feature)" );

#include <windows.h>
#include <shlobj.h>
#include <psapi.h>



namespace LocalSystem
{

// taken from qt-win-opensource-src-4.2.2/src/corelib/io/qsettings.cpp
QString windowsConfigPath( int _type )
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

}

#endif


#ifdef ITALC_HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef ITALC_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef ITALC_HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef ITALC_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef ITALC_HAVE_ERRNO_H
#include <errno.h>
#endif

#include "LocalSystem.h"
#include "minilzo.h"



static LocalSystem::p_pressKey __pressKey;
static QString __log_file;
static QFile * __debug_out = NULL;



static QString properLineEnding( QString _out )
{
	if( _out.right( 1 ) != "\012" )
	{
		_out += "\012";
	}
#ifdef ITALC_BUILD_WIN32
	if( _out.right( 1 ) != "\015" )
	{
		_out.replace( QString( "\012" ), QString( "\015\012" ) );
	}
#elif ITALC_BUILD_LINUX
	if( _out.right( 1 ) != "\015" ) // MAC
	{
		_out.replace( QString( "\012" ), QString( "\015" ) );
	}
#endif
	return( _out );
}



void msgHandler( QtMsgType _type, const char * _msg )
{
	if( LocalSystem::logLevel == 0 )
	{
		return ;
	}
#ifdef ITALC_BUILD_WIN32
	if( QString( _msg ).contains( "timers cannot be stopped",
							Qt::CaseInsensitive ) )
	{
		exit( 0 );
	}
#endif
	if( __debug_out == NULL )
	{
		QString tmp_path = QDir::rootPath() +
#ifdef ITALC_BUILD_WIN32
						"temp"
#else
						"tmp"
#endif
				;
		foreach( const QString s, QProcess::systemEnvironment() )
		{
			if( s.toLower().left( 5 ) == "temp=" )
			{
				tmp_path = s.toLower().mid( 5 );
				break;
			}
			else if( s.toLower().left( 4 ) == "tmp=" )
			{
				tmp_path = s.toLower().mid( 4 );
				break;
			}
		}
		if( !QDir( tmp_path ).exists() )
		{
			if( QDir( QDir::rootPath() ).mkdir( tmp_path ) )
			{
				QFile::setPermissions( tmp_path,
						QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
						QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
						QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
						QFile::ReadOther | QFile::WriteOther | QFile::ExeOther );
			}
		}
		const QString log_path = tmp_path + QDir::separator();
		__debug_out = new QFile( log_path + __log_file );
		__debug_out->open( QFile::WriteOnly | QFile::Append |
							QFile::Unbuffered );
	}

	QString out;
	switch( _type )
	{
		case QtDebugMsg:
			if( LocalSystem::logLevel > 8)
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [debug] %1" ).arg( _msg ) + "\n";
			}
			break;
		case QtWarningMsg:
			if( LocalSystem::logLevel > 5 )
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [warning] %1" ).arg( _msg ) + "\n";
			}
			break;
		case QtCriticalMsg:
			if( LocalSystem::logLevel > 3 )
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [critical] %1" ).arg( _msg ) + "\n";
			}
			break;
		case QtFatalMsg:
			if( LocalSystem::logLevel > 1 )
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [fatal] %1" ).arg( _msg ) + "\n";
			}
		default:
			out = QDateTime::currentDateTime().toString() + QString( ": [unknown] %1" ).arg( _msg ) + "\n";
			break;
	}
	if( out.trimmed().size() )
	{
		out = properLineEnding( out );
		__debug_out->write( out.toUtf8() );
		printf( "%s", out.toUtf8().constData() );
	}
}


void initResources( void )
{
	Q_INIT_RESOURCE(italc_core);
}

namespace LocalSystem
{

int logLevel = 6;


void initialize( p_pressKey _pk, const QString & _log_file )
{
	__pressKey = _pk;
	__log_file = _log_file;

	lzo_init();

	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "italcsolutions.org" );
	QCoreApplication::setApplicationName( "iTALC" );

	QSettings settings( QSettings::SystemScope, "iTALC Solutions", "iTALC" );

	if( settings.contains( "settings/LogLevel" ) )
	{
		logLevel = settings.value( "settings/LogLevel" ).toInt();
	}

	qInstallMsgHandler( msgHandler );

	initResources();
}




int freePort( int _default_port )
{
	QTcpServer t;
	if( t.listen( QHostAddress::LocalHost, _default_port ) )
	{
		return( _default_port );
	}
	t.listen( QHostAddress::LocalHost );
	return( t.serverPort() );
}




void sleep( const int _ms )
{
#ifdef ITALC_BUILD_WIN32
	Sleep( static_cast<unsigned int>( _ms ) );
#else
	struct timespec ts = { _ms / 1000, ( _ms % 1000 ) * 1000 * 1000 } ;
	nanosleep( &ts, NULL );
#endif
}




void execInTerminal( const QString & _cmds )
{
	QProcess::startDetached(
#ifdef ITALC_BUILD_WIN32
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

#ifdef ITALC_BUILD_WIN32
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
#ifdef ITALC_BUILD_WIN32
	closesocket( sock );
	WSACleanup();
#else
	close( sock );
#endif


#if 0
#ifdef ITALC_BUILD_LINUX
	QProcess::startDetached( "etherwake " + _mac );
#endif
#endif
}



static inline void pressAndReleaseKey( int _key )
{
	__pressKey( _key, TRUE );
	__pressKey( _key, FALSE );
}


void logonUser( const QString & _uname, const QString & _passwd,
						const QString & _domain )
{
#ifdef ITALC_BUILD_WIN32

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
/*
	// does not work :(
	enablePrivilege( SE_TCB_NAME, TRUE );
	HANDLE hToken;
	if( !LogonUser(
		(CHAR *)_uname.toAscii().constData(),
		(CHAR*)(_domain.isEmpty() ? "." : _domain.toAscii().constData()),
		(CHAR *)_passwd.toAscii().constData(),
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_DEFAULT,
		&hToken ) )
	{
		CloseHandle( hToken );
	}
	enablePrivilege( SE_TCB_NAME, FALSE );*/

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

	// send Secure Attention Sequence (SAS) for making sure we can enter
	// username and password
	__pressKey( XK_Alt_L, TRUE );
	__pressKey( XK_Control_L, TRUE );
	pressAndReleaseKey( XK_Delete );
	__pressKey( XK_Control_L, FALSE );
	__pressKey( XK_Alt_L, FALSE );

	const ushort * accels = QObject::tr( tr_accels ).utf16();

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
	__pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[0] );
	__pressKey( XK_Alt_L, FALSE );

	pressAndReleaseKey( XK_space );

	__pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[0] );
	__pressKey( XK_Alt_L, FALSE );

	pressAndReleaseKey( XK_BackSpace );
#endif

	for( int i = 0; i < _uname.size(); ++i )
	{
		pressAndReleaseKey( _uname.utf16()[i] );
	}

#ifdef ITALC_BUILD_WIN32
	__pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[1] );
	__pressKey( XK_Alt_L, FALSE );
#else
	pressAndReleaseKey( XK_Tab );
#endif

	for( int i = 0; i < _passwd.size(); ++i )
	{
		pressAndReleaseKey( _passwd.utf16()[i] );
	}

#ifdef ITALC_BUILD_WIN32
	if( !_domain.isEmpty() )
	{
		__pressKey( XK_Alt_L, TRUE );
		pressAndReleaseKey( accels[2] );
		__pressKey( XK_Alt_L, FALSE );
		for( int i = 0; i < _domain.size(); ++i )
		{
			pressAndReleaseKey( _domain.utf16()[i] );
		}
	}
#endif

	pressAndReleaseKey( XK_Return );
}




static const QString userRoleNames[] =
{
	"none",
	"teacher",
	"admin",
	"supporter",
	"other"
} ;


QString userRoleName( const ItalcCore::UserRoles _role )
{
	return( userRoleNames[_role] );
}


inline QString keyPath( const ItalcCore::UserRoles _role, const QString _group,
							bool _only_path )
{
	QSettings settings( QSettings::SystemScope, "iTALC Solutions",
								"iTALC" );
	if( _role <= ItalcCore::RoleNone || _role >= ItalcCore::RoleCount )
	{
		qWarning( "invalid role" );
		return( "" );
	}
	const QString fallback_dir =
#ifdef ITALC_BUILD_LINUX
		"/etc/italc/keys/"
#else
#ifdef ITALC_BUILD_WIN32
		"c:\\italc\\keys\\"
#endif
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


QString privateKeyPath( const ItalcCore::UserRoles _role, bool _only_path )
{
	return( keyPath( _role, "private", _only_path ) );
}


QString publicKeyPath( const ItalcCore::UserRoles _role, bool _only_path )
{
	return( keyPath( _role, "public", _only_path ) );
}




void setKeyPath( QString _path, const ItalcCore::UserRoles _role,
							const QString _group )
{
	_path = _path.left( 1 ) + _path.mid( 1 ).
		replace( QString( QDir::separator() ) + QDir::separator(),
							QDir::separator() );

	QSettings settings( QSettings::SystemScope, "iTALC Solutions",
								"iTALC" );
	if( _role <= ItalcCore::RoleNone || _role >= ItalcCore::RoleCount )
	{
		qWarning( "invalid role" );
		return;
	}
	settings.setValue( "keypaths" + _group + "/" +
						userRoleNames[_role], _path );
}


void setPrivateKeyPath( const QString & _path, const ItalcCore::UserRoles _role )
{
	setKeyPath( _path, _role, "private" );
}




void setPublicKeyPath( const QString & _path, const ItalcCore::UserRoles _role )
{
	setKeyPath( _path, _role, "public" );
}




QString snapshotDir( void )
{
	QSettings settings;
	return( settings.value( "paths/snapshots",
#ifdef ITALC_BUILD_WIN32
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
	const QString d = settings.value( "paths/personalconfigdir" ).toString();
	return( d.isEmpty() ?
#ifdef ITALC_BUILD_WIN32
				windowsConfigPath( CSIDL_APPDATA ) +
						QDir::separator() + "iTALC"
#else
				QDir::homePath() + QDir::separator() +
								".italc"
#endif
				+ QDir::separator()
		:
				d + QDir::separator() );
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
#ifdef ITALC_BUILD_WIN32
	return( windowsConfigPath( CSIDL_COMMON_STARTMENU ) +
							QDir::separator() );
#else
	return( "/usr/share/applnk/Applications/" );
#endif
}




QString parameter( const QString & _name )
{
	return( QSettings().value( "parameters/" + _name ).toString() );
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



#ifdef ITALC_BUILD_WIN32
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
#ifdef ITALC_BUILD_WIN32
	SetWindowPos( _w->winId(), HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
	SetWindowPos( _w->winId(), HWND_NOTOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
#endif
}


} // end of namespace LocalSystem

