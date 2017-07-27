/*
 *  RemoteAccessWidget.cpp - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 *  This file is part of Veyon - http://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include "RemoteAccessWidget.h"
#include "VncView.h"
#include "VeyonCoreConnection.h"
#include "Computer.h"
#include "ComputerControlInterface.h"
#include "LocalSystem.h"
#include "ToolButton.h"
#include "Screenshot.h"

#include "rfb/rfbclient.h"

#include <math.h>

#include <QMenu>
#include <QtCore/QTimer>
#include <QtGui/QBitmap>
#include <QLayout>
#include <QtGui/QPainter>
#include <QPaintEvent>



// toolbar for remote-control-widget
RemoteAccessWidgetToolBar::RemoteAccessWidgetToolBar( RemoteAccessWidget* parrent, bool viewOnly ) :
	QWidget( parrent ),
	m_parent( parrent ),
	m_showHideTimeLine(),
	m_iconStateTimeLine(),
	m_connecting( false ),
	m_icon( QPixmap( ":/resources/icon128.png" ).scaled( QSize( 48, 48 ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) ),
	m_viewOnlyButton( new ToolButton( QPixmap( ":/remoteaccess/kmag.png" ), tr( "View only" ), tr( "Remote control" ) ) ),
	m_sendShortcutButton( new ToolButton( QPixmap( ":/remoteaccess/preferences-desktop-keyboard.png" ), tr( "Send shortcut" ) ) ),
	m_screenshotButton( new ToolButton( QPixmap( ":/remoteaccess/camera-photo.png" ), tr( "Screenshot" ) ) ),
	m_fullScreenButton( new ToolButton( QPixmap( ":/remoteaccess/view-fullscreen.png" ), tr( "Fullscreen" ), tr( "Window" ) ) ),
	m_quitButton( new ToolButton( QPixmap( ":/remoteaccess/application-exit.png" ), tr( "Quit" ) ) )
{
	QPalette pal = palette();
	pal.setBrush( QPalette::Window, QPixmap( ":/resources/toolbar-background.png" ) );
	setPalette( pal );

	setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, 0 );
	show();
	startConnection();

	m_viewOnlyButton->setCheckable( true );
	m_fullScreenButton->setCheckable( true );
	m_viewOnlyButton->setChecked( viewOnly );
	m_fullScreenButton->setChecked( false );

	connect( m_viewOnlyButton, &ToolButton::toggled, this, &RemoteAccessWidgetToolBar::updateControls );
	connect( m_viewOnlyButton, &QAbstractButton::toggled, parrent, &RemoteAccessWidget::toggleViewOnly );
	connect( m_fullScreenButton, &QAbstractButton::toggled, parrent, &RemoteAccessWidget::toggleFullScreen );
	connect( m_screenshotButton, &QAbstractButton::clicked, parrent, &RemoteAccessWidget::takeScreenshot );
	connect( m_quitButton, &QAbstractButton::clicked, parrent, &QWidget::close );

	auto vncView = parrent->m_vncView;

	auto shortcutMenu = new QMenu();
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
	connect( shortcutMenu->addAction( tr( "Ctrl+Alt+Del" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutCtrlAltDel ); } );
	connect( shortcutMenu->addAction( tr( "Ctrl+Esc" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutCtrlEscape ); } );
	connect( shortcutMenu->addAction( tr( "Alt+Tab" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutAltTab ); } );
	connect( shortcutMenu->addAction( tr( "Alt+F4" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutAltF4 ); } );
	connect( shortcutMenu->addAction( tr( "Win+Tab" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutWinTab ); } );
	connect( shortcutMenu->addAction( tr( "Win" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutWin ); } );
	connect( shortcutMenu->addAction( tr( "Menu" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutMenu ); } );
	connect( shortcutMenu->addAction( tr( "Alt+Ctrl+F1" ) ), &QAction::triggered,
				[=]() { vncView->sendShortcut( VncView::ShortcutAltCtrlF1 ); } );
#else
	shortcutMenu->addAction( tr( "Ctrl+Alt+Del" ), [=]() { vncView->sendShortcut( VncView::ShortcutCtrlAltDel ); }  );
	shortcutMenu->addAction( tr( "Ctrl+Esc" ), [=]() { vncView->sendShortcut( VncView::ShortcutCtrlEscape ); }  );
	shortcutMenu->addAction( tr( "Alt+Tab" ), [=]() { vncView->sendShortcut( VncView::ShortcutAltTab ); }  );
	shortcutMenu->addAction( tr( "Alt+F4" ), [=]() { vncView->sendShortcut( VncView::ShortcutAltF4 ); }  );
	shortcutMenu->addAction( tr( "Win+Tab" ), [=]() { vncView->sendShortcut( VncView::ShortcutWinTab ); }  );
	shortcutMenu->addAction( tr( "Win" ), [=]() { vncView->sendShortcut( VncView::ShortcutWin ); }  );
	shortcutMenu->addAction( tr( "Menu" ), [=]() { vncView->sendShortcut( VncView::ShortcutMenu ); }  );
	shortcutMenu->addAction( tr( "Alt+Ctrl+F1" ), [=]() { vncView->sendShortcut( VncView::ShortcutAltCtrlF1 ); }  );
#endif

	m_sendShortcutButton->setMenu( shortcutMenu );
	m_sendShortcutButton->setPopupMode( QToolButton::InstantPopup );

	auto layout = new QHBoxLayout( this );
	layout->setMargin( 1 );
	layout->setSpacing( 1 );
	layout->addStretch( 0 );
	layout->addWidget( m_sendShortcutButton );
	layout->addWidget( m_viewOnlyButton );
	layout->addWidget( m_screenshotButton );
	layout->addWidget( m_fullScreenButton );
	layout->addWidget( m_quitButton );
	layout->addSpacing( 5 );
	connect( m_parent->m_vncView, &VncView::startConnection, this, &RemoteAccessWidgetToolBar::startConnection );
	connect( m_parent->m_vncView, &VncView::connectionEstablished, this, &RemoteAccessWidgetToolBar::connectionEstablished );

	setFixedHeight( 52 );

	m_showHideTimeLine.setFrameRange( 0, height() );
	m_showHideTimeLine.setDuration( 800 );
	m_showHideTimeLine.setCurveShape( QTimeLine::EaseInCurve );
	connect( &m_showHideTimeLine, &QTimeLine::valueChanged, this, &RemoteAccessWidgetToolBar::updatePosition );

	m_iconStateTimeLine.setFrameRange( 0, 100 );
	m_iconStateTimeLine.setDuration( 1500 );
	m_iconStateTimeLine.setUpdateInterval( 60 );
	m_iconStateTimeLine.setCurveShape( QTimeLine::SineCurve );
	connect( &m_iconStateTimeLine, &QTimeLine::valueChanged, this, &RemoteAccessWidgetToolBar::updateConnectionAnimation );
	connect( &m_iconStateTimeLine, &QTimeLine::finished, &m_iconStateTimeLine, &QTimeLine::start );
}




RemoteAccessWidgetToolBar::~RemoteAccessWidgetToolBar()
{
}




void RemoteAccessWidgetToolBar::appear()
{
	m_showHideTimeLine.setDirection( QTimeLine::Backward );
	if( m_showHideTimeLine.state() != QTimeLine::Running )
	{
		m_showHideTimeLine.resume();
	}
}




void RemoteAccessWidgetToolBar::disappear()
{
	if( !m_connecting && !rect().contains( mapFromGlobal( QCursor::pos() ) ) )
	{
		if( m_showHideTimeLine.state() != QTimeLine::Running )
		{
			m_showHideTimeLine.setDirection( QTimeLine::Forward );
			m_showHideTimeLine.resume();
		}
	}
}



void RemoteAccessWidgetToolBar::updateControls( bool viewOnly )
{
	m_sendShortcutButton->setVisible( viewOnly == false );
}



void RemoteAccessWidgetToolBar::leaveEvent( QEvent *event )
{
	QTimer::singleShot( 500, this, SLOT( disappear() ) );
	QWidget::leaveEvent( event );
}




void RemoteAccessWidgetToolBar::paintEvent( QPaintEvent *paintEv )
{
	QPainter p( this );
	QFont f = p.font();

	p.setOpacity( 0.8-0.8*m_showHideTimeLine.currentValue() );
	p.fillRect( paintEv->rect(), palette().brush( QPalette::Window ) );
	p.setOpacity( 1 );

	p.drawPixmap( 5, 2, m_icon );

	f.setPointSize( 12 );
	f.setBold( true );
	p.setFont( f );

	//p.setPen( Qt::white );
	//p.drawText( 64, 22, m_parent->windowTitle() );

	p.setPen( QColor( 192, 192, 192 ) );
	f.setPointSize( 10 );
	p.setFont( f );

	if( m_connecting )
	{
		QString dots;
		for( int i = 0; i < ( m_iconStateTimeLine.currentTime() / 120 ) % 6; ++i )
		{
			dots += ".";
		}
		p.drawText( 64, 40, tr( "Connecting %1" ).arg( dots ) );
	}
	else
	{
		p.drawText( 64, 40, tr( "Connected." ) );
	}
}




void RemoteAccessWidgetToolBar::updateConnectionAnimation()
{
	repaint();
}




void RemoteAccessWidgetToolBar::updatePosition()
{
	const int newY = m_showHideTimeLine.currentFrame();
	if( newY != -y() )
	{
		move( x(), qMax( -height(), -newY ) );
	}
}




void RemoteAccessWidgetToolBar::startConnection()
{
	m_connecting = true;
	m_iconStateTimeLine.start();
	appear();
	update();
}




void RemoteAccessWidgetToolBar::connectionEstablished()
{
	m_connecting = false;
	m_iconStateTimeLine.stop();
	QTimer::singleShot( 3000, this, SLOT( disappear() ) );
	// within the next 1000ms the username should be known and therefore
	// we update
	QTimer::singleShot( 1000, this, SLOT( update() ) );
}





RemoteAccessWidget::RemoteAccessWidget( const ComputerControlInterface& computerControlInterface,
											bool viewOnly ) :
	QWidget( nullptr ),
	m_computerControlInterface( computerControlInterface ),
	m_vncView( new VncView( computerControlInterface.computer().hostAddress(), -1, this, VncView::RemoteControlMode ) ),
	m_coreConnection( new VeyonCoreConnection( m_vncView->vncConnection() ) ),
	m_toolBar( new RemoteAccessWidgetToolBar( this, viewOnly ) )
{
	setWindowTitle( tr( "%1 Remote Access" ).arg( VeyonCore::applicationName() ) );
	setWindowIcon( QPixmap( ":/remoteaccess/kmag.png" ) );
	setAttribute( Qt::WA_DeleteOnClose, true );

	m_vncView->move( 0, 0 );
	connect( m_vncView, SIGNAL( mouseAtTop() ), m_toolBar,
							SLOT( appear() ) );
	connect( m_vncView, SIGNAL( keyEvent( int, bool ) ),
				this, SLOT( checkKeyEvent( int, bool ) ) );
	connect( m_vncView, SIGNAL( sizeHintChanged() ),
					this, SLOT( updateSize() ) );

	showMaximized();
	LocalSystem::activateWindow( this );
	showNormal();

	move( 0, 0 );

	toggleViewOnly( viewOnly );
}




RemoteAccessWidget::~RemoteAccessWidget()
{
	delete m_coreConnection;
	delete m_vncView;
}



void RemoteAccessWidget::enterEvent( QEvent* event )
{
	QTimer::singleShot( 500, m_toolBar, SLOT( disappear() ) );
	QWidget::enterEvent( event );
}




void RemoteAccessWidget::leaveEvent( QEvent* event )
{
	m_toolBar->appear();
	QWidget::leaveEvent( event );
}




void RemoteAccessWidget::resizeEvent( QResizeEvent* event )
{
	m_vncView->resize( size() );
	m_toolBar->setFixedSize( width(), m_toolBar->height() );

	QWidget::resizeEvent( event );
}




void RemoteAccessWidget::checkKeyEvent( int key, bool pressed )
{
	if( pressed && key == XK_Escape && !m_coreConnection->isConnected() )
	{
		close();
	}
}



void RemoteAccessWidget::updateSize()
{
	if( !( windowState() & Qt::WindowFullScreen ) &&
			m_vncView->sizeHint().isEmpty() == false )
	{
		resize( m_vncView->sizeHint() );
	}
}



void RemoteAccessWidget::toggleFullScreen( bool _on )
{
	if( _on )
	{
		setWindowState( windowState() | Qt::WindowFullScreen );
	}
	else
	{
		setWindowState( windowState() & ~Qt::WindowFullScreen );
	}
}




void RemoteAccessWidget::toggleViewOnly( bool viewOnly )
{
	m_vncView->setViewOnly( viewOnly );
	m_toolBar->updateControls( viewOnly );
	m_toolBar->update();
}




void RemoteAccessWidget::takeScreenshot()
{
	Screenshot().take( m_computerControlInterface );
}


