/*
 * IcaMain.cpp - main-file for ICA (iTALC Client Application)
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtCore/QLocale>
#include <QtCore/QTime>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtNetwork/QHostInfo>

#include "IcaMain.h"
#include "SystemService.h"
#include "ItalcCoreServer.h"
#include "ItalcVncServer.h"
#include "LocalSystemIca.h"
#include "Debug.h"
#include "DsaKey.h"

#include "Ipc/Slave.h"

#include "DemoClientSlave.h"
#include "MessageBoxSlave.h"
#include "ScreenLockSlave.h"
#include "SystemTrayIconSlave.h"


QString __app_name = "iTALC Client";
const QString SERVICE_ARG = "-service";


int serviceMain( SystemService * _srv )
{
	int c = 1;
	char * * v = new char *[1];
	v[0] = _srv->argv()[0];
	return ICAMain( c, v );
}


#ifdef ITALC_BUILD_WIN32

// event-filter which makes ICA ignore quit- end end-session-messages for not
// quitting at user logoff
bool eventFilter( void * _msg, long * _result )
{
	DWORD msg = ( ( MSG *) _msg )->message;
	if(/* msg == WM_QUIT ||*/msg == WM_ENDSESSION )
	{
		return true;
	}
	return false;
}

#endif



int createKeyPair( int argc, char **argv )
{
	const QString roleArg = argv[2];
	ItalcCore::UserRoles role = ItalcCore::RoleTeacher;
	if( roleArg == "admin" )
	{
		role = ItalcCore::RoleAdmin;
	}
	else if( roleArg == "supporter" )
	{
		role = ItalcCore::RoleSupporter;
	}
	else if( roleArg == "other" )
	{
		role = ItalcCore::RoleOther;
	}
	bool customPath = argc > 3;
	QString priv = customPath ? argv[3] :
					LocalSystem::privateKeyPath( role );
	QString pub = customPath ?
					( argc > 4 ? argv[4] : priv + ".pub" )
				:
					LocalSystem::publicKeyPath( role );
	printf( "\n\ncreating new key-pair ... \n" );
	PrivateDSAKey pkey( 1024 );
	if( !pkey.isValid() )
	{
		qCritical( "key generation failed!" );
		return -1;
	}
	pkey.save( priv );
	PublicDSAKey( pkey ).save( pub );
	printf( "...done, saved key-pair in\n\n%s\n\nand\n\n%s",
						priv.toUtf8().constData(),
						pub.toUtf8().constData() );
	printf( "\n\n\nFor now the file is only readable by "
				"root and members of group root (if you\n"
				"didn't ran this command as non-root).\n"
				"I suggest changing the ownership of the "
				"private key so that the file is\nreadable "
				"by all members of a special group to which "
				"all users belong who are\nallowed to use "
				"iTALC.\n\n\n" );
	return 0;
}



static bool initCoreApplication( QCoreApplication *app = NULL )
{
	const QString loc = QLocale::system().name().left( 2 );

	foreach( const QString & qm, QStringList()
												<< loc + "-core"
												<< loc
												<< "qt_" + loc )
	{
		QTranslator * tr = new QTranslator( app );
		tr->load( QString( ":/resources/%1.qm" ).arg( qm ) );
		app->installTranslator( tr );
	}

	LocalSystem::initialize();

	if( LocalSystem::parameter( "serverport" ).toInt() > 0 )
	{
		ItalcCore::serverPort = LocalSystem::parameter( "serverport" ).toInt();
	}

	QStringListIterator argIt( QCoreApplication::arguments() );
	argIt.next();	// skip application file name
	while( argIt.hasNext() )
	{
		const QString & a = argIt.next();
		if( a == "-port" && argIt.hasNext() )
		{
			ItalcCore::serverPort = argIt.next().toInt();
		}
		else if( a == "-role" )
		{
			if( argIt.hasNext() )
			{
				const QString role = argIt.next();
				if( role == "teacher" )
				{
					ItalcCore::role = ItalcCore::RoleTeacher;
				}
				else if( role == "admin" )
				{
					ItalcCore::role = ItalcCore::RoleAdmin;
				}
				else if( role == "supporter" )
				{
					ItalcCore::role = ItalcCore::RoleSupporter;
				}
			}
			else
			{
				printf( "-role needs an argument:\n"
					"	teacher\n"
					"	admin\n"
					"	supporter\n\n" );
				return false;
			}
		}
#ifdef ITALC_BUILD_LINUX
		// accept these options for x11vnc
		else if( a == "-nosel" || a == "-nosetclipboard" ||
				a == "-noshm" || a == "-solid" ||
				a == "-xrandr" || a == "-onetile" )
		{
		}
		else if( a == "-h" || a == "--help" )
		{
			QProcess::execute( "man ica" );
			return false;
		}
#endif
		else if( a == "-v" || a == "--version" )
		{
			printf( "%s\n", ITALC_VERSION );
			return false;
		}
		else if( a == "-slave" )
		{
			return true;
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
	initCoreApplication( &app );

#ifdef ITALC_BUILD_WIN32
	app.setEventFilter( eventFilter );
#endif

	ItalcCoreServer coreServer;
	ItalcVncServer vncServer;

	// start the SystemTrayIconSlave and set the default tooltip
	coreServer.masterProcess()->setSystemTrayToolTip(
		QApplication::tr( "iTALC Client %1 on %2:%3" ).
							arg( ITALC_VERSION ).
							arg( QHostInfo::localHostName() ).
							arg( QString::number( vncServer.serverPort() ) ) );

//	vncServer.run();
	vncServer.start();

	return app.exec();
}



template<class SlaveClass>
static int runSlave( int argc, char **argv )
{
	QApplication app( argc, argv );
	initCoreApplication( &app );

	SlaveClass s;

	return app.exec();
}



int ICAMain( int argc, char **argv )
{
	qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

	// decide whether to create a QCoreApplication or QApplication
	if( argc >= 2 )
	{
		const QString arg1 = argv[1];
		if( arg1 == "-service" )
		{
			SystemService s( "icas", SERVICE_ARG, __app_name,
								"", serviceMain, argc, argv );
			if( s.evalArgs( argc, argv ) )
			{
				return 0;
			}
		}
		else if( arg1 == "-createkeypair" )
		{
			return createKeyPair( argc, argv );
		}
		else if( arg1 == "-slave" )
		{
			if( argc <= 2 )
			{
				qCritical( "Need to specify slave" );
				return -1;
			}
			const QString arg2 = argv[2];
			if( arg2 == MasterProcess::IdCoreServer )
			{
				return runCoreServer( argc, argv );
			}
			else if( arg2 == MasterProcess::IdDemoClient )
			{
				return runSlave<DemoClientSlave>( argc, argv );
			}
			else if( arg2 == MasterProcess::IdMessageBox )
			{
				return runSlave<MessageBoxSlave>( argc, argv );
			}
			else if( arg2 == MasterProcess::IdScreenLock )
			{
				return runSlave<ScreenLockSlave>( argc, argv );
			}
			else if( arg2 == MasterProcess::IdSystemTrayIcon )
			{
				return runSlave<SystemTrayIconSlave>( argc, argv );
			}
			else
			{
				qCritical( "Unknown slave" );
				return -1;
			}
		}
	}

	return runCoreServer( argc, argv );
}



// platform-specific startup-code follows

#ifdef ITALC_BUILD_WIN32

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

	char **argv = new char *[cmdline.size()];
	int argc = 0;
	for( QStringList::iterator it = cmdline.begin(); it != cmdline.end();
								++it, ++argc )
	{
		argv[argc] = new char[it->length() + 1];
		strcpy( argv[argc], it->toUtf8().constData() );
	}

	return ICAMain( argc, argv );
}


#else


int main( int argc, char **argv )
{
#ifdef DEBUG
	extern int _Xdebug;
//	_Xdebug = 1;
#endif

	return ICAMain( argc, argv );
}


#endif


