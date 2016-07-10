/*
 * main.cpp - main-file for iTALC Master Application
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtCore/QModelIndex>
#include <QtGui/QApplication>
#include <QtGui/QSplashScreen>

#ifdef ITALC3
#include "MasterCore.h"
#endif
#include "MainWindow.h"
#include "ItalcConfiguration.h"
#include "ItalcCoreConnection.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "RemoteControlWidget.h"



QSplashScreen * splashScreen = NULL;

#ifndef ITALC3
QString __default_domain;
#endif


// good old main-function... initializes qt-app and starts iTALC
int main( int argc, char * * argv )
{
	QApplication app( argc, argv );

	ItalcCore::init();

	Logger l( "ItalcMaster" );

	app.connect( &app, SIGNAL( lastWindowClosed() ), SLOT( quit() ) );

	app.setStyleSheet(
		"QMenu { border:1px solid black; background-color: white; "
			"background-image:url(:/resources/tray-menu-bg.png); "
			"background-repeat:no-repeat; "
			"background-position: bottom right; }"
		"QMenu::separator { height: 1px; background: rgb(128,128,128); "
					"margin-left: 5px; margin-right: 5px; }"
		"QMenu::item { padding: 2px 32px 2px 20px; "
						"margin:3px; }"
		"QMenu::item:selected { color: white; font-weight:bold; "
			"background-color: rgba(0, 0, 0, 160); "
						"margin:3px; }"
		"QMenu::item:disabled { color: white;  margin:0px; "
			"background-color: rgba(0,0,0,192); font-size:14px;"
			"font-weight:bold; padding: 4px 32px 4px 20px; }" );

	// load translations
	qRegisterMetaType<QModelIndex>( "QModelIndex" );
	qRegisterMetaType<quint16>( "quint16" );


	ItalcCore::role = ItalcCore::RoleTeacher;

	// parse arguments
	QStringListIterator arg_it( QCoreApplication::arguments() );
	arg_it.next();
	int screen = -1;
	while( argc > 1 && arg_it.hasNext() )
	{
		const QString & a = arg_it.next();
		if( a == "-rctrl" && arg_it.hasNext() )
		{
			if( !ItalcCore::initAuthentication() )
			{
				ilog_failed( "ItalcCore::initAuthentication()" );
				return -1;
			}

			const QString host = arg_it.next();
			bool view_only = arg_it.hasNext() ?
						arg_it.next().toInt()
					:
						false;
			new RemoteControlWidget( host, view_only );
			return app.exec();
		}
		else if( a == "-screen" && arg_it.hasNext() )
		{
			screen = arg_it.next().toInt();
		}
		else if( a == "-role" )
		{
			if( arg_it.hasNext() )
			{
				const QString role = arg_it.next();
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
				return -1;
			}
		}
	}


	QSplashScreen splashScreen( QPixmap( ":/resources/splash.png" ) );
	splashScreen.show();

	if( !MainWindow::initAuthentication() )
	{
		return -1;
	}
	// now create the main-window
	MainWindow mainWindow( screen );

	if( !mainWindow.localICA() ||
		!mainWindow.localICA()->isConnected() )
	{
		qCritical( "No connection to local ICA - terminating now" );
		if( ItalcCore::config->logLevel() < Logger::LogLevelDebug )
		{
			return -1;
		}
	}

	// hide splash-screen as soon as main-window is shown
	splashScreen.finish( &mainWindow );

	mainWindow.show();

	ilog( Info, "Exec" );

	// let's rock!!
	return app.exec();
}

