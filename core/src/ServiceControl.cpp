/*
 * ServiceControl.cpp - class for controlling veyon service
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "VeyonCore.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QThread>

#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "ServiceControl.h"


ServiceControl::ServiceControl(QWidget *parent) :
	QObject( parent ),
	m_parent( parent )
{
}



QString ServiceControl::serviceFilePath()
{
	QString path = QCoreApplication::applicationDirPath() + QDir::separator() + "veyon-service";
#ifdef VEYON_BUILD_WIN32
	path += ".exe";
#endif
	return QDTNS( path );
}



bool ServiceControl::isServiceRegistered()
{
#ifdef VEYON_BUILD_WIN32
	SC_HANDLE serviceManager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( !serviceManager )
	{
		return false;
	}

	SC_HANDLE serviceHandle = OpenService( serviceManager, L"VeyonService", SERVICE_QUERY_STATUS );
	if( !serviceHandle )
	{
		CloseServiceHandle( serviceManager );
		return false;
	}

	CloseServiceHandle( serviceHandle );
	CloseServiceHandle( serviceManager );

	return true;
#else
	return false;
#endif
}



bool ServiceControl::isServiceRunning()
{
#ifdef VEYON_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, L"VeyonService", SERVICE_QUERY_STATUS );
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



void ServiceControl::startService()
{
	serviceControl( tr( "Starting %1 Service" ).arg( VeyonCore::applicationName() ), { "-startservice" } );
}




void ServiceControl::stopService()
{
	serviceControl( tr( "Stopping %1 Service" ).arg( VeyonCore::applicationName() ), { "-stopservice" } );
}



void ServiceControl::registerService()
{
	serviceControl( tr( "Registering %1 Service" ).arg( VeyonCore::applicationName() ), { "-registerservice" } );
}



void ServiceControl::unregisterService()
{
	serviceControl( tr( "Unregistering %1 Service" ).arg( VeyonCore::applicationName() ), { "-unregisterservice" } );
}



void ServiceControl::serviceControl( const QString& title, QStringList arguments )
{
	// not running as graphical/interactive application?
	if( m_parent == nullptr )
	{
		// then prevent service application from showing any message boxes
		arguments.prepend( "-quiet" );
	}

	QProcess serviceProcess;
	serviceProcess.start( serviceFilePath(), arguments );
	serviceProcess.waitForStarted();

	if( m_parent )
	{
		graphicalFeedback( title, serviceProcess);
	}
	else
	{
		textFeedback( title, serviceProcess );
	}
}



void ServiceControl::graphicalFeedback( const QString &title, const QProcess& serviceProcess )
{
	QProgressDialog pd( title, QString(), 0, 0, m_parent );
	pd.setWindowTitle( VeyonCore::applicationName() );

	auto b = new QProgressBar( &pd );
	b->setMaximum( 100 );
	b->setTextVisible( false );
	pd.setBar( b );
	b->show();
	pd.setWindowModality( Qt::WindowModal );
	pd.show();

	int j = 0;
	while( serviceProcess.state() == QProcess::Running )
	{
		QCoreApplication::processEvents();
		b->setValue( ++j % 100 );
		QThread::msleep( 10 );
	}
}



void ServiceControl::textFeedback( const QString& title, const QProcess& serviceProcess )
{
	printf( "%s", qUtf8Printable( title ) );

	while( serviceProcess.state() == QProcess::Running )
	{
		QCoreApplication::processEvents();
		QThread::msleep( 200 );
		printf( "." );
	}
}
