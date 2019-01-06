/*
 * main.cpp - main file for Veyon Configurator
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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
#include <QMessageBox>

#include "ConfigurationTestController.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "MainWindow.h"
#include "PlatformCoreFunctions.h"
#include "Logger.h"



int main( int argc, char **argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );

	VeyonCore core( &app, QStringLiteral("Configurator") );

	// make sure to run as admin
	if( qEnvironmentVariableIntValue( "VEYON_CONFIGURATOR_NO_ELEVATION" ) == 0 &&
			VeyonCore::platform().coreFunctions().isRunningAsAdmin() == false )
	{
		if( VeyonCore::platform().coreFunctions().runProgramAsAdmin( QCoreApplication::applicationFilePath(),
																	 app.arguments().mid( 1 ) ) )
		{
			return 0;
		}

		QMessageBox::warning( nullptr, MainWindow::tr( "Insufficient privileges" ),
							  MainWindow::tr( "Could not start with administrative privileges. "
											  "Please make sure a sudo-like program is installed "
											  "for your desktop environment! The program will "
											  "be run with normal user privileges.") );
	}

	if( !VeyonConfiguration().isStoreWritable() &&
			VeyonCore::config().logLevel() < Logger::LogLevelDebug )
	{
		QMessageBox::critical( nullptr,
							   MainWindow::tr( "Configuration not writable" ),
							   MainWindow::tr( "The local configuration backend reported that the "
											   "configuration is not writable! Please run the %1 "
											   "Configurator with higher privileges." ).arg( VeyonCore::applicationName() ) );
		return -1;
	}

	// parse arguments
	QStringListIterator argIt( app.arguments() );
	argIt.next();

	while( argc > 1 && argIt.hasNext() )
	{
		const QString a = argIt.next().toLower();

		if( a == QStringLiteral("-test") )
		{
			return ConfigurationTestController( app.arguments().mid( 2 ) ).run();
		}
	}

	// now create the main window
	auto mainWindow = new MainWindow;

	mainWindow->show();

	qInfo( "App.Exec" );

	return app.exec();
}
