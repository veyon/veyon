/*
 * main.cpp - startup routine for Veyon Master Application
 *
 * Copyright (c) 2004-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include "DocumentationFigureCreator.h"
#include "VeyonMaster.h"
#include "MainWindow.h"


int main( int argc, char** argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );
	app.connect( &app, &QApplication::lastWindowClosed, &QApplication::quit );

	VeyonCore core( &app, VeyonCore::Component::Master, QStringLiteral("Master") );

#ifdef VEYON_DEBUG
	if( qEnvironmentVariableIsSet( "VEYON_MASTER_CREATE_DOC_FIGURES") )
	{
		DocumentationFigureCreator().run();
		return 0;
	}
#endif

	QSplashScreen splashScreen( QPixmap( QStringLiteral(":/master/splash.png") ) );
	splashScreen.show();

	if( MainWindow::initAuthentication() == false ||
			MainWindow::initAccessControl() == false )
	{
		return -1;
	}

	VeyonMaster masterCore( &core );

	// hide splash-screen as soon as main-window is shown
	splashScreen.finish( masterCore.mainWindow() );

	masterCore.mainWindow()->show();

	return core.exec();
}
