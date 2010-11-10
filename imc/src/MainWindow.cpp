/*
 * MainWindow.cpp - implementation of MainWindow class
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

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>

#include "Configuration/XmlStore.h"
#include "Configuration/UiMapping.h"

#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "MainWindow.h"


MainWindow::MainWindow() :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_configChanged( false )
{
	ui->setupUi( this );

	setWindowTitle( tr( "iTALC Management Console %1" ).arg( ITALC_VERSION ) );

	// reset all widget's values to current configuration
	reset();

	// connect widget signals to configuration property write methods
	FOREACH_ITALC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)

	connect( ui->startService, SIGNAL( clicked() ),
				this, SLOT( startService() ) );
	connect( ui->stopService, SIGNAL( clicked() ),
				this, SLOT( stopService() ) );

	connect( ui->buttonBox, SIGNAL( clicked( QAbstractButton * ) ),
				this, SLOT( resetOrApply( QAbstractButton * ) ) );

	updateServiceControl();

	QTimer *serviceUpdateTimer = new QTimer( this );
	serviceUpdateTimer->start( 2000 );

	connect( serviceUpdateTimer, SIGNAL( timeout() ),
				this, SLOT( updateServiceControl() ) );

	connect( ItalcCore::config, SIGNAL( configurationChanged() ),
				this, SLOT( configurationChanged() ) );
}




MainWindow::~MainWindow()
{
}



void MainWindow::reset()
{
	ItalcCore::config->reloadFromStore();

	FOREACH_ITALC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY)

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
}




void MainWindow::apply()
{
	if( LocalSystem::Process::isRunningAsAdmin() )
	{
		ImcCore::applyConfiguration( *ItalcCore::config );
	}
	else
	{
		QTemporaryFile f;
		f.setAutoRemove( false );
		if( !f.open() )
		{
			ilog_failed( "QTemporaryFile::open()" );
			return;
		}
		// write current configuration to temporary file
		Configuration::XmlStore xs( Configuration::XmlStore::System,
									f.fileName() );
		xs.flush( ItalcCore::config );

		// launch ourselves as admin again and apply configuration from
		// temporary file
		LocalSystem::Process::runAsAdmin(
				QCoreApplication::applicationFilePath(),
				QString( "-apply %1" ).arg( f.fileName() ) );
	}

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
}




void MainWindow::configurationChanged()
{
	ui->buttonBox->setEnabled( true );
	m_configChanged = true;
}




void MainWindow::resetOrApply( QAbstractButton *btn )
{
	if( ui->buttonBox->standardButton( btn ) & QDialogButtonBox::Apply )
	{
		apply();
	}
	else if( ui->buttonBox->standardButton( btn ) & QDialogButtonBox::Reset )
	{
		reset();
	}
}




void MainWindow::startService()
{
	QProcess::startDetached( ImcCore::icaFilePath() + " -startservice" );

	updateServiceControl();
}




void MainWindow::stopService()
{
	QProcess::startDetached( ImcCore::icaFilePath() + " -stopservice" );

	updateServiceControl();
}




void MainWindow::updateServiceControl()
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




bool MainWindow::isServiceRunning()
{
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "icas", SERVICE_QUERY_STATUS );
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
