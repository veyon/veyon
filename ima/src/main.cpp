/*
 * main.cpp - main-file for iTALC-Application
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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



QSplashScreen * splashScreen = NULL;

QString __demo_network_interface;
QString __demo_master_ip;
QString __default_domain;
int __demo_quality = 0;


// good old main-function... initializes qt-app and starts iTALC
int main( int argc, char * * argv )
{
	QApplication app( argc, argv );

	qRegisterMetaType<QModelIndex>( "QModelIndex" );
	qRegisterMetaType<quint16>( "quint16" );

	localSystem::initialize();

	__role = ISD::RoleTeacher;

	app.connect( &app, SIGNAL( lastWindowClosed() ), SLOT( quit() ) );

	splashScreen = new QSplashScreen( QPixmap( ":/resources/splash.png" ) );
	splashScreen->show();


	// load translations
	const QString loc = QLocale::system().name().left( 2 );

	QTranslator app_tr;
	app_tr.load( ":/resources/" + loc + ".qm" );
	app.installTranslator( &app_tr );

	QTranslator qt_tr;
	qt_tr.load( ":/resources/qt_" + loc + ".qm" );
	app.installTranslator( &qt_tr );

	// now create the main-window
	mainWindow * main_window = new mainWindow();

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

