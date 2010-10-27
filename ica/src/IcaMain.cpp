/*
 * IcaMain.cpp - main file for ICA (iTALC Client Application)
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

#include <italcconfig.h>

#include <QtCore/QProcess>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtNetwork/QHostInfo>

#include "WindowsService.h"
#include "ItalcCoreServer.h"
#include "ItalcVncServer.h"
#include "LocalSystemIca.h"
#include "Debug.h"
#include "DsaKey.h"

#include "AccessDialogSlave.h"
#include "DemoClientSlave.h"
#include "DemoServerSlave.h"
#include "MessageBoxSlave.h"
#include "InputLockSlave.h"
#include "ScreenLockSlave.h"
#include "SystemTrayIconSlave.h"


#ifdef ITALC_BUILD_WIN32
static HANDLE hShutdownEvent = NULL;

// event-filter which makes ICA recognize logoff events etc.
bool eventFilter( void *msg, long *result )
{
	DWORD winMsg = ( ( MSG *) msg )->message;

	if( winMsg == WM_QUERYENDSESSION )
	{
		// tell UltraVNC server to quit
		SetEvent( hShutdownEvent );
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



void initCoreApplication( QCoreApplication *app = NULL )
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
}



static bool parseArguments( const QStringList &arguments )
{
	QStringListIterator argIt( arguments );
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
	if( !parseArguments( app.arguments() ) )
	{
		return -1;
	}

#ifdef ITALC_BUILD_WIN32
	hShutdownEvent = CreateEvent( NULL, FALSE, FALSE,
									"Global\\SessionEventUltra" );
	app.setEventFilter( eventFilter );
#endif

	ItalcCoreServer coreServer;
	ItalcVncServer vncServer;

	// start the SystemTrayIconSlave and set the default tooltip
	coreServer.slaveManager()->setSystemTrayToolTip(
		QApplication::tr( "iTALC Client %1 on %2:%3" ).
							arg( ITALC_VERSION ).
							arg( QHostInfo::localHostName() ).
							arg( QString::number( vncServer.serverPort() ) ) );

	// make app terminate once the VNC server thread has finished
	app.connect( &vncServer, SIGNAL( finished() ), SLOT( quit() ) );

	vncServer.start();

	app.exec();

#ifdef ITALC_BUILD_WIN32
	CloseHandle( hShutdownEvent );
#endif

	return 0;
}



template<class SlaveClass, class Application>
static int runSlave( int argc, char **argv )
{
	Application app( argc, argv );

	initCoreApplication( &app );
	if( !parseArguments( app.arguments() ) )
	{
		return -1;
	}

	SlaveClass s;

	return app.exec();
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
		if( arg1.contains( "service" ) )
		{
			WindowsService winService( "icas", "-service", "iTALC Client",
										QString(), argc, argv );
			if( winService.evalArgs( argc, argv ) )
			{
				return 0;
			}
		}
#endif
		if( arg1 == "-createkeypair" )
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
			if( arg2 == ItalcSlaveManager::IdCoreServer )
			{
				return runCoreServer( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdAccessDialog )
			{
				return runSlave<AccessDialogSlave, QApplication>( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdDemoClient )
			{
				return runSlave<DemoClientSlave, QApplication>( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdMessageBox )
			{
				return runSlave<MessageBoxSlave, QApplication>( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdScreenLock )
			{
				return runSlave<ScreenLockSlave, QApplication>( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdInputLock )
			{
				return runSlave<InputLockSlave, QApplication>( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdSystemTrayIcon )
			{
				return runSlave<SystemTrayIconSlave, QApplication>( argc, argv );
			}
			else if( arg2 == ItalcSlaveManager::IdDemoServer )
			{
				return runSlave<DemoServerSlave, QCoreApplication>( argc, argv );
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


