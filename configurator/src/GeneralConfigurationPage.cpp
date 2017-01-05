/*
 * GeneralConfigurationPage.cpp - configuration page with general settings
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QTimer>

#include "GeneralConfigurationPage.h"
#include "FileSystemBrowser.h"
#include "ConfiguratorCore.h"
#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "Configuration/UiMapping.h"

#include "ui_GeneralConfigurationPage.h"


GeneralConfigurationPage::GeneralConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::GeneralConfigurationPage)
{
	ui->setupUi(this);

	// retrieve list of builtin translations and populate language combobox
	QStringList languages;
	for( auto language : QDir(":/resources/").entryList( QStringList("*.qm") ) )
	{
		QLocale loc(language);
		if( loc.language() == QLocale::C )
		{
			loc = QLocale( QLocale::English );
		}
		languages += QString( "%1 - %2 (%3)" ).arg( QLocale::languageToString(loc.language() ),
													loc.nativeLanguageName(),
													loc.name() );
	}

	qSort( languages );

	ui->uiLanguage->addItems( languages );

#ifndef ITALC_BUILD_WIN32
	ui->logToWindowsEventLog->hide();
#endif

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( startService );
	CONNECT_BUTTON_SLOT( stopService );

	CONNECT_BUTTON_SLOT( openLogFileDirectory );
	CONNECT_BUTTON_SLOT( clearLogFiles );

	updateServiceControl();

	QTimer *serviceUpdateTimer = new QTimer( this );
	serviceUpdateTimer->start( 2000 );

	connect( serviceUpdateTimer, SIGNAL( timeout() ),
				this, SLOT( updateServiceControl() ) );
}



GeneralConfigurationPage::~GeneralConfigurationPage()
{
	delete ui;
}



void GeneralConfigurationPage::resetWidgets()
{
	FOREACH_ITALC_UI_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void GeneralConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_ITALC_UI_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



bool GeneralConfigurationPage::isServiceRunning()
{
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "ItalcService", SERVICE_QUERY_STATUS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	SERVICE_STATUS status;
	QueryServiceStatus( hservice, &status );

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );

	return( status.dwCurrentState == SERVICE_RUNNING );
#else
	return false;
#endif
}



void GeneralConfigurationPage::startService()
{
	serviceControlWithProgressBar( tr( "Starting %1 service" ).arg( QCoreApplication::applicationName() ), "-startservice" );
}




void GeneralConfigurationPage::stopService()
{
	serviceControlWithProgressBar( tr( "Stopping %1 service" ).arg( QCoreApplication::applicationName() ), "-stopservice" );
}




void GeneralConfigurationPage::updateServiceControl()
{
	bool running = isServiceRunning();
#ifdef ITALC_BUILD_WIN32
	ui->startService->setEnabled( !running );
	ui->stopService->setEnabled( running );
#else
	ui->startService->setEnabled( false );
	ui->stopService->setEnabled( false );
#endif
	ui->serviceState->setText( running ? tr( "Running" ) : tr( "Stopped" ) );
}




void GeneralConfigurationPage::openLogFileDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->logFileDirectory );
}




void GeneralConfigurationPage::clearLogFiles()
{
#ifdef ITALC_BUILD_WIN32
	bool stopped = false;
	if( isServiceRunning() )
	{
		if( QMessageBox::question( this, tr( "%1 Service" ).arg( QCoreApplication::applicationName() ),
				tr( "The %1 service needs to be stopped temporarily "
					"in order to remove the log files. Continue?"
					).arg( QCoreApplication::applicationName() ), QMessageBox::Yes | QMessageBox::No,
				QMessageBox::Yes ) == QMessageBox::Yes )
		{
			stopService();
			stopped = true;
		}
		else
		{
			return;
		}
	}
#endif

	bool success = true;
	QDir d( LocalSystem::Path::expand( ItalcCore::config->logFileDirectory() ) );
	foreach( const QString &f, d.entryList( QStringList() << "Italc*.log" ) )
	{
		if( f != "ItalcManagementConsole.log" )
		{
			success &= d.remove( f );
		}
	}

#ifdef ITALC_BUILD_WIN32
	d = QDir( "C:\\Windows\\Temp" );
#else
	d = QDir( "/tmp" );
#endif

	foreach( const QString &f, d.entryList( QStringList() << "Italc*.log" ) )
	{
		if( f != "ItalcManagementConsole.log" )
		{
			success &= d.remove( f );
		}
	}

#ifdef ITALC_BUILD_WIN32
	if( stopped )
	{
		startService();
	}
#endif

	if( success )
	{
		QMessageBox::information( this, tr( "Log files cleared" ),
			tr( "All log files were cleared successfully." ) );
	}
	else
	{
		QMessageBox::critical( this, tr( "Error" ),
			tr( "Could not remove all log files." ) );
	}
}



void GeneralConfigurationPage::serviceControlWithProgressBar( const QString &title,
												const QString &arg )
{
	QProcess p;
	p.start( ConfiguratorCore::icaFilePath(), QStringList() << arg );
	p.waitForStarted();

	QProgressDialog pd( title, QString(), 0, 0, this );
	pd.setWindowTitle( windowTitle() );

	QProgressBar *b = new QProgressBar( &pd );
	b->setMaximum( 100 );
	b->setTextVisible( false );
	pd.setBar( b );
	b->show();
	pd.setWindowModality( Qt::WindowModal );
	pd.show();

	int j = 0;
	while( p.state() == QProcess::Running )
	{
		QApplication::processEvents();
		b->setValue( ++j % 100 );
		LocalSystem::sleep( 10 );
	}

	updateServiceControl();
}
