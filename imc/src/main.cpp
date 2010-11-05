/*
 * main.cpp - main file for iTALC Management Console
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtCore/QTranslator>
#include <QtGui/QApplication>

#include "ImcCore.h"
#include "MainWindow.h"
#include "LocalSystem.h"
#include "Logger.h"



// good old main-function... initializes and starts application
int main( int argc, char **argv )
{
	QApplication app( argc, argv );

	LocalSystem::initialize( NULL );

	Logger l( "ItalcManagementConsole" );

	app.connect( &app, SIGNAL( lastWindowClosed() ), SLOT( quit() ) );

	const QString loc = QLocale::system().name().left( 2 );

	foreach( const QString & qm, QStringList()
												<< loc + "-core"
												<< loc
												<< "qt_" + loc )
	{
		QTranslator * tr = new QTranslator( &app );
		tr->load( QString( ":/resources/%1.qm" ).arg( qm ) );
		app.installTranslator( tr );
	}

	ImcCore::init();

	// parse arguments
	QStringListIterator argIt( QCoreApplication::arguments() );
	argIt.next();

	while( argc > 1 && argIt.hasNext() )
	{
		const QString & a = argIt.next();
	}

	// now create the main window
	MainWindow mainWindow;

	mainWindow.show();

	ilog( Info, "App.Exec" );

	int ret = app.exec();

	ImcCore::deinit();

	return ret;
}

