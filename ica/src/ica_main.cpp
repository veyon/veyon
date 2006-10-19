/*
 * ica_main.cpp - main-file for ICA (iTALC Client Application)
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

#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtNetwork/QHostInfo>

#include "ica_main.h"
#include "service.h"
#include "remote_control_widget.h"
#include "isd_server.h"
#include "ivs.h"
#include "local_system.h"
#include "debug.h"
#include "system_key_trapper.h"

#ifdef SYSTEMTRAY_SUPPORT
#include <QtGui/QSystemTrayIcon>
#endif


int __isd_port = PortOffsetISD;
#ifdef SYSTEMTRAY_SUPPORT
QSystemTrayIcon * __systray_icon = NULL;
#endif


int ICAMain( int argc, char * * argv )
{
#ifdef DEBUG
#ifdef BUILD_LINUX
	extern int _Xdebug;
	_Xdebug = 1;
#endif
#endif
	QCoreApplication * app = NULL;
	if( argc > 1 && QString( argv[1] ) == "-rx11vs" )
	{
		app = new QCoreApplication( argc, argv );
	}
	else if( argc > 1 && QString( argv[1] ) == SERVICE_ARG )
	{
		if( icaServiceMain() )
		{
			return( 0 );
		}
	}
	else
	{
		QApplication * a = new QApplication( argc, argv );
		a->setQuitOnLastWindowClosed( FALSE );
		app = a;
	}

	localSystem::initialize();

	QTranslator app_tr;
	app_tr.load( ":/resources/" + QLocale::system().name().left( 2 ) +
									".qm" );
	app->installTranslator( &app_tr );

	int ivs_port = PortOffsetIVS;
#ifdef BUILD_LINUX
	bool rx11vs = FALSE;
#endif

	QStringListIterator arg_it( QApplication::arguments() );
	arg_it.next();
	while( arg_it.hasNext() )
	{
		const QString & a = arg_it.next();
		if( a == "-isdport" && arg_it.hasNext() )
		{
			__isd_port = arg_it.next().toInt();
		}
		else if( ( a == "-ivsport" || a == "-rfbport" ) &&
							arg_it.hasNext() )
		{
			ivs_port = arg_it.next().toInt();
		}
#ifdef BUILD_LINUX
		else if( a == "-rx11vs" )
		{
			rx11vs = TRUE;
		}
#endif
		else if( a == "-rctrl" && arg_it.hasNext() )
		{
			const QString host = arg_it.next();
			bool view_only = arg_it.hasNext() ?
						arg_it.next().toInt()
					:
						FALSE;
			new remoteControlWidget( host, view_only );
			app->connect( app, SIGNAL( lastWindowClosed() ),
							SLOT( quit() ) );
			return( app->exec() );
		}
		else if( a == "-registerservice" )
		{
			icaServiceInstall( 0 );
			return( 0 );
		}
		else if( a == "-unregisterservice" )
		{
			icaServiceRemove( 0 );
			return( 0 );
		}
		else if( a == "-stopservice" )
		{
			icaServiceStop( 0 );
			return( 0 );
		}
		else if( a == SERVICE_ARG )
		{
			if( icaServiceMain() )
			{
				return( 0 );
			}
		}
#ifdef BUILD_LINUX
		else if( a == "-nosel" || a == "-nosetclipboard" )
		{
		}
#endif
		else
		{
			printf( "Unrecognized commandline-argument %s\n",
						a.toAscii().constData() );
			return( -1 );
		}
	}
	
#ifdef BUILD_LINUX
	if( rx11vs )
	{
#if 1
		IVS( ivs_port, argc, argv, TRUE );
		return( 0 );
#else
		IVS * i = new IVS( ivs_port, argc, argv );
		i->start( IVS::HighestPriority );
		return( app->exec() );
#endif
	}
#endif

#ifdef SYSTEMTRAY_SUPPORT
	QIcon icon( ":/resources/icon16.png" );
	icon.addFile( ":/resources/icon22.png" );
	icon.addFile( ":/resources/icon32.png" );

	__systray_icon = new QSystemTrayIcon( icon );
	__systray_icon->setToolTip(
				QApplication::tr( "iTALC Client on %1:%2" ).
					arg( QHostInfo::localHostName() ).
					arg( QString::number( ivs_port ) ) );
	__systray_icon->show();
#endif
						
	new isdServer( ivs_port, argc, argv );

	return( app->exec() );
}



// platform-specific startup-code follows

#ifdef BUILD_WIN32

#include <windows.h>


extern HINSTANCE	hAppInstance;
extern DWORD		mainthreadId;


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
						PSTR szCmdLine, int iCmdShow )
{
	// save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();

	const int pathlen = 2048;
	char path[pathlen];
	if( GetModuleFileName( NULL, path, pathlen ) == 0 )
	{
		printf( "could not determine module-filename!\n" ); 
	}

	QStringList cmdline = QString( szCmdLine ).toLower().split( ' ' );
	cmdline.push_front( path );

	char * * argv = new char *[cmdline.size()];
	int argc = 0;
	for( QStringList::iterator it = cmdline.begin(); it != cmdline.end();
								++it, ++argc )
	{
		argv[argc] = new char[it->length() + 1];
		strcpy( argv[argc], it->toAscii().constData() );
	}

#if 0
	// try to hide our process
	HINSTANCE nt_query_system_info = LoadLibrary( "HookNTQSI.dll" );
	if( nt_query_system_info )
	{
		int (_cdecl *pfnHook)(DWORD);
		pfnHook = (int(*)(DWORD))GetProcAddress( nt_query_system_info, "Hook" );
		pfnHook( GetCurrentProcessId() );
	}
#endif

	return( ICAMain( argc, argv ) );
}


#else


int main( int argc, char * * argv )
{
	return( ICAMain( argc, argv ) );
}


#endif


