/*
 * main_window.cpp - main-file for iTALC-Application
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCloseEvent>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QSplashScreen>
#include <QtGui/QSplitter>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QWorkspace>
#include <QtNetwork/QHostAddress>

#include "main_window.h"
#include "classroom_manager.h"
#include "dialogs.h"
#include "italc_side_bar.h"
#include "overview_widget.h"
#include "user_list.h"
#include "snapshot_list.h"
#include "config_widget.h"
#include "support_widget.h"
#include "messagebox.h"
#include "tool_button.h"
#include "tool_bar.h"
#include "isd_connection.h"
#include "local_system.h"
#include "remote_control_widget.h"



bool mainWindow::ensureConfigPathExists( void )
{
/*	const QString d = localSystem::personalConfigDir();
	if( !QDir( d ).exists() )
	{
		QDir::home().rmpath( d );
		return( QDir::home().mkpath( QDir( d ).dirName() ) );
	}
	return( TRUE );*/
	return( localSystem::ensurePathExists(
					localSystem::personalConfigDir() ) );
}


bool mainWindow::s_atExit = FALSE;


mainWindow::mainWindow() :
	QMainWindow(/* 0, Qt::FramelessWindowHint*/ ),
	m_openedTabInSideBar( 1 ),
	m_localISD( NULL ),
	m_rctrlLock(),
	m_remoteControlWidget( NULL )
{
	setWindowTitle( tr( "iTALC" ) + " " + VERSION );
	setWindowIcon( QPixmap( ":/resources/logo.png" ) );

	if( mainWindow::ensureConfigPathExists() == FALSE )
	{
		if( splashScreen != NULL )
		{
			splashScreen->hide();
		}
		messageBox::information( tr( "No write-access" ),
			tr( "Could not read/write or create directory %1! "
			"For running iTALC, make sure you're permitted to "
			"create or write this directory." ).arg(
					localSystem::personalConfigDir() ) );
		return;
	}

	QWidget * hbox = new QWidget( this );
	QHBoxLayout * hbox_layout = new QHBoxLayout( hbox );
	hbox_layout->setMargin( 0 );

	// create splitter, which is used for splitting sidebar-workspaces
	// from main-workspace
	m_splitter = new QSplitter( Qt::Horizontal, hbox );
#if QT_VERSION >= 0x030200
	m_splitter->setChildrenCollapsible( FALSE );
#endif

	// create sidebar
	m_sideBar = new italcSideBar( italcSideBar::Vertical, hbox,
								m_splitter );
	m_sideBar->setStyle( italcSideBar::VSNET/*KDEV3ICON*/ );



	m_workspace = new QWorkspace( m_splitter );
	m_workspace->setScrollBarsEnabled( TRUE );


	QWidget * twp = m_sideBar->tabWidgetParent();
	// now create all sidebar-workspaces
	m_overviewWidget = new overviewWidget( this, twp );
	m_classroomManager = new classroomManager( this, twp );
	m_userList = new userList( this, twp );
	m_snapshotList = new snapshotList( this, twp );
	m_configWidget = new configWidget( this, twp );
	m_supportWidget = new supportWidget( this, twp );

	// append sidebar-workspaces to sidebar
	int id = 0;
	m_sideBar->appendTab( m_overviewWidget, ++id );
	m_sideBar->appendTab( m_classroomManager, ++id );
	m_sideBar->appendTab( m_userList, ++id );
	m_sideBar->appendTab( m_snapshotList, ++id );
	m_sideBar->appendTab( m_configWidget, ++id );
	m_sideBar->appendTab( m_supportWidget, ++id );
	m_sideBar->setPosition( italcSideBar::Left );
	m_sideBar->setTab( m_openedTabInSideBar, TRUE );

	setCentralWidget( hbox );
	hbox_layout->addWidget( m_sideBar );
	hbox_layout->addWidget( m_splitter );




	// create the action-toolbar
	m_toolBar = new toolBar( tr( "Actions" ), this );
	m_toolBar->setMovable( FALSE );
	m_toolBar->setObjectName( "maintoolbar" );
	m_toolBar->toggleViewAction()->setEnabled( FALSE );

	addToolBar( Qt::TopToolBarArea, m_toolBar );

	toolButton * scr = new toolButton(
			QPixmap( ":/resources/tutorials.png" ),
			tr( "Classroom" ), QString::null,
			tr( "Switch classroom" ),
			tr( "Click this button to open a menu where you can "
				"choose the active classroom." ),
			NULL, NULL, m_toolBar );
	scr->setMenu( m_classroomManager->quickSwitchMenu() );
	scr->setPopupMode( toolButton::InstantPopup );
	scr->setWhatsThis( tr( "Click on this button, to switch between "
							"classrooms." ) );

	m_modeGroup = new QButtonGroup( this );

	toolButton * overview_mode = new toolButton(
			QPixmap( ":/resources/overview_mode.png" ),
			tr( "Overview" ), QString::null,
			tr( "Overview mode" ),
			tr( "This is the default mode in iTALC and allows you "
				"to have an overview over all visible "
				"computers. Also click on this button for "
				"unlocking locked workstations or for leaving "
				"demo-mode." ),
			NULL, NULL, m_toolBar );

	toolButton * fsdemo_mode = new toolButton(
			QPixmap( ":/resources/fullscreen_demo.png" ),
			tr( "Demo on" ), tr( "Demo off" ),
			tr( "Fullscreen demo" ),
			tr( "In this mode your screen is being displayed on "
				"all shown computers. Furthermore the users "
				"aren't able to do something else as all input "
				"devices are locked in this mode." ),
			NULL, NULL, m_toolBar );

	toolButton * windemo_mode = new toolButton(
			QPixmap( ":/resources/window_demo.png" ),
			tr( "Demo/window on" ), tr( "Demo/window off" ),
			tr( "Window demo" ),
			tr( "In this mode your screen being displayed in a "
				"window on all shown computers. The users are "
				"able to switch to other windows and thus "
				"can continue to work." ),
			NULL, NULL, m_toolBar );

	toolButton * lock_mode = new toolButton(
			QPixmap( ":/resources/locked.png" ),
			tr( "Lock all" ), tr( "Unlock all" ),
			tr( "Lock/unlock desktops" ),
			tr( "To have all user's full attention you can lock "
				"their desktops using this button. "
				"In this mode all input devices are locked and "
				"the screen is black." ),
			NULL, NULL, m_toolBar );

	overview_mode->setCheckable( TRUE );
	fsdemo_mode->setCheckable( TRUE );
	windemo_mode->setCheckable( TRUE );
	lock_mode->setCheckable( TRUE );

	m_modeGroup->addButton( overview_mode, client::Mode_Overview );
	m_modeGroup->addButton( fsdemo_mode, client::Mode_FullscreenDemo );
	m_modeGroup->addButton( windemo_mode, client::Mode_WindowDemo );
	m_modeGroup->addButton( lock_mode, client::Mode_Locked );

	overview_mode->setChecked( TRUE );
	connect( m_modeGroup, SIGNAL( buttonClicked( int ) ),
			this, SLOT( changeGlobalClientMode( int ) ) );



	toolButton * text_msg = new toolButton(
			QPixmap( ":/resources/text_message.png" ),
			tr( "Text message" ), QString::null,
			tr( "Send text message" ),
			tr( "Use this button to send a text message to all "
				"users e.g. to tell them new tasks etc." ),
			m_classroomManager, SLOT( sendMessage() ), m_toolBar );


	toolButton * power_on = new toolButton(
			QPixmap( ":/resources/power_on.png" ),
			tr( "Power on" ), QString::null,
			tr( "Power on computers" ),
			tr( "Click this button to power on all visible "
				"computers. This way you do not have to turn "
				"on each computer by hand." ),
			m_classroomManager, SLOT( powerOnClients() ), m_toolBar );

	toolButton * power_off = new toolButton(
			QPixmap( ":/resources/power_off.png" ),
			tr( "Power down" ), QString::null,
			tr( "Power down computers" ),
			tr( "To power down all shown computers (e.g. after "
				"the lesson has finished) you can click this "
				"button." ),
			m_classroomManager,
					SLOT( powerDownClients() ), m_toolBar );

	toolButton * multilogon = new toolButton(
			QPixmap( ":/resources/multilogon.png" ),
			tr( "Logon" ), QString::null,
			tr( "Multi logon" ),
			tr( "After clicking this button you can enter a "
				"username and password for logging in the "
				"according user on all visible computers." ),
			m_classroomManager, SLOT( multiLogon() ), m_toolBar );


	toolButton * adjust_size = new toolButton(
			QPixmap( ":/resources/adjust_size.png" ),
			tr( "Adjust/align" ), QString::null,
			tr( "Adjust windows and their size" ),
			tr( "When clicking this button the biggest possible "
				"size for the client-windows is adjusted. "
				"Furthermore all windows are aligned." ),
			m_classroomManager, SLOT( adjustWindows() ), m_toolBar );

	toolButton * auto_arrange = new toolButton(
			QPixmap( ":/resources/auto_arrange.png" ),
			tr( "Auto view" ), QString::null,
			tr( "Auto re-arrange windows and their size" ),
			tr( "When clicking this button all visible windows "
				"are re-arranged and adjusted." ),
			m_classroomManager, SLOT( arrangeWindows() ), m_toolBar );

	m_toolBar->addWidget( scr );
	m_toolBar->addWidget( overview_mode );
	m_toolBar->addWidget( fsdemo_mode );
	m_toolBar->addWidget( windemo_mode );
	m_toolBar->addWidget( lock_mode );
	m_toolBar->addWidget( text_msg );
	m_toolBar->addWidget( power_on );
	m_toolBar->addWidget( power_off );
	m_toolBar->addWidget( multilogon );
	m_toolBar->addWidget( adjust_size );
	m_toolBar->addWidget( auto_arrange );

	restoreState( QByteArray::fromBase64(
				m_classroomManager->winCfg().toAscii() ) );

	if( isdConnection::initAuthentication() == FALSE )
	{
		if( splashScreen != NULL )
		{
			splashScreen->hide();
		}
		messageBox::information( tr( "No valid keys found" ),
			tr( 	"No authentication-keys were found or your "
				"old ones were broken. Please create a new "
				"key-pair using ICA (see documentation at "
		"http://italc.sf.net/wiki/index.php?title=Installation).\n"
				"Otherwise you won't be able to access "
						"computers using iTALC." ) );
	}

	m_localISD = new isdConnection( QHostAddress( QHostAddress::LocalHost
								).toString() );
	if( m_localISD->open() != isdConnection::Connected )
	{
		messageBox::information( tr( "iTALC service not running" ),
			tr( 	"There seems to be no iTALC service running "
				"on this computer or the authentication-keys "
				"aren't set up properly. The service is "
				"required for running iTALC. Contact your "
				"administrator for solving this problem." ),
					QPixmap( ":/resources/stop.png" ) );
		return;
	}

	m_localISD->setRole( __role );
	m_localISD->demoServerStop();
	m_localISD->demoServerRun( __demo_quality, localSystem::freePort() );

	QTimer::singleShot( 2000, m_classroomManager, SLOT( updateClients() ) );

	m_updateThread = new updateThread( this );
}




mainWindow::~mainWindow()
{
	m_classroomManager->doCleanupWork();
	delete m_workspace;


	m_localISD->demoServerStop();
	delete m_localISD;
	m_localISD = NULL;
}




void mainWindow::keyPressEvent( QKeyEvent * _e )
{
	if( _e->key() == Qt::Key_F11 )
	{
		QWidget::setWindowState( QWidget::windowState() ^
							Qt::WindowFullScreen );
	}
	else
	{
		QMainWindow::keyPressEvent( _e );
	}
}




void mainWindow::closeEvent( QCloseEvent * _ce )
{
	s_atExit = TRUE;

	m_updateThread->terminate();
	m_updateThread->wait();

	m_classroomManager->savePersonalConfig();
	m_classroomManager->saveGlobalClientConfig();

	_ce->accept();
	deleteLater();
}



void mainWindow::remoteControlDisplay( const QString & _ip, bool _view_only )
{
	QWriteLocker wl( &m_rctrlLock );
	if( m_remoteControlWidget  )
	{
		return;
	}
	m_remoteControlWidget = new remoteControlWidget( _ip, _view_only, this );
	connect( m_remoteControlWidget, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( remoteControlWidgetClosed( QObject * ) ) );
}




void mainWindow::remoteControlWidgetClosed( QObject * )
{
	m_rctrlLock.lockForWrite();
        m_remoteControlWidget = NULL;
	m_rctrlLock.unlock();
}




void mainWindow::aboutITALC( void )
{
	aboutDialog( this ).exec();
}




void mainWindow::changeGlobalClientMode( int _mode )
{
	client::modes new_mode = static_cast<client::modes>( _mode );
	if( new_mode == m_classroomManager->globalClientMode()/* &&
					new_mode != client::Mode_Overview*/ )
	{
		m_classroomManager->changeGlobalClientMode(
							client::Mode_Overview );
		m_modeGroup->button( client::Mode_Overview )->setChecked(
									TRUE );
	}
	else
	{
		m_classroomManager->changeGlobalClientMode( _mode );
	}
}







mainWindow::updateThread::updateThread( mainWindow * _main_window ) :
	QThread(),
	m_mainWindow( _main_window )
{
	start( QThread::LowestPriority );
}




void mainWindow::updateThread::run( void )
{
	while( 1 )
	{
		m_mainWindow->m_localISD->handleServerMessages();

		m_mainWindow->m_userList->reload();

		if( client::reloadSnapshotList() )
		{
			m_mainWindow->m_snapshotList->reloadList();
		}
		client::resetReloadOfSnapshotList();

		// now sleep before reloading clients again
		QThread::sleep(
			m_mainWindow->getClassroomManager()->updateInterval() );

		// now do cleanup-work
		m_mainWindow->getClassroomManager()->doCleanupWork();
	}
}




#include "main_window.moc"
#include "italc_side_bar.moc"
