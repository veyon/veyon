/*
 * ica_main.cpp - main-file for ICA (iTALC Client Application)
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

#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtNetwork/QHostInfo>


#include "ica_main.h"
#include "system_service.h"
#include "remote_control_widget.h"
#include "isd_server.h"
#include "ivs.h"
#include "local_system.h"
#include "debug.h"
#include "system_key_trapper.h"
#include "dsa_key.h"
#include "messagebox.h"

#ifdef SYSTEMTRAY_SUPPORT
#include <QtGui/QSystemTrayIcon>
#endif


int __isd_port = PortOffsetISD;
int __ivs_port = PortOffsetIVS;

#ifdef BUILD_LINUX
bool __rx11vs = FALSE;
#endif

QString __app_name = "iTALC Client";
const QString SERVICE_ARG = "-service";

#include <QtCore/QProcess>

int serviceMain( systemService * _srv )
{
	int c = 1;
	char * * v = new char *[1];
	v[0] = _srv->argv()[0];
	return( ICAMain( c, v ) );
}


#ifdef BUILD_WIN32

// event-filter which makes ICA ignore quit- end end-session-messages for not
// quitting at user logoff
bool eventFilter( void * _msg, long * _result )
{
	DWORD msg = ( ( MSG *) _msg )->message;
	if(/* msg == WM_QUIT ||*/msg == WM_ENDSESSION )
	{
		return( TRUE );
	}
	return( FALSE );
}

#endif


int ICAMain( int argc, char * * argv )
{
#ifdef DEBUG
#ifdef BUILD_LINUX
	extern int _Xdebug;
	_Xdebug = 1;
#endif
#endif

	// decide whether to create a QCoreApplication or QApplication
	bool core_app = FALSE;
	for( int i = 1; i < argc; ++i )
	{
		if( QString( argv[i] ) == "-rx11vs" ||
			QString( argv[i] ) == "-createkeypair" ||
			QString( argv[i] ) == "-h" )
		{
			core_app = TRUE;
		}
	}

	if( !core_app )
	{
		systemService s( "icas", SERVICE_ARG, __app_name,
#ifndef NO_LUPUS_INTEGRATION
				"lupus",
#else
				"",
#endif
						serviceMain, argc, argv );
		if( s.evalArgs( argc, argv ) || argc == 0 )
		{
			return( 0 );
		}
	}

	QCoreApplication * app = NULL;

	if( core_app )
	{
		app = new QCoreApplication( argc, argv );
	}
	else
	{
		QApplication * a = new QApplication( argc, argv );
		a->setQuitOnLastWindowClosed( FALSE );
		app = a;
	}

#ifdef BUILD_WIN32
	app->setEventFilter( eventFilter );
#endif

	const QString loc = QLocale::system().name().left( 2 );
	QTranslator app_tr;
	app_tr.load( ":/resources/" + loc + ".qm" );
	app->installTranslator( &app_tr );

	QTranslator qt_tr;
	qt_tr.load( ":/resources/qt_" + loc + ".qm" );
	app->installTranslator( &qt_tr );

	localSystem::initialize();

	QStringListIterator arg_it( QCoreApplication::arguments() );
	arg_it.next();
	while( argc > 1 && arg_it.hasNext() )
	{
		const QString & a = arg_it.next();
		if( a == "-isdport" && arg_it.hasNext() )
		{
			__isd_port = arg_it.next().toInt();
		}
		else if( ( a == "-ivsport" || a == "-rfbport" ) &&
							arg_it.hasNext() )
		{
			__ivs_port = arg_it.next().toInt();
		}
#ifdef BUILD_LINUX
		else if( a == "-rx11vs" )
		{
			__rx11vs = TRUE;
		}
		else if( a == ACCESS_DIALOG_ARG && arg_it.hasNext() )
		{
			return( isdServer::showAccessDialog( arg_it.next() ) );
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
		else if( a == "-role" )
		{
			if( arg_it.hasNext() )
			{
				const QString role = arg_it.next();
				if( role == "teacher" )
				{
					__role = ISD::RoleTeacher;
				}
				else if( role == "admin" )
				{
					__role = ISD::RoleAdmin;
				}
				else if( role == "supporter" )
				{
					__role = ISD::RoleSupporter;
				}
			}
			else
			{
				printf( "-role needs an argument:\n"
					"	teacher\n"
					"	admin\n"
					"	supporter\n\n" );
				return( -1 );
			}
		}
		else if( a == "-createkeypair" )
		{
			ISD::userRoles role = ( __role != ISD::RoleOther ) ?
						__role : ISD::RoleTeacher;
			bool user_path = arg_it.hasNext();
			QString priv = user_path ? arg_it.next() :
					localSystem::privateKeyPath( role );
			QString pub = user_path ?
					( arg_it.hasNext() ? 
						arg_it.next() : priv + ".pub" )
				:
					localSystem::publicKeyPath( role );
			printf( "\n\ncreating new key-pair ... \n" );
			privateDSAKey pkey( 1024 );
			if( !pkey.isValid() )
			{
				printf( "key generation failed!\n" );
				return( -1 );
			}
			pkey.save( priv );
			publicDSAKey( pkey ).save( pub );
			printf( "...done, saved key-pair in\n\n%s\n\nand\n\n%s",
						priv.toAscii().constData(),
						pub.toAscii().constData() );
			printf( "\n\n\nFor now the file is only readable by "
				"root and members of group root (if you\n"
				"didn't ran this command as non-root).\n"
				"I suggest changing the ownership of the "
				"private key so that the file is\nreadable "
				"by all members of a special group to which "
				"all users belong who are\nallowed to use "
				"iTALC.\n\n\n" );
			return( 0 );
		}
#ifdef BUILD_LINUX
		else if( a == "-nosel" || a == "-nosetclipboard" ||
				a == "-noshm" || a == "-solid" ||
				a == "-xrandr" || a == "-onetile" )
		{
		}
		else if( a == "-h" || a == "--help" )
		{
			QProcess::execute( "man ica" );
			return( 0 );
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
	if( __rx11vs )
	{
#if 1
		IVS( __ivs_port, argc, argv, TRUE );
		return( 0 );
#else
		IVS * i = new IVS( __ivs_port, argc, argv );
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
					arg( QString::number( __ivs_port ) ) );
	__systray_icon->show();
#endif

	new isdServer( __ivs_port, argc, argv );

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
		qCritical( "WinMain(...): "
				"could not determine module-filename!" ); 
		return -1;
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

	return( ICAMain( argc, argv ) );
}


#else


int main( int argc, char * * argv )
{
	return( ICAMain( argc, argv ) );
}


#endif


