/*
 * main.cpp - main-file for iTALC-Application
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtCore/QLocale>
#include <QtCore/QModelIndex>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtGui/QSplashScreen>

#include "main_window.h"
#include "ivs_connection.h"
#include "local_system_ima.h"
#include "remote_control_widget.h"



QSplashScreen * splashScreen = NULL;

QString __demo_network_interface;
QString __demo_master_ip;
QString __default_domain;
int __demo_quality = 0;


// good old main-function... initializes qt-app and starts iTALC
int main( int argc, char * * argv )
{
	QApplication app( argc, argv );
	app.connect( &app, SIGNAL( lastWindowClosed() ), SLOT( quit() ) );

	// load translations
	const QString loc = QLocale::system().name().left( 2 );

	QTranslator app_tr;
	app_tr.load( ":/resources/" + loc + ".qm" );
	app.installTranslator( &app_tr );

	QTranslator core_tr;
	core_tr.load( ":/resources/" + loc + "-core.qm" );
	app.installTranslator( &core_tr );

	QTranslator qt_tr;
	qt_tr.load( ":/resources/qt_" + loc + ".qm" );
	app.installTranslator( &qt_tr );


	qRegisterMetaType<QModelIndex>( "QModelIndex" );
	qRegisterMetaType<quint16>( "quint16" );


	localSystem::initialize();

	__role = ISD::RoleTeacher;

	// parse arguments
	QStringListIterator arg_it( QCoreApplication::arguments() );
	arg_it.next();
	int screen = -1;
	while( argc > 1 && arg_it.hasNext() )
	{
		const QString & a = arg_it.next();
		if( a == "-rctrl" && arg_it.hasNext() )
		{
			const QString host = arg_it.next();
			bool view_only = arg_it.hasNext() ?
						arg_it.next().toInt()
					:
						FALSE;
			new remoteControlWidget( host, view_only );
			return( app.exec() );
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
	}



	splashScreen = new QSplashScreen( QPixmap( ":/resources/splash.png" ) );
	splashScreen->show();


	// now create the main-window
	mainWindow * main_window = new mainWindow( screen );

	if( !main_window->localISD() ||
		main_window->localISD()->state() != isdConnection::Connected )
	{
		return( -1 );
	}

	// hide splash-screen as soon as main-window is shown
	splashScreen->finish( main_window );

	main_window->show();

	// let's rock!!
	return( app.exec() );
}

