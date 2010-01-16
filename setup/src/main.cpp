/*
 * main.cpp - main file for iTALC setup tool
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

#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>

#include "dialogs.h"
#include "local_system.h"



// good old main-function... initializes qt-app and starts iTALC
int main( int argc, char * * argv )
{
	QApplication app( argc, argv );

	app.connect( &app, SIGNAL( lastWindowClosed() ), SLOT( quit() ) );

	localSystem::initialize( NULL, "italc_setup.log" );

	// load translations
	const QString loc = QLocale::system().name().left( 2 );

	QTranslator app_tr;
	app_tr.load( ":/resources/" + loc + ".qm" );
	app.installTranslator( &app_tr );

	QTranslator qt_tr;
	qt_tr.load( ":/resources/qt_" + loc + ".qm" );
	app.installTranslator( &qt_tr );

	QString installDir = QCoreApplication::applicationDirPath();
	if( app.arguments().size() > 1 && QFileInfo( app.arguments()[1] ).isDir() )
	{
		installDir = app.arguments()[1];
	}
	setupWizard sw( installDir );

	if( app.arguments().size() > 1 )
	{
		QString arg = app.arguments()[1];
		if( QFileInfo( arg ).exists() )
		{
#ifdef BUILD_WIN32
			// UNC-paths on windows need some special handling
			// as QFile does not relocate relative file-paths
			// while current-dir of application is a UNC-path
			if( !QFileInfo( arg ).isAbsolute() &&
				QDir::currentPath().left( 2 ) == "//" )
			{
				arg = QDir::currentPath()+"/"+arg;
			}
#endif
			sw.loadSettings( arg );
			sw.doInstallation( TRUE );
			return( 0 );
		}
		else
		{
			QMessageBox::information( NULL,
					setupWizard::tr( "File does not exist" ),
					setupWizard::tr( "The file %1 could not be "
						"found. Please check this and "
						"try again." ).
						arg( app.arguments()[1] ) );
		}
	}
	else
	{
		sw.show();
	}

	return app.exec();
}

