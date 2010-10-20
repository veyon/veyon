/*
 * LocalSystemIca.cpp - namespace LocalSystem, providing an interface for
 *                      transparent usage of operating-system-specific
 *                      functions
 *
 * Copyright (c) 2007-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef ITALC_BUILD_WIN32
#define _WIN32_WINNT 0x0501
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#ifdef ITALC_BUILD_WIN32

#include <windows.h>
#include <psapi.h>
#include <lm.h>
#include <shlobj.h>
#include <winable.h>

#if _WIN32_WINNT >= 0x500
#define SHUTDOWN_FLAGS (EWX_FORCE | EWX_FORCEIFHUNG)
#else
#define SHUTDOWN_FLAGS (EWX_FORCE)
#endif

#if _WIN32_WINNT >= 0x501
#include <reason.h>
#define SHUTDOWN_REASON SHTDN_REASON_MAJOR_OTHER
#else
#define SHUTDOWN_REASON 0
#endif

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include "LocalSystem.h"
#include "SystemKeyTrapper.h"
#include "SystemService.h"

/*

void getUserName( char * * _str )
{
	if( !_str )
	{
		return;
	}
	*_str = NULL;

	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof( aProcesses ), &cbNeeded ) )
	{
		return;
	}

	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	for( DWORD i = 0; i < cProcesses; i++ )
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
							false, aProcesses[i] );
		HMODULE hMod;
		if( hProcess == NULL ||
			!EnumProcessModules( hProcess, &hMod, sizeof( hMod ),
								&cbNeeded ) )
	        {
			continue;
		}

		TCHAR szProcessName[MAX_PATH];
		GetModuleBaseName( hProcess, hMod, szProcessName,
				sizeof( szProcessName ) / sizeof( TCHAR) );
		for( TCHAR * ptr = szProcessName; *ptr; ++ptr )
		{
			*ptr = tolower( *ptr );
		}

		if( strcmp( szProcessName, "explorer.exe" ) )
		{
			CloseHandle( hProcess );
			continue;
		}

		HANDLE hToken;
		OpenProcessToken( hProcess, TOKEN_READ, &hToken );
		DWORD len = 0;

		GetTokenInformation( hToken, TokenUser, NULL, 0, &len ) ;
		if( len <= 0 )
		{
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		char * buf = new char[len];
		if( buf == NULL )
		{
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		if ( !GetTokenInformation( hToken, TokenUser, buf, len, &len ) )
		{
			delete[] buf;
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}

		PSID psid = ((TOKEN_USER*) buf)->User.Sid;

		DWORD accname_len = 0;
		DWORD domname_len = 0;
		SID_NAME_USE nu;
		LookupAccountSid( NULL, psid, NULL, &accname_len, NULL,
							&domname_len, &nu );
		if( accname_len == 0 || domname_len == 0 )
		{
			delete[] buf;
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		char * accname = new char[accname_len];
		char * domname = new char[domname_len];
		if( accname == NULL || domname == NULL )
		{
			delete[] buf;
			delete[] accname;
			delete[] domname;
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		LookupAccountSid( NULL, psid, accname, &accname_len,
						domname, &domname_len, &nu );
		WCHAR wszDomain[256];
		MultiByteToWideChar( CP_ACP, 0, domname,
			strlen( domname ) + 1, wszDomain, sizeof( wszDomain ) /
						sizeof( wszDomain[0] ) );
		WCHAR wszUser[256];
		MultiByteToWideChar( CP_ACP, 0, accname,
			strlen( accname ) + 1, wszUser, sizeof( wszUser ) /
							sizeof( wszUser[0] ) );
		PBYTE domcontroller = NULL;
		if( NetGetDCName( NULL, wszDomain, &domcontroller ) !=
								NERR_Success )
		{
			domcontroller = NULL;
		}
		LPUSER_INFO_2 pBuf = NULL;
		NET_API_STATUS nStatus = NetUserGetInfo( (LPWSTR)domcontroller,
						wszUser, 2, (LPBYTE *) &pBuf );
		if( nStatus == NERR_Success && pBuf != NULL )
		{
			len = WideCharToMultiByte( CP_ACP, 0,
							pBuf->usri2_full_name,
						-1, NULL, 0, NULL, NULL );
			if( len > 0 )
			{
				char * mbstr = new char[len];
				len = WideCharToMultiByte( CP_ACP, 0,
							pBuf->usri2_full_name,
						-1, mbstr, len, NULL, NULL );
				if( strlen( mbstr ) < 1 )
				{
					*_str = new char[2*accname_len+4];
					sprintf( *_str, "%s (%s)", accname,
								accname );
				}
				else
				{
					*_str = new char[len+accname_len+4];
					sprintf( *_str, "%s (%s)", mbstr,
								accname );
				}
				delete[] mbstr;
			}
			else
			{
				*_str = new char[2*accname_len+4];
				sprintf( *_str, "%s (%s)", accname, accname );
			}
		}
		if( pBuf != NULL )
		{
			NetApiBufferFree( pBuf );
		}
		if( domcontroller != NULL )
		{
			NetApiBufferFree( domcontroller );
		}
		delete[] accname;
		delete[] domname;
		FreeSid( psid );
		delete[] buf;
		CloseHandle( hToken );
		CloseHandle( hProcess );
	}
}



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
			char * name = NULL;
			getUserName( &name );
			if( name )
			{
				QMutexLocker m( &m_mutex );
				m_name = name;
				m_name.detach();
				delete[] name;
			}
			Sleep( 5000 );
		} // end while( 1 )

	}

	QString m_name;
	QMutex m_mutex;

}  static * __user_poll_thread = NULL;



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
extern __declspec(dllimport) BOOL __localInputsDisabled;

static inline void pressKey( int _key, bool _down )
{
	if( !__server )
	{
		return;
	}
	vncKeymap::keyEvent( _key, _down, __server );
	LocalSystem::sleep( 50 );
}
*/
//extern __declspec(dllimport) BOOL __localInputsDisabled;
static inline void pressKey( int _key, bool _down )
{
}

#else

#include "LocalSystem.h"

#ifdef ITALC_HAVE_X11
#include <X11/Xlib.h>
#else
#define KeySym int
#endif

#ifdef ITALC_HAVE_PWD_H
#include <pwd.h>
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


extern QString __sessCurUser;

namespace LocalSystem
{


void initialize( void )
{
	initialize( pressKey, "italc_client.log" );
#ifdef ITALC_BUILD_WIN32
//	__user_poll_thread = new userPollThread();

//	SetConsoleCtrlHandler( consoleCtrlHandler, TRUE );
#endif

}





QString currentUser( void )
{
	QString ret = "unknown";

#ifdef ITALC_BUILD_WIN32
	return __sessCurUser;
/*
	if( __user_poll_thread && !__user_poll_thread->name().isEmpty() )
	{
		ret = __user_poll_thread->name();
	}
*/
#else

	char * user_name = getenv( "USER" );
#ifdef ITALC_HAVE_PWD_H
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
		QString shell( pw_entry->pw_shell );

		/* Skip not real users */
		if ( shell.endsWith( "/false" ) ||
				shell.endsWith( "/true" ) ||
				shell.endsWith( "/null" ) ||
				shell.endsWith( "/nologin") )
		{
			return "";
		}

		return QString( "%1 (%2)" ).
				arg( QString::fromUtf8( pw_entry->pw_gecos ) ).
				arg( QString::fromUtf8( pw_entry->pw_name ) );
	}
#endif
	if( user_name )
	{
		return QString::fromUtf8( user_name );
	}

#endif

	return ret;
}




void disableLocalInputs( bool _disabled )
{
#if 0
#ifdef ITALC_BUILD_WIN32
	static systemKeyTrapper * __skt = NULL;
	if( __localInputsDisabled != _disabled )
	{
		__localInputsDisabled = _disabled;
		if( _disabled && __skt == NULL )
		{
			__skt = new systemKeyTrapper();
		}
		else
		{
			delete __skt;
			__skt = NULL;
		}
	}
#else
#warning TODO
#endif
#endif
}



void powerDown( void )
{
#ifdef ITALC_BUILD_WIN32
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
	if( currentUser() == "root (root)" )
	{
		QProcess::startDetached( "poweroff" );
	}
	else
	{
		QProcess::startDetached( "gdm-signal -h" ); // Gnome shutdown
		QProcess::startDetached( "gnome-session-save --silent --kill" ); // Gnome logout
		QProcess::startDetached( "dcop ksmserver ksmserver logout 0 2 0" ); // KDE shutdown
	}
#endif
}




void logoutUser( void )
{
#ifdef ITALC_BUILD_WIN32
	ExitWindowsEx( EWX_LOGOFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
#else
	QProcess::startDetached( "gnome-session-save --silent --kill" ); // Gnome logout
	QProcess::startDetached( "dcop ksmserver ksmserver logout 0 0 0" ); // KDE logout
#endif
}



void reboot( void )
{
#ifdef ITALC_BUILD_WIN32
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
	if( currentUser() == "root (root)" )
	{
		QProcess::startDetached( "poweroff" );
	}
	else
	{
		QProcess::startDetached( "gdm-signal -r" ); // Gnome reboot
		QProcess::startDetached( "gnome-session-save --silent --kill" ); // Gnome logout
		QProcess::startDetached( "dcop ksmserver ksmserver logout 0 1 0" ); // KDE reboot
	}
#endif
}





} // end of namespace LocalSystem

