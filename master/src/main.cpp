/*
 * main.cpp - startup routine for Veyon Master Application
 *
 * Copyright (c) 2004-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QApplication>
#include <QSplashScreen>

#include "MasterCore.h"
#include "MainWindow.h"
#include "VeyonConfiguration.h"
#include "VeyonCoreConnection.h"
#include "LocalSystem.h"
#include "Logger.h"


QSplashScreen * splashScreen = nullptr;

// good old main-function... initializes qt-app and starts Veyon
int main( int argc, char * * argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );
	app.connect( &app, SIGNAL( lastWindowClosed() ), SLOT( quit() ) );

	VeyonCore core( &app, "Master" );

	// parse arguments
	QStringListIterator arg_it( QCoreApplication::arguments() );
	arg_it.next();

	while( argc > 1 && arg_it.hasNext() )
	{
		const QString & a = arg_it.next();
		if( a == "-role" )
		{
			if( arg_it.hasNext() )
			{
				const QString role = arg_it.next();
				if( role == "teacher" )
				{
					core.setUserRole( VeyonCore::RoleTeacher );
				}
				else if( role == "admin" )
				{
					core.setUserRole( VeyonCore::RoleAdmin );
				}
				else if( role == "supporter" )
				{
					core.setUserRole( VeyonCore::RoleSupporter );
				}
			}
			else
			{
				printf( "-role needs an argument:\n"
					"	teacher\n"
					"	admin\n"
					"	supporter\n\n" );
				return -1;
			}
		}
	}

	QSplashScreen splashScreen( QPixmap( ":/resources/splash.png" ) );
	if( VeyonCore::config().applicationName().isEmpty() )
	{
		splashScreen.show();
	}

	if( !MainWindow::initAuthentication() )
	{
		return -1;
	}

	MasterCore masterCore;

	// now create the main-window
	MainWindow mainWindow( masterCore );

#if 0
	if( !mainWindow.localICA() ||
		!mainWindow.localICA()->isConnected() )
	{
		qCritical( "No connection to local ICA - terminating now" );
		if( VeyonCore::config().logLevel() < Logger::LogLevelDebug )
		{
			return -1;
		}
	}
#endif

	// hide splash-screen as soon as main-window is shown
	splashScreen.finish( &mainWindow );

	mainWindow.show();

	ilog( Info, "Exec" );

	// let's rock!!
	return app.exec();
}

