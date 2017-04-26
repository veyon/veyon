/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2004-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>

#include "AboutDialog.h"
#include "MainWindow.h"
#include "BuiltinFeatures.h"
#include "AuthenticationCredentials.h"
#include "ComputerManager.h"
#include "ComputerManagementView.h"
#include "SnapshotManagementWidget.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "MasterCore.h"
#include "UserConfig.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( MasterCore &masterCore ) :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_masterCore( masterCore ),
	m_modeGroup( new QButtonGroup( this ) ),
	m_systemTrayIcon( this )
{
	ui->setupUi( this );

	setWindowTitle( QString( "%1 Master %2" ).arg( VeyonCore::applicationName() ).arg( VEYON_VERSION ) );

	restoreState( QByteArray::fromBase64( m_masterCore.userConfig().windowState().toUtf8() ) );
	restoreGeometry( QByteArray::fromBase64( m_masterCore.userConfig().windowGeometry().toUtf8() ) );

	ui->computerMonitoringView->setMasterCore( m_masterCore );

	// add widgets to status bar
	ui->statusBar->addWidget( ui->computerManagementButton );
	ui->statusBar->addWidget( ui->snapshotManagementButton );
	ui->statusBar->addWidget( ui->spacerLabel, 1 );
	ui->statusBar->addWidget( ui->gridSizeSlider, 2 );
	ui->statusBar->addWidget( ui->autoFitButton );
	ui->statusBar->addWidget( ui->spacerLabel2 );
	ui->statusBar->addWidget( ui->aboutButton );

	// create all views
	auto splitter = new QSplitter( Qt::Horizontal, ui->centralWidget );
	splitter->setChildrenCollapsible( false );

	ui->centralLayout->addWidget( splitter );

	m_computerManagementView = new ComputerManagementView( m_masterCore.computerManager(), splitter );
	m_snapshotManagementWidget = new SnapshotManagementWidget( splitter );

	splitter->addWidget( m_computerManagementView );
	splitter->addWidget( m_snapshotManagementWidget );
	splitter->addWidget( ui->computerMonitoringView );

	// hide views per default and connect related button
	m_computerManagementView->hide();
	m_snapshotManagementWidget->hide();

	connect( ui->computerManagementButton, &QAbstractButton::toggled,
			 m_computerManagementView, &QWidget::setVisible );
	connect( ui->snapshotManagementButton, &QAbstractButton::toggled,
			 m_snapshotManagementWidget, &QWidget::setVisible );


	// initialize monitoring screen size slider
	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 ui->computerMonitoringView, &ComputerMonitoringView::setComputerScreenSize );
	connect( ui->autoFitButton, &QToolButton::clicked,
			 ui->computerMonitoringView, &ComputerMonitoringView::autoAdjustComputerScreenSize );

	int size = ComputerMonitoringView::DefaultComputerScreenSize;
	if( m_masterCore.userConfig().monitoringScreenSize() >= ui->gridSizeSlider->minimum() )
	{
		size = m_masterCore.userConfig().monitoringScreenSize();
	}

	ui->gridSizeSlider->setValue( size );


	// create the main toolbar
	ui->toolBar->layout()->setSpacing( 4 );
	ui->toolBar->toggleViewAction()->setEnabled( false );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

#if 0
	ToolButton * scr = new ToolButton(
			QPixmap( ":/resources/applications-education.png" ),
			tr( "Classroom" ), QString::null,
			tr( "Click this button to open a menu where you can "
				"choose the active classroom." ) );
	//scr->setMenu( m_classroomManager->quickSwitchMenu() );
	scr->setPopupMode( ToolButton::InstantPopup );
	scr->setWhatsThis( tr( "Click this button to switch between classrooms." ) );

	scr->addTo( ui->toolBar );
#endif

	addFeaturesToToolBar();
	addFeaturesToSystemTrayMenu();

	m_modeGroup->button( qHash( m_masterCore.builtinFeatures().monitoringMode().feature().uid() ) )->setChecked( true );

	// setup system tray icon
	QIcon icon( ":/resources/icon16.png" );
	icon.addFile( ":/resources/icon22.png" );
	icon.addFile( ":/resources/icon32.png" );
	icon.addFile( ":/resources/icon64.png" );

	m_systemTrayIcon.setIcon( icon );
	m_systemTrayIcon.setToolTip( tr( "%1 Master Control" ).arg( VeyonCore::applicationName() ) );
	m_systemTrayIcon.show();
	connect( &m_systemTrayIcon, SIGNAL( activated(
					QSystemTrayIcon::ActivationReason ) ),
		this, SLOT( handleSystemTrayEvent(
					QSystemTrayIcon::ActivationReason ) ) );

	VeyonCore::enforceBranding( this );
}




MainWindow::~MainWindow()
{
	m_systemTrayIcon.hide();
}




bool MainWindow::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication( AuthenticationCredentials::AllTypes ) )
	{
		return true;
	}

	if( VeyonCore::instance()->userRole() != VeyonCore::RoleTeacher )
	{
		VeyonCore::instance()->setUserRole( VeyonCore::RoleTeacher );
		return initAuthentication();
	}

	// if we have logon credentials, assume they are fine and continue
	if( VeyonCore::authenticationCredentials().hasCredentials(
									AuthenticationCredentials::UserLogon ) )
	{
		return true;
	}

	QMessageBox::information( nullptr,
			tr( "Authentication impossible" ),
			tr(	"No authentication key files were found or your current ones "
				"are outdated. Please create new key files using the %1 "
				"Configurator. Alternatively set up logon authentication "
				"using the %1 Configurator. Otherwise you won't be "
				"able to access computers using %1." ).arg( VeyonCore::applicationName() ) );

	return false;
}



void MainWindow::closeEvent( QCloseEvent* event )
{
	m_masterCore.userConfig().setWindowState( saveState().toBase64() );
	m_masterCore.userConfig().setWindowGeometry( saveGeometry().toBase64() );

	QMainWindow::closeEvent( event );
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
			m.addAction( m_systemTrayIcon.toolTip() )->setEnabled( false );
			for( auto action : m_sysTrayActions )
			{
				m.addAction( action );
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



void MainWindow::showAboutDialog()
{
	AboutDialog( this ).exec();
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
	for( auto feature : m_masterCore.features() )
	{
		ToolButton* btn = new ToolButton( feature.iconUrl(),
										  feature.displayName(),
										  feature.displayNameActive(),
										  feature.description() );
		connect( btn, &QToolButton::clicked, [=] () {
			m_masterCore.runFeature( feature, this );
			updateModeButtonGroup();
		} );
		btn->addTo( ui->toolBar );

		if( feature.testFlag( Feature::Mode ) )
		{
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, qHash( feature.uid() ) );
		}
	}
}



void MainWindow::addFeaturesToSystemTrayMenu()
{
	// TODO
}



void MainWindow::updateModeButtonGroup()
{
	const Feature::Uid& monitoringMode = m_masterCore.builtinFeatures().monitoringMode().feature().uid();

	if( m_masterCore.currentMode() == monitoringMode )
	{
		m_modeGroup->button( qHash( monitoringMode ) )->setChecked( true );
	}
}
