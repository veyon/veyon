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

#include <QDesktopWidget>
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
#include "ToolButton.h"
#include "ItalcConfiguration.h"
#include "ItalcCoreConnection.h"
#include "ItalcVncConnection.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "MasterCore.h"
#include "RemoteControlWidget.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( int _rctrl_screen ) :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_systemTrayIcon( this ),
	m_openedTabInSideBar( 1 ),
	//m_localICA( NULL ),
	m_remoteControlWidget( NULL ),
	m_remoteControlScreen( _rctrl_screen > -1 ?
				qMin( _rctrl_screen,
					QApplication::desktop()->numScreens() )
				:
				QApplication::desktop()->screenNumber( this ) )
{
	ui->setupUi( this );

	setWindowTitle( QString( "%1 Master %2" ).arg( ItalcCore::applicationName() ).arg( ITALC_VERSION ) );

/*	if( LocalSystem::Path::ensurePathExists(
						LocalSystem::Path::personalConfigDataPath() ) == false )
	{
		if( splashScreen != NULL )
		{
			splashScreen->hide();
		}
		QMessageBox::information( this, tr( "No write access" ),
			tr( "Could not read/write or create directory %1! "
			"For running %2, make sure you're permitted to "
			"create or write this directory." ).arg(
					LocalSystem::Path::personalConfigDataPath() ).
								  arg( ItalcCore::applicationName() ) );
		return;
	}*/

	// configure side bar
	ui->sideBar->setOrientation( Qt::Vertical );

	// create all sidebar workspaces
	m_welcomeWidget = new WelcomeWidget( ui->centralWidget );
	m_computerManagementView = new ComputerManagementView( ui->centralWidget );
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
	ui->toolBar->setObjectName( "maintoolbar" );
	ui->toolBar->toggleViewAction()->setEnabled( FALSE );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

	ToolButton * scr = new ToolButton(
			QPixmap( ":/resources/applications-education.png" ),
			tr( "Classroom" ), QString::null,
			tr( "Switch classroom" ),
			tr( "Click this button to open a menu where you can "
				"choose the active classroom." ),
			NULL, NULL, ui->toolBar );
	//scr->setMenu( m_classroomManager->quickSwitchMenu() );
	scr->setPopupMode( ToolButton::InstantPopup );
	scr->setWhatsThis( tr( "Click on this button, to switch between "
							"classrooms." ) );

	m_modeGroup = new QButtonGroup( this );

	QAction * a;

	a = new QAction( QIcon( ":/resources/presentation-none.png" ),
						tr( "Overview mode" ), this );
	m_sysTrayActions << a;
	ToolButton * overview_mode = new ToolButton(
			a, tr( "Overview" ), QString::null,
			tr( "This is the default mode in %1 and allows you "
				"to have an overview over all visible "
				"computers. Also click on this button for "
				"unlocking locked workstations or for leaving "
				"demo-mode." ).arg( ItalcCore::applicationName() ),
			this, SLOT( mapOverview() ), ui->toolBar );


	a = new QAction( QIcon( ":/resources/presentation-fullscreen.png" ),
						tr( "Fullscreen demo" ), this );
	m_sysTrayActions << a;
	ToolButton * fsdemo_mode = new ToolButton(
			a, tr( "Fullscreen Demo" ), tr( "Stop Demo" ),
			tr( "In this mode your screen is being displayed on "
				"all shown computers. Furthermore the users "
				"aren't able to do something else as all input "
				"devices are locked in this mode." ),
			this, SLOT( mapFullscreenDemo() ), ui->toolBar );

	a = new QAction( QIcon( ":/resources/presentation-window.png" ),
						tr( "Window demo" ), this );
	m_sysTrayActions << a;
	ToolButton * windemo_mode = new ToolButton(
			a, tr( "Window Demo" ), tr( "Stop Demo" ),
			tr( "In this mode your screen being displayed in a "
				"window on all shown computers. The users are "
				"able to switch to other windows and thus "
				"can continue to work." ),
			this, SLOT( mapWindowDemo() ), ui->toolBar );

	a = new QAction( QIcon( ":/resources/system-lock-screen.png" ),
					tr( "Lock/unlock desktops" ), this );
	m_sysTrayActions << a;
	ToolButton * lock_mode = new ToolButton(
			a, tr( "Lock all" ), tr( "Unlock all" ),
			tr( "To have all user's full attention you can lock "
				"their desktops using this button. "
				"In this mode all input devices are locked and "
				"the screen is black." ),
			this, SLOT( mapScreenLock() ), ui->toolBar );

	overview_mode->setCheckable( TRUE );
	fsdemo_mode->setCheckable( TRUE );
	windemo_mode->setCheckable( TRUE );
	lock_mode->setCheckable( TRUE );

	m_modeGroup->addButton( overview_mode, Computer::ModeMonitoring );
	m_modeGroup->addButton( fsdemo_mode, Computer::ModeFullScreenDemo );
	m_modeGroup->addButton( windemo_mode, Computer::ModeWindowDemo );
	m_modeGroup->addButton( lock_mode, Computer::ModeLocked );

	overview_mode->setChecked( TRUE );



	a = new QAction( QIcon( ":/resources/dialog-information.png" ),
					tr( "Send text message" ), this );
//	m_sysTrayActions << a;
	ToolButton * text_msg = new ToolButton(
			a, tr( "Text message" ), QString::null,
			tr( "Use this button to send a text message to all "
				"users e.g. to tell them new tasks etc." ),
			&MasterCore::i().computerManager(), SLOT( sendMessage() ), ui->toolBar );


	a = new QAction( QIcon( ":/resources/preferences-system-power-management.png" ),
					tr( "Power on computers" ), this );
	m_sysTrayActions << a;
	ToolButton * power_on = new ToolButton(
			a, tr( "Power on" ), QString::null,
			tr( "Click this button to power on all visible "
				"computers. This way you do not have to turn "
				"on each computer by hand." ),
			&MasterCore::i().computerManager(), SLOT( powerOnClients() ),
								ui->toolBar );

	a = new QAction( QIcon( ":/resources/system-shutdown.png" ),
					tr( "Power down computers" ), this );
	m_sysTrayActions << a;
	ToolButton * power_off = new ToolButton(
			a, tr( "Power down" ), QString::null,
			tr( "To power down all shown computers (e.g. after "
				"the lesson has finished) you can click this "
				"button." ),
			&MasterCore::i().computerManager(),
					SLOT( powerDownClients() ), ui->toolBar );

	ToolButton * directsupport = new ToolButton(
			QPixmap( ":/resources/remote_control.png" ),
			tr( "Support" ), QString::null,
			tr( "Direct support" ),
			tr( "If you need to support someone at a certain "
				"computer you can click this button and enter "
				"the according hostname or IP afterwards." ),
			&MasterCore::i().computerManager(), SLOT( directSupport() ), ui->toolBar );

/*	ToolButton * adjust_size = new ToolButton(
			QPixmap( ":/resources/zoom-fit-best.png" ),
			tr( "Adjust/align" ), QString::null,
			tr( "Adjust windows and their size" ),
			tr( "When clicking this button the biggest possible "
				"size for the client-windows is adjusted. "
				"Furthermore all windows are aligned." ),
			&MasterCore::i().computerManager(), SLOT( adjustWindows() ), ui->toolBar );

	ToolButton * auto_arrange = new ToolButton(
			QPixmap( ":/resources/vcs-locally-modified.png" ),
			tr( "Auto view" ), QString::null,
			tr( "Auto re-arrange windows and their size" ),
			tr( "When clicking this button all visible windows "
					"are re-arranged and adjusted." ),
			NULL, NULL, ui->toolBar );
	auto_arrange->setCheckable( true );
	auto_arrange->setChecked( m_classroomManager->isAutoArranged() );
	connect( auto_arrange, SIGNAL( toggled( bool ) ), m_classroomManager,
						 SLOT( arrangeWindowsToggle ( bool ) ) );*/

	scr->addTo( ui->toolBar );
	overview_mode->addTo( ui->toolBar );
	fsdemo_mode->addTo( ui->toolBar );
	windemo_mode->addTo( ui->toolBar );
	lock_mode->addTo( ui->toolBar );
	text_msg->addTo( ui->toolBar );
	power_on->addTo( ui->toolBar );
	power_off->addTo( ui->toolBar );
	directsupport->addTo( ui->toolBar );
	//adjust_size->addTo( ui->toolBar );
	//auto_arrange->addTo( ui->toolBar );

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


	//m_updateThread = new MainWindowUpdateThread( this );

	ItalcCore::enforceBranding( this );
}




MainWindow::~MainWindow()
{
	/*m_localICA->stopDemoServer();

	delete m_localICA;
	m_localICA = NULL;*/

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
			for( const auto& computer : MasterCore::i().computerManager().computerList() )
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




void MainWindow::stopDemoAfterRemoteControl()
{
	MasterCore::i().computerManager().setGlobalMode( Computer::ModeMonitoring );
}




void MainWindow::changeGlobalClientMode( int mode )
{
	Computer::Mode newMode = static_cast<Computer::Modes>( mode );
	ComputerManager& computerManager = MasterCore::i().computerManager();

	if( newMode == computerManager.globalMode() )
	{
		computerManager.setGlobalMode( Computer::ModeMonitoring );
		m_modeGroup->button( Computer::ModeMonitoring )->setChecked( true );
	}
	else
	{
		computerManager.setGlobalMode( newMode );
	}
}
