/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2004-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>

#include "MainWindow.h"
#include "AuthenticationCredentials.h"
#include "ClassroomManager.h"
#include "Dialogs.h"
#include "PasswordDialog.h"
#include "WelcomeWidget.h"
#include "ComputerManagementView.h"
#include "SnapshotManagementWidget.h"
#include "ConfigWidget.h"
#include "FeatureManager.h"
#include "ToolButton.h"
#include "ItalcConfiguration.h"
#include "ItalcCoreConnection.h"
#include "ItalcVncConnection.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "MasterCore.h"
#include "RemoteControlWidget.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( MasterCore &masterCore ) :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_masterCore( masterCore ),
	m_modeGroup( new QButtonGroup( this ) ),
	m_systemTrayIcon( this )
{
	ui->setupUi( this );

	setWindowTitle( QString( "%1 Master %2" ).arg( ItalcCore::applicationName() ).arg( ITALC_VERSION ) );

	ui->computerMonitoringView->setComputerManager( m_masterCore.computerManager() );
	ui->computerMonitoringView->setConfiguration( m_masterCore.personalConfig() );
	ui->computerMonitoringView->setFeatureManager( m_masterCore.featureManager() );

	// configure side bar
	ui->sideBar->setOrientation( Qt::Vertical );

	// create all sidebar workspaces
	m_welcomeWidget = new WelcomeWidget( ui->centralWidget );
	m_computerManagementView = new ComputerManagementView( m_masterCore.computerManager(), ui->centralWidget );
	m_snapshotManagementWidget = new SnapshotManagementWidget( ui->centralWidget );
	m_configWidget = new ConfigWidget( ui->centralWidget );

	// append sidebar-workspaces to sidebar
	ui->sideBar->appendTab( m_welcomeWidget, tr( "Welcome" ), QPixmap( ":/resources/view-calendar-month.png" ) );
	ui->sideBar->appendTab( m_computerManagementView, tr( "Computer management" ), QPixmap( ":/resources/applications-education.png" ) );
	ui->sideBar->appendTab( m_snapshotManagementWidget, tr( "Snapshot management" ), QPixmap( ":/resources/camera-photo.png" ) );
	ui->sideBar->appendTab( m_configWidget, tr( "Configuration" ), QPixmap( ":/resources/adjustrgb.png" ) );

	ui->centralLayout->insertWidget( 0, m_welcomeWidget );
	ui->centralLayout->insertWidget( 0, m_computerManagementView );
	ui->centralLayout->insertWidget( 0, m_snapshotManagementWidget );
	ui->centralLayout->insertWidget( 0, m_configWidget );

	// create the action-toolbar
	ui->toolBar->layout()->setSpacing( 4 );
	ui->toolBar->setObjectName( "MainToolBar" );
	ui->toolBar->toggleViewAction()->setEnabled( FALSE );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

	ToolButton * scr = new ToolButton(
			QPixmap( ":/resources/applications-education.png" ),
			tr( "Classroom" ), QString::null,
			tr( "Click this button to open a menu where you can "
				"choose the active classroom." ) );
	//scr->setMenu( m_classroomManager->quickSwitchMenu() );
	scr->setPopupMode( ToolButton::InstantPopup );
	scr->setWhatsThis( tr( "Click this button to switch between classrooms." ) );


	scr->addTo( ui->toolBar );

	addFeaturesToToolBar();
	addFeaturesToSystemTrayMenu();

	m_modeGroup->button( qHash( m_masterCore.featureManager().monitoringModeFeature().uid() ) )->setChecked( true );

/*	restoreState( QByteArray::fromBase64(
				m_classroomManager->winCfg().toUtf8() ) );
	QStringList hidden_buttons = m_classroomManager->toolBarCfg().
								split( '#' );
	foreach( QAction * a, ui->toolBar->actions() )
	{
		if( hidden_buttons.contains( a->text() ) )
		{
			a->setVisible( FALSE );
		}
	}

	foreach( QAbstractButton * btn, m_sideBar->tabs() )
	{
		if( hidden_buttons.contains( btn->text() ) )
		{
			btn->setVisible( false );
		}
	}*/

/*	ItalcVncConnection * conn = new ItalcVncConnection( this );
	// attach ItalcCoreConnection to it so we can send extended iTALC commands
	m_localICA = new ItalcCoreConnection( conn );

	conn->setHost( QHostAddress( QHostAddress::LocalHost ).toString() );
	conn->setPort( ItalcCore::config->coreServerPort() );
	conn->start();

	if( !conn->waitForConnected( 5000 ) )
	{
		QMessageBox::information( this,
			tr( "Could not contact %1 service" ).arg( ItalcCore::applicationName() ),
			tr( "Could not contact the local %1 service. It is likely "
				"that you entered wrong credentials or key files are "
				"not set up properly. Try again or contact your "
				"administrator for solving this problem using the %2 "
				"Configurator." ).arg( ItalcCore::applicationName() ).
								  arg( ItalcCore::applicationName() ) );
		if( ItalcCore::config->logLevel() < Logger::LogLevelDebug )
		{
			return;
		}
	}

	// update the role under which ICA is running
	m_localICA->setRole( ItalcCore::role );
	m_localICA->startDemoServer( ItalcCore::config->coreServerPort(),
									ItalcCore::config->demoServerPort() );*/

	// setup system tray icon
	QIcon icon( ":/resources/icon16.png" );
	icon.addFile( ":/resources/icon22.png" );
	icon.addFile( ":/resources/icon32.png" );

	m_systemTrayIcon.setIcon( icon );
	m_systemTrayIcon.setToolTip( tr( "%1 Master Control" ).arg( ItalcCore::applicationName() ) );
	m_systemTrayIcon.show();
	connect( &m_systemTrayIcon, SIGNAL( activated(
					QSystemTrayIcon::ActivationReason ) ),
		this, SLOT( handleSystemTrayEvent(
					QSystemTrayIcon::ActivationReason ) ) );

	ItalcCore::enforceBranding( this );
}




MainWindow::~MainWindow()
{
	m_systemTrayIcon.hide();
}




bool MainWindow::initAuthentication()
{
	if( ItalcCore::initAuthentication() )
	{
		return true;
	}

	if( ItalcCore::role != ItalcCore::RoleTeacher )
	{
		ItalcCore::role = ItalcCore::RoleTeacher;
		return initAuthentication();
	}

	// if we have logon credentials, assume they are fine and continue
	if( ItalcCore::authenticationCredentials->hasCredentials(
									AuthenticationCredentials::UserLogon ) )
	{
		return true;
	}

	QMessageBox::information( NULL,
			tr( "Authentication impossible" ),
			tr(	"No authentication key files were found or your current ones "
				"are outdated. Please create new key files using the %1 "
				"Configurator. Alternatively set up logon authentication "
				"using the %1 Configurator. Otherwise you won't be "
				"able to access computers using %1." ).arg( ItalcCore::applicationName() ) );

	return false;
}




void MainWindow::keyPressEvent( QKeyEvent* event )
{
	if( event->key() == Qt::Key_F11 )
	{
		QWidget::setWindowState( QWidget::windowState() ^ Qt::WindowFullScreen );
	}
	else
	{
		QMainWindow::keyPressEvent( event );
	}
}



void MainWindow::handleSystemTrayEvent( QSystemTrayIcon::ActivationReason _r )
{
	switch( _r )
	{
		case QSystemTrayIcon::Trigger:
			setVisible( !isVisible() );
			break;
		case QSystemTrayIcon::Context:
		{
			QMenu m( this );
			m.addAction( m_systemTrayIcon.toolTip() )->setEnabled( FALSE );
			foreach( QAction * a, m_sysTrayActions )
			{
				m.addAction( a );
			}

			m.addSeparator();

			QMenu rcm( this );
			QAction * rc = m.addAction( tr( "Remote control" ) );
			rc->setMenu( &rcm );
			for( const auto& computer : m_masterCore.computerManager().computerList() )
			{
				rcm.addAction( computer.name() )->setData( computer.hostAddress() );
			}
			connect( &rcm, SIGNAL( triggered( QAction * ) ),
				this,
				SLOT( remoteControlClient( QAction * ) ) );

			m.addSeparator();

			QAction * qa = m.addAction(
					QIcon( ":/resources/application-exit.png" ),
					tr( "Quit" ) );
			connect( qa, SIGNAL( triggered( bool ) ),
					this, SLOT( close() ) );
			m.exec( QCursor::pos() );
			break;
		}
		default:
			break;
	}
}



#if 0
void MainWindow::remoteControlClient( QAction* action )
{
	show();
/*	remoteControlDisplay( _a->data().toString(),
				m_classroomManager->clientDblClickAction() );*/
}




void MainWindow::remoteControlDisplay( const QString& hostname,
										bool viewOnly,
										bool stopDemoAfterwards )
{
	if( m_remoteControlWidget )
	{
		return;
	}

	m_remoteControlWidget = new RemoteControlWidget( hostname, viewOnly );

	// determine screen offset where to show the remote control window
	int x = 0;
	for( int i = 0; i < m_remoteControlScreen; ++i )
	{
		x += QApplication::desktop()->screenGeometry( i ).width();
	}
	m_remoteControlWidget->move( x, 0 );

	if( stopDemoAfterwards )
	{
		connect( m_remoteControlWidget, SIGNAL( objectDestroyed( QObject* ) ),
				this, SLOT( stopDemoAfterRemoteControl() ) );
	}
}
#endif



void MainWindow::addFeaturesToToolBar()
{
	for( auto feature : m_masterCore.featureManager().features() )
	{
		ToolButton* btn = new ToolButton( feature.icon(),
										  feature.displayName(),
										  feature.displayNameActive(),
										  feature.description() );
		connect( btn, &QToolButton::clicked, [=] () {
			m_masterCore.computerManager().runFeature( m_masterCore.featureManager(), feature, this );
			updateModeButtonGroup();
		} );
		btn->addTo( ui->toolBar );

		switch( feature.type() )
		{
		case Feature::Mode:
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, qHash( feature.uid() ) );
			break;
		default:
			break;
		}
	}
}



void MainWindow::addFeaturesToSystemTrayMenu()
{
	// TODO
}



void MainWindow::updateModeButtonGroup()
{
	if( m_masterCore.computerManager().currentMode() == m_masterCore.featureManager().monitoringModeFeature().uid() )
	{
		m_modeGroup->button( qHash( m_masterCore.featureManager().monitoringModeFeature().uid() ) )->setChecked( true );
	}
}
