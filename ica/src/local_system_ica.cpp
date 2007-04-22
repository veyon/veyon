/*
 * local_system_ica.cpp - namespace localSystem, providing an interface for
 *                        transparent usage of operating-system-specific
 *                        functions
 *
 * Copyright (c) 2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "local_system.h"


#ifdef BUILD_WIN32

#include <QtCore/QThread>

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <shlobj.h>
#include <psapi.h>
//#include <winable.h>


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
	localSystem::sleep( 50 );
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




namespace localSystem
{


void initialize( void )
{
	initialize( pressKey, "italc_client.log" );
#ifdef BUILD_WIN32
	__user_poll_thread = new userPollThread();

	SetConsoleCtrlHandler( consoleCtrlHandler, TRUE );
#endif

}


} // end of namespace localSystem

