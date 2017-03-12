/*
 * main.cpp - main file for iTALC Service Application
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QProcess>
#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QtNetwork/QHostInfo>

#include "WindowsService.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "ComputerControlServer.h"
#include "VncServer.h"
#include "Logger.h"


#ifdef ITALC_BUILD_WIN32
static HANDLE hShutdownEvent = NULL;

// event filter which makes ICA recognize logoff events etc.
class LogoffEventFilter : public QAbstractNativeEventFilter
{
public:
	virtual bool nativeEventFilter( const QByteArray& eventType, void *message, long *result)
	{
		Q_UNUSED(eventType);
		Q_UNUSED(result);

		DWORD winMsg = ( ( MSG *) message )->message;

		if( winMsg == WM_QUERYENDSESSION )
		{
			ilog( Info, "Got WM_QUERYENDSESSION - initiating server shutdown" );

			// tell UltraVNC server to quit
			SetEvent( hShutdownEvent );
		}

		return false;
	}

};

#endif



static bool parseArguments( const QStringList &arguments )
{
	QStringListIterator argIt( arguments );
	argIt.next();	// skip application file name
	while( argIt.hasNext() )
	{
		const QString & a = argIt.next().toLower();
#ifdef ITALC_BUILD_LINUX
		// accept these options for x11vnc
		if( a == "-nosel" || a == "-nosetclipboard" ||
				a == "-noshm" || a == "-solid" ||
				a == "-xrandr" || a == "-onetile" )
		{
		}
		else if( a == "-h" || a == "--help" )
		{
			QProcess::execute( "man italc-service" );
			return false;
		}
#endif
		if( a == "-v" || a == "--version" )
		{
			printf( "%s\n", ITALC_VERSION );
			return false;
		}
		else
		{
			qWarning() << "Unrecognized commandline argument" << a;
			return false;
		}
	}

	return true;
}




static int runCoreServer( int argc, char **argv )
{
	QCoreApplication app( argc, argv );

	ItalcCore core( &app, "Service" );

	if( !parseArguments( app.arguments() ) )
	{
		return -1;
	}

#ifdef ITALC_BUILD_WIN32
	hShutdownEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE,
								"Global\\SessionEventUltra" );
	if( !hShutdownEvent )
	{
		// no global event available already as we're not running under the
		// control of the ICA service supervisor?
		if( GetLastError() == ERROR_FILE_NOT_FOUND )
		{
			qWarning( "Creating session event" );
			// then create our own event as otherwise the VNC server main loop
			// will eat 100% CPU due to failing WaitForSingleObject() calls
			hShutdownEvent = CreateEvent( NULL, FALSE, FALSE,
											"Global\\SessionEventUltra" );
		}
		else
		{
			qCritical( "Could not open or create session event" );
			return -1;
		}
	}

	LogoffEventFilter eventFilter;

	app.installNativeEventFilter( &eventFilter );
#endif

	ComputerControlServer coreServer;
	coreServer.start();

	ilog( Info, "Exec" );
	int ret = app.exec();

	ilog( Info, "Exec Done" );

#ifdef ITALC_BUILD_WIN32
	CloseHandle( hShutdownEvent );
#endif

	return ret;
}

#ifdef ITALC_BUILD_WIN32
#include <windows.h>

extern HINSTANCE	hAppInstance;
extern DWORD		mainthreadId;
#endif

int main( int argc, char **argv )
{
#ifdef DEBUG
	extern int _Xdebug;
//	_Xdebug = 1;
#endif

#ifdef ITALC_BUILD_WIN32
	// initialize global instance handler and main thread ID
	hAppInstance = GetModuleHandle( NULL );
	mainthreadId = GetCurrentThreadId();
#endif

	// decide in what mode to run
	if( argc >= 2 )
	{
		const QString arg1 = argv[1];
#ifdef ITALC_BUILD_WIN32
		for( int i = 1; i < argc; ++i )
		{
			if( QString( argv[i] ).toLower().contains( "service" ) )
			{
				WindowsService winService( "ItalcService", "-service", "iTALC Service",
											QString(), argc, argv );
				if( winService.evalArgs( argc, argv ) )
				{
					return 0;
				}
				break;
			}
		}
#endif
	}

	return runCoreServer( argc, argv );
}


