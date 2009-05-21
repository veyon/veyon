/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QScrollArea>
#include <QtGui/QSplashScreen>
#include <QtGui/QSplitter>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QWorkspace>
#include <QtNetwork/QHostAddress>

#include "MainWindow.h"
#include "ClassroomManager.h"
#include "Dialogs.h"
#include "ItalcSideBar.h"
#include "OverviewWidget.h"
#include "SnapshotList.h"
#include "ConfigWidget.h"
#include "DecoratedMessageBox.h"
#include "ToolButton.h"
#include "ToolBar.h"
#include "ItalcCoreConnection.h"
#include "ItalcVncConnection.h"
#include "LocalSystem.h"
#include "RemoteControlWidget.h"


QSystemTrayIcon * __systray_icon = NULL;


extern int __ivs_port;
extern QString __ivs_host;


bool MainWindow::ensureConfigPathExists( void )
{
	return( LocalSystem::ensurePathExists(
					LocalSystem::personalConfigDir() ) );
}


bool MainWindow::s_atExit = FALSE;


MainWindow::MainWindow( int _rctrl_screen ) :
	QMainWindow(/* 0, Qt::FramelessWindowHint*/ ),
	m_openedTabInSideBar( 1 ),
	m_localISD( NULL ),
	m_rctrlLock(),
	m_remoteControlWidget( NULL ),
	m_stopDemo( FALSE ),
	m_remoteControlScreen( _rctrl_screen > -1 ?
				qMin( _rctrl_screen,
					QApplication::desktop()->numScreens() )
				:
				QApplication::desktop()->screenNumber( this ) )
{
	setWindowTitle( tr( "iTALC" ) + " " + ITALC_VERSION );
	setWindowIcon( QPixmap( ":/resources/logo.png" ) );

	if( MainWindow::ensureConfigPathExists() == FALSE )
	{
		if( splashScreen != NULL )
		{
			splashScreen->hide();
		}
		DecoratedMessageBox::information( tr( "No write-access" ),
			tr( "Could not read/write or create directory %1! "
			"For running iTALC, make sure you're permitted to "
			"create or write this directory." ).arg(
					LocalSystem::personalConfigDir() ) );
		return;
	}

	QWidget * hbox = new QWidget( this );
	QHBoxLayout * hbox_layout = new QHBoxLayout( hbox );
	hbox_layout->setMargin( 0 );
	hbox_layout->setSpacing( 0 );

	// create splitter, which is used for splitting sidebar-workspaces
	// from main-workspace
	m_splitter = new QSplitter( Qt::Horizontal, hbox );
#if QT_VERSION >= 0x030200
	m_splitter->setChildrenCollapsible( FALSE );
#endif

	// create sidebar
	m_sideBar = new ItalcSideBar( ItalcSideBar::VSNET, hbox, m_splitter );



	QScrollArea * sa = new QScrollArea( m_splitter );
	sa->setBackgroundRole( QPalette::Dark );
	sa->setFrameStyle( QFrame::NoFrame );
	m_splitter->setStretchFactor( m_splitter->indexOf( sa ), 10 );
	m_workspace = new clientWorkspace( sa );


	QWidget * twp = m_sideBar->tabWidgetParent();
	// now create all sidebar-workspaces
	m_overviewWidget = new OverviewWidget( this, twp );
	m_classroomManager = new ClassroomManager( this, twp );
	m_snapshotList = new SnapshotList( this, twp );
	m_configWidget = new ConfigWidget( this, twp );

	m_workspace->m_contextMenu = m_classroomManager->quickSwitchMenu();

	// append sidebar-workspaces to sidebar
	int id = 0;
	m_sideBar->appendTab( m_overviewWidget, ++id );
	m_sideBar->appendTab( m_classroomManager, ++id );
	m_sideBar->appendTab( m_snapshotList, ++id );
	m_sideBar->appendTab( m_configWidget, ++id );
	m_sideBar->setPosition( ItalcSideBar::Left );
	m_sideBar->setTab( m_openedTabInSideBar, TRUE );

	setCentralWidget( hbox );
	hbox_layout->addWidget( m_sideBar );
	hbox_layout->addWidget( m_splitter );




	// create the action-toolbar
	m_toolBar = new ToolBar( tr( "Actions" ), this );
	m_toolBar->layout()->setSpacing( 4 );
	m_toolBar->setMovable( FALSE );
	m_toolBar->setObjectName( "maintoolbar" );
	m_toolBar->toggleViewAction()->setEnabled( FALSE );

	addToolBar( Qt::TopToolBarArea, m_toolBar );

	ToolButton * scr = new ToolButton(
			QPixmap( ":/resources/classroom.png" ),
			tr( "Classroom" ), QString::null,
			tr( "Switch classroom" ),
			tr( "Click this button to open a menu where you can "
				"choose the active classroom." ),
			NULL, NULL, m_toolBar );
	scr->setMenu( m_classroomManager->quickSwitchMenu() );
	scr->setPopupMode( ToolButton::InstantPopup );
	scr->setWhatsThis( tr( "Click on this button, to switch between "
							"classrooms." ) );

	m_modeGroup = new QButtonGroup( this );

	QAction * a;

	a = new QAction( QIcon( ":/resources/overview_mode.png" ),
						tr( "Overview mode" ), this );
	m_sysTrayActions << a;
	ToolButton * overview_mode = new ToolButton(
			a, tr( "Overview" ), QString::null,
			tr( "This is the default mode in iTALC and allows you "
				"to have an overview over all visible "
				"computers. Also click on this button for "
				"unlocking locked workstations or for leaving "
				"demo-mode." ),
			this, SLOT( mapOverview() ), m_toolBar );


	a = new QAction( QIcon( ":/resources/fullscreen_demo.png" ),
						tr( "Fullscreen demo" ), this );
	m_sysTrayActions << a;
	ToolButton * fsdemo_mode = new ToolButton(
			a, tr( "Fullscreen Demo" ), tr( "Stop Demo" ),
			tr( "In this mode your screen is being displayed on "
				"all shown computers. Furthermore the users "
				"aren't able to do something else as all input "
				"devices are locked in this mode." ),
			this, SLOT( mapFullscreenDemo() ), m_toolBar );

	a = new QAction( QIcon( ":/resources/window_demo.png" ),
						tr( "Window demo" ), this );
	m_sysTrayActions << a;
	ToolButton * windemo_mode = new ToolButton(
			a, tr( "Window Demo" ), tr( "Stop Demo" ),
			tr( "In this mode your screen being displayed in a "
				"window on all shown computers. The users are "
				"able to switch to other windows and thus "
				"can continue to work." ),
			this, SLOT( mapWindowDemo() ), m_toolBar );

	a = new QAction( QIcon( ":/resources/locked.png" ),
					tr( "Lock/unlock desktops" ), this );
	m_sysTrayActions << a;
	ToolButton * lock_mode = new ToolButton(
			a, tr( "Lock all" ), tr( "Unlock all" ),
			tr( "To have all user's full attention you can lock "
				"their desktops using this button. "
				"In this mode all input devices are locked and "
				"the screen is black." ),
			this, SLOT( mapScreenLock() ), m_toolBar );

	overview_mode->setCheckable( TRUE );
	fsdemo_mode->setCheckable( TRUE );
	windemo_mode->setCheckable( TRUE );
	lock_mode->setCheckable( TRUE );

	m_modeGroup->addButton( overview_mode, Client::Mode_Overview );
	m_modeGroup->addButton( fsdemo_mode, Client::Mode_FullscreenDemo );
	m_modeGroup->addButton( windemo_mode, Client::Mode_WindowDemo );
	m_modeGroup->addButton( lock_mode, Client::Mode_Locked );

	overview_mode->setChecked( TRUE );



	a = new QAction( QIcon( ":/resources/text_message.png" ),
					tr( "Send text message" ), this );
//	m_sysTrayActions << a;
	ToolButton * text_msg = new ToolButton(
			a, tr( "Text message" ), QString::null,
			tr( "Use this button to send a text message to all "
				"users e.g. to tell them new tasks etc." ),
			m_classroomManager, SLOT( sendMessage() ), m_toolBar );


	a = new QAction( QIcon( ":/resources/power_on.png" ),
					tr( "Power on computers" ), this );
	m_sysTrayActions << a;
	ToolButton * power_on = new ToolButton(
			a, tr( "Power on" ), QString::null,
			tr( "Click this button to power on all visible "
				"computers. This way you do not have to turn "
				"on each computer by hand." ),
			m_classroomManager, SLOT( powerOnClients() ),
								m_toolBar );

	a = new QAction( QIcon( ":/resources/power_off.png" ),
					tr( "Power down computers" ), this );
	m_sysTrayActions << a;
	ToolButton * power_off = new ToolButton(
			a, tr( "Power down" ), QString::null,
			tr( "To power down all shown computers (e.g. after "
				"the lesson has finished) you can click this "
				"button." ),
			m_classroomManager,
					SLOT( powerDownClients() ), m_toolBar );

	ToolButton * remotelogon = new ToolButton(
			QPixmap( ":/resources/remotelogon.png" ),
			tr( "Logon" ), QString::null,
			tr( "Remote logon" ),
			tr( "After clicking this button you can enter a "
				"username and password to log on the "
				"according user on all visible computers." ),
			m_classroomManager, SLOT( remoteLogon() ), m_toolBar );

	ToolButton * directsupport = new ToolButton(
			QPixmap( ":/resources/remote_control.png" ),
			tr( "Support" ), QString::null,
			tr( "Direct support" ),
			tr( "If you need to support someone at a certain "
				"computer you can click this button and enter "
				"the according hostname or IP afterwards." ),
			m_classroomManager, SLOT( directSupport() ), m_toolBar );

	ToolButton * adjust_size = new ToolButton(
			QPixmap( ":/resources/adjust_size.png" ),
			tr( "Adjust/align" ), QString::null,
			tr( "Adjust windows and their size" ),
			tr( "When clicking this button the biggest possible "
				"size for the client-windows is adjusted. "
				"Furthermore all windows are aligned." ),
			m_classroomManager, SLOT( adjustWindows() ), m_toolBar );

	ToolButton * auto_arrange = new ToolButton(
			QPixmap( ":/resources/auto_arrange.png" ),
			tr( "Auto view" ), QString::null,
			tr( "Auto re-arrange windows and their size" ),
			tr( "When clicking this button all visible windows "
					"are re-arranged and adjusted." ),
			NULL, NULL, m_toolBar );
	auto_arrange->setCheckable( true );
	auto_arrange->setChecked( m_classroomManager->isAutoArranged() );
	connect( auto_arrange, SIGNAL( toggled( bool ) ), m_classroomManager,
						 SLOT( arrangeWindowsToggle ( bool ) ) );

	scr->addTo( m_toolBar );
	overview_mode->addTo( m_toolBar );
	fsdemo_mode->addTo( m_toolBar );
	windemo_mode->addTo( m_toolBar );
	lock_mode->addTo( m_toolBar );
	text_msg->addTo( m_toolBar );
	power_on->addTo( m_toolBar );
	power_off->addTo( m_toolBar );
	remotelogon->addTo( m_toolBar );
	directsupport->addTo( m_toolBar );
	adjust_size->addTo( m_toolBar );
	auto_arrange->addTo( m_toolBar );

	restoreState( QByteArray::fromBase64(
				m_classroomManager->winCfg().toUtf8() ) );
	QStringList hidden_buttons = m_classroomManager->toolBarCfg().
								split( '#' );
	foreach( QAction * a, m_toolBar->actions() )
	{
		if( hidden_buttons.contains( a->text() ) )
		{
			a->setVisible( FALSE );
		}
	}

	foreach( KMultiTabBarTab * tab, m_sideBar->tabs() )
	{
		if( hidden_buttons.contains( tab->text() ) )
		{
			tab->setTabVisible( FALSE );
		}
	}

	while( 1 )
	{
		if( ItalcCore::initAuthentication() == FALSE )
		{
			if( ItalcCore::role != ItalcCore::RoleTeacher )
			{
				ItalcCore::role = ItalcCore::RoleTeacher;
				continue;
			}
			if( splashScreen != NULL )
			{
				splashScreen->hide();
			}
			DecoratedMessageBox::information(
				tr( "No valid keys found" ),
			tr( 	"No authentication-keys were found or your "
				"old ones were broken. Please create a new "
				"key-pair using ICA (see documentation at "
		"http://italc.sf.net/wiki/index.php?title=Installation).\n"
				"Otherwise you won't be able to access "
						"computers using iTALC." ) );
		}
		break;
	}

	ItalcVncConnection * conn = new ItalcVncConnection( this );
	conn->setHost( QHostAddress( __ivs_host ).toString() );
	conn->setPort( __ivs_port );
	conn->start();

	m_localISD = new ItalcCoreConnection( conn );
	if( 0 )//##ITALC2: m_localISD->open() != isdConnection::Connected )
	{
		DecoratedMessageBox::information(
			tr( "iTALC service not running" ),
			tr( 	"There seems to be no iTALC service running "
				"on this computer or the authentication-keys "
				"aren't set up properly. The service is "
				"required for running iTALC. Contact your "
				"administrator for solving this problem." ),
				QPixmap( ":/resources/stop.png" ) );
		return;
	}

/*	##ITALC2: m_localISD->demoServerRun( __demo_quality,
						LocalSystem::freePort( 5858 ) );*/


//	##ITALC2: m_localISD->hideTrayIcon();

	QIcon icon( ":/resources/icon16.png" );
	icon.addFile( ":/resources/icon22.png" );
	icon.addFile( ":/resources/icon32.png" );

	__systray_icon = new QSystemTrayIcon( icon, this );
	__systray_icon->setToolTip( tr( "iTALC Master Control" ) );
	__systray_icon->show();
	connect( __systray_icon, SIGNAL( activated(
					QSystemTrayIcon::ActivationReason ) ),
		this, SLOT( handleSystemTrayEvent(
					QSystemTrayIcon::ActivationReason ) ) );


	QTimer::singleShot( 2000, m_classroomManager, SLOT( updateClients() ) );

	m_updateThread = new mainWindowUpdateThread( this );
}




MainWindow::~MainWindow()
{
	m_classroomManager->doCleanupWork();

#ifdef BUILD_WIN32
	qApp->processEvents( QEventLoop::AllEvents, 3000 );
	LocalSystem::sleep( 3000 );
#endif

	// also delets clients
	delete m_workspace;

	delete m_localISD->vncConnection();
	delete m_localISD;
	m_localISD = NULL;

	__systray_icon->hide();
	delete __systray_icon;

#ifdef BUILD_WIN32
	qApp->processEvents( QEventLoop::AllEvents, 3000 );
	LocalSystem::sleep( 3000 );
	exit( 0 );
#endif
}




void MainWindow::keyPressEvent( QKeyEvent * _e )
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




void MainWindow::closeEvent( QCloseEvent * _ce )
{
	s_atExit = TRUE;

	m_updateThread->quit();
	m_updateThread->wait();
	delete m_updateThread;
	m_updateThread = NULL;

	/*ÜQList<Client *> clients = m_workspace->findChildren<Client *>();
	foreach( Client * c, clients )
	{
		c->quit();
	}*/

	m_classroomManager->savePersonalConfig();
	m_classroomManager->saveGlobalClientConfig();

	_ce->accept();
	deleteLater();
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
			m.addAction( __systray_icon->toolTip() )->setEnabled( FALSE );
			foreach( QAction * a, m_sysTrayActions )
			{
				m.addAction( a );
			}

			m.addSeparator();

			QMenu rcm( this );
			QAction * rc = m.addAction( tr( "Remote control" ) );
			rc->setMenu( &rcm );
			foreach( Client * c,
					m_classroomManager->visibleClients() )
			{
				rcm.addAction( c->name() )->
						setData( c->hostname() );
			}
			connect( &rcm, SIGNAL( triggered( QAction * ) ),
				this,
				SLOT( remoteControlClient( QAction * ) ) );

			m.addSeparator();

			QAction * qa = m.addAction(
					QIcon( ":/resources/quit.png" ),
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




void MainWindow::remoteControlClient( QAction * _a )
{
	show();
	remoteControlDisplay( _a->data().toString(),
				m_classroomManager->clientDblClickAction() );
}




void MainWindow::remoteControlDisplay( const QString & _hostname,
						bool _view_only,
						bool _stop_demo_afterwards )
{
	QWriteLocker wl( &m_rctrlLock );
	if( m_remoteControlWidget  )
	{
		return;
	}
	m_remoteControlWidget = new RemoteControlWidget( _hostname, _view_only,
									this );
	int x = 0;
	for( int i = 0; i < m_remoteControlScreen; ++i )
	{
		x += QApplication::desktop()->screenGeometry( i ).width();
	}
	m_remoteControlWidget->move( x, 0 );
	m_stopDemo = _stop_demo_afterwards;
	connect( m_remoteControlWidget, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( remoteControlWidgetClosed( QObject * ) ) );
}




void MainWindow::remoteControlWidgetClosed( QObject * )
{
	m_rctrlLock.lockForWrite();
	m_remoteControlWidget = NULL;
	m_rctrlLock.unlock();
	if( m_stopDemo )
	{
		m_classroomManager->changeGlobalClientMode(
							Client::Mode_Overview );
		m_stopDemo = FALSE;
	}
}




void MainWindow::aboutITALC( void )
{
	AboutDialog( this ).exec();
}




void MainWindow::changeGlobalClientMode( int _mode )
{
	Client::Modes new_mode = static_cast<Client::Modes>( _mode );
	if( new_mode == m_classroomManager->globalClientMode()/* &&
					new_mode != Client::Mode_Overview*/ )
	{
		m_classroomManager->changeGlobalClientMode(
							Client::Mode_Overview );
		m_modeGroup->button( Client::Mode_Overview )->setChecked(
									TRUE );
	}
	else
	{
		m_classroomManager->changeGlobalClientMode( _mode );
	}
}







mainWindowUpdateThread::mainWindowUpdateThread( MainWindow * _main_window ) :
	QThread(),
	m_mainWindow( _main_window )
{
	start( QThread::LowestPriority );
}




void mainWindowUpdateThread::update( void )
{
	if( Client::reloadSnapshotList() )
	{
		m_mainWindow->m_snapshotList->reloadList();
	}
	Client::resetReloadOfSnapshotList();

	// now do cleanup-work
	m_mainWindow->getClassroomManager()->doCleanupWork();
}


void mainWindowUpdateThread::run( void )
{
	QTimer t;
	connect( &t, SIGNAL( timeout() ), this, SLOT( update() ) );
	t.start( m_mainWindow->getClassroomManager()->updateInterval() * 1000 );
	exec();
}





clientWorkspace::clientWorkspace( QScrollArea * _parent ) :
	QWidget( _parent ),
	m_contextMenu( NULL )
{
	_parent->setWidget( this );
	_parent->setWidgetResizable( TRUE );
	setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
					QSizePolicy::MinimumExpanding ) );
	show();

}



QSize clientWorkspace::sizeHint( void ) const
{
	return( childrenRect().size() );
}




void clientWorkspace::contextMenuEvent( QContextMenuEvent * _event )
{
	m_contextMenu->exec( _event->globalPos() );
	_event->accept();
}



