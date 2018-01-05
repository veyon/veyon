/*
 * main.cpp - main file for Veyon Configurator
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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
#include <QDir>

#include "ConfigurationTestController.h"
#include "ConfiguratorCore.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "MainWindow.h"
#include "PlatformCoreFunctions.h"
#include "Logger.h"



bool checkWritableConfiguration()
{
	if( !VeyonConfiguration().isStoreWritable() &&
			VeyonCore::config().logLevel() < Logger::LogLevelDebug )
	{
		ConfiguratorCore::criticalMessage( MainWindow::tr( "Configuration not writable" ),
			MainWindow::tr( "The local configuration backend reported that the "
							"configuration is not writable! Please run the %1 "
							"Configurator with higher privileges." ).arg( VeyonCore::applicationName() ) );
		return false;
	}

	return true;
}



int createKeyPair( QStringListIterator& argIt )
{
	const QString destDir = argIt.hasNext() ? argIt.next() : QString();
	ConfiguratorCore::createKeyPair( VeyonCore::instance()->userRole(), destDir );
	return 0;
}



int importPublicKey( QStringListIterator& argIt )
{
	QString pubKeyFile;
	if( !argIt.hasNext() )
	{
		QStringList l =
			QDir::current().entryList( QStringList() << QStringLiteral("*.key.txt"),
										QDir::Files | QDir::Readable );
		if( l.size() != 1 )
		{
			qCritical( "Please specify location of the public key "
						"to import" );
			return -1;
		}
		pubKeyFile = QDir::currentPath() + QDir::separator() +
											l.first();
		qWarning() << "No public key file specified. Trying to import "
						"the public key file found at" << pubKeyFile;
	}
	else
	{
		pubKeyFile = argIt.next();
	}

	if( ConfiguratorCore::importPublicKey( VeyonCore::instance()->userRole(),
										   pubKeyFile,
										   QString() ) == false )
	{
		qInfo( "Public key import failed" );
		return -1;
	}

	qInfo( "Public key successfully imported" );

	return 0;
}



bool parseRole( QStringListIterator& argIt )
{
	if( argIt.hasNext() )
	{
		const QString role = argIt.next();
		if( role == QStringLiteral("teacher") )
		{
			VeyonCore::instance()->setUserRole( VeyonCore::RoleTeacher );
		}
		else if( role == QStringLiteral("admin") )
		{
			VeyonCore::instance()->setUserRole( VeyonCore::RoleAdmin );
		}
		else if( role == QStringLiteral("supporter") )
		{
			VeyonCore::instance()->setUserRole( VeyonCore::RoleSupporter );
		}
	}
	else
	{
		qCritical( "-role needs an argument:\n"
			"	teacher\n"
			"	admin\n"
			"	supporter\n\n" );
		return false;
	}

	return true;
}



int main( int argc, char **argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );

	VeyonCore core( &app, QStringLiteral("Configurator") );

	// make sure to run as admin
	if( VeyonCore::platform().coreFunctions().isRunningAsAdmin() == false )
	{
		VeyonCore::platform().coreFunctions().runProgramAsAdmin( QCoreApplication::applicationFilePath(),
																 app.arguments().mid( 1 ) );
		return 0;
	}

	if( checkWritableConfiguration() == false )
	{
		return -1;
	}

	// parse arguments
	QStringListIterator argIt( app.arguments() );
	argIt.next();

	while( argc > 1 && argIt.hasNext() )
	{
		const QString a = argIt.next().toLower();

		if( a == QStringLiteral("-role") )
		{
			if( parseRole( argIt ) == false )
			{
				return -1;
			}
		}
		else if( a == QStringLiteral("-createkeypair") )
		{
			return createKeyPair( argIt );
		}
		else if( a == QStringLiteral("-importpublickey") || a == QStringLiteral("-i") )
		{
			return importPublicKey( argIt );
		}
		else if( a == QStringLiteral("-test") )
		{
			return ConfigurationTestController( app.arguments().mid( 2 ) ).run();
		}
	}

	// now create the main window
	auto mainWindow = new MainWindow;

	mainWindow->show();

	ilog( Info, "App.Exec" );

	return app.exec();
}
