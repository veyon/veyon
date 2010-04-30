/*
 *  RemoteControlWidget.cpp - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 *  This file is part of iTALC - http://italc.sourceforge.net
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


#include "RemoteControlWidget.h"
#include "VncView.h"
#include "ItalcCoreConnection.h"
#include "LocalSystem.h"
#include "ToolButton.h"
#include "MainWindow.h"

#include <math.h>

#include <QtCore/QTimer>
#include <QtGui/QBitmap>
#include <QtGui/QDesktopWidget>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>



// toolbar for remote-control-widget
RemoteControlWidgetToolBar::RemoteControlWidgetToolBar(
			RemoteControlWidget * _parent, bool _view_only ) :
	QWidget( _parent ),
	m_parent( _parent ),
	m_disappear( false ),
	m_connecting( false ),
	m_icon( FastQImage( QImage( ":/resources/logo.png" ) ).
					scaled( QSize( 48, 48 ) ) ),
	m_iconGray( FastQImage( m_icon ).toGray().darken( 50 ) ),
	m_iconState()
{
	QPalette pal = palette();
	pal.setBrush( QPalette::Window, QPixmap( ":/resources/toolbar-background.png" ) );
	setPalette( pal );

	setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, 0 );
	show();
	startConnection();

	ToolButton * vo_btn = new ToolButton(
				QPixmap( ":/resources/overview_mode.png" ),
				tr( "View only" ), tr( "Remote control" ),
				QString::null, QString::null, 0, 0,
				this );
	ToolButton * ls_btn = new ToolButton(
				QPixmap( ":/resources/no_mouse.png" ),
				tr( "Lock student" ), tr( "Unlock student" ),
				QString::null, QString::null, 0, 0,
				this );
	ToolButton * ss_btn = new ToolButton(
				QPixmap( ":/resources/snapshot.png" ),
				tr( "Snapshot" ), QString::null,
				QString::null, QString::null, 0, 0,
				this );
	ToolButton * fs_btn = new ToolButton(
				QPixmap( ":/resources/fullscreen.png" ),
				tr( "Fullscreen" ), tr( "Window" ),
				QString::null, QString::null, 0, 0,
				this );
	ToolButton * quit_btn = new ToolButton(
				QPixmap( ":/resources/quit.png" ),
				tr( "Quit" ), QString::null,
				QString::null, QString::null, 0, 0,
				this );
	vo_btn->setCheckable( true );
	ls_btn->setCheckable( true );
	fs_btn->setCheckable( true );
	vo_btn->setChecked( _view_only );
	fs_btn->setChecked( true );

	connect( vo_btn, SIGNAL( toggled( bool ) ),
				_parent, SLOT( toggleViewOnly( bool ) ) );
	connect( ls_btn, SIGNAL( toggled( bool ) ),
				_parent, SLOT( lockStudent( bool ) ) );
	connect( fs_btn, SIGNAL( toggled( bool ) ),
				_parent, SLOT( toggleFullScreen( bool ) ) );
	connect( ss_btn, SIGNAL( clicked() ), _parent, SLOT( takeSnapshot() ) );
	connect( quit_btn, SIGNAL( clicked() ), _parent, SLOT( close() ) );

	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 1 );
	layout->setSpacing( 1 );
	layout->addStretch( 0 );
	layout->addWidget( vo_btn );
	layout->addWidget( ls_btn );
	layout->addWidget( ss_btn );
	layout->addWidget( fs_btn );
	layout->addWidget( quit_btn );
	layout->addSpacing( 5 );
	connect( m_parent->m_vncView, SIGNAL( startConnection() ),
					this, SLOT( startConnection() ) );
	connect( m_parent->m_vncView, SIGNAL( connectionEstablished() ),
					this, SLOT( connectionEstablished() ) );
}




RemoteControlWidgetToolBar::~RemoteControlWidgetToolBar()
{
}




void RemoteControlWidgetToolBar::appear( void )
{
	m_disappear = false;
	if( y() <= -height() )
	{
		updatePosition();
	}
}




void RemoteControlWidgetToolBar::disappear( void )
{
	if( !m_connecting )
	{
		m_disappear = true;
		if( y() == 0 )
		{
			updatePosition();
		}
	}
}




void RemoteControlWidgetToolBar::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	p.fillRect( _pe->rect(), palette().brush( QPalette::Window ) );

	p.drawImage( 5, 2, m_icon );

	QFont f = p.font();
	f.setPointSize( 12 );
	f.setBold( true );
	p.setFont( f );

	p.setPen( QColor( 255, 212, 0 ) );
	m_parent->updateWindowTitle();
	p.drawText( 64, 22, m_parent->windowTitle() );

	p.setPen( QColor( 255, 255, 255 ) );
	f.setPointSize( 10 );
	p.setFont( f );

	if( m_connecting )
	{
		FastQImage tmp = m_iconGray;
		tmp.alphaFillMax( (int)( 150 + 90.0 *
				sin( m_iconState.elapsed()*3.141592/900 ) ) );
		p.drawImage( 5, 2, tmp );

		QString dots;
		for( int i = 0; i < ( m_iconState.elapsed() / 400 ) % 6;++i)
		{
			dots += ".";
		}
		p.drawText( 64, 40, tr( "Connecting %1" ).arg( dots ) );
		QTimer::singleShot( 50, this, SLOT( update() ) );
	}
	else
	{
		p.drawText( 64, 40, tr( "Connected." ) );
	}
}




void RemoteControlWidgetToolBar::updatePosition( void )
{
	bool again;
	if( m_disappear )
	{
		move( x(), qMax( -height(), y()-3 ) );
		again = y() > -height();
	}
	else
	{
		move( x(), qMin( 0, y()+3 ) );
		again = y() < 0;
	}

	update();

	if( again )
	{
		QTimer::singleShot( 15, this, SLOT( updatePosition() ) );
	}
}




void RemoteControlWidgetToolBar::startConnection( void )
{
	m_connecting = true;
	m_iconState.restart();
	appear();
	update();
}




void RemoteControlWidgetToolBar::connectionEstablished( void )
{
	m_connecting = false;
	QTimer::singleShot( 3000, this, SLOT( disappear() ) );
	// within the next 1000ms the username should be known and therefore
	// we update
	QTimer::singleShot( 1000, this, SLOT( update() ) );
}










RemoteControlWidget::RemoteControlWidget( const QString & _host,
						bool _view_only,
						MainWindow * _main_window ) :
	QWidget( 0 ),
	m_vncView( new VncView( _host, this, false ) ),
	m_icc( new ItalcCoreConnection( m_vncView->vncConnection() ) ),
	m_toolBar( new RemoteControlWidgetToolBar( this, _view_only ) ),
	m_mainWindow( _main_window ),
	m_extraStates( Qt::WindowMaximized )
{
	setWindowIcon( QPixmap( ":/resources/remote_control.png" ) );
	setAttribute( Qt::WA_DeleteOnClose, true );
	m_vncView->move( 0, 0 );
	connect( m_vncView, SIGNAL( mouseAtTop() ), m_toolBar,
							SLOT( appear() ) );
/*	connect( m_vncView, SIGNAL( keyEvent( Q_UINT32, bool ) ),
				this, SLOT( checkKeyEvent( Q_UINT32, bool ) ) );*/
//	showMaximized();
	//showFullScreen();
	show();
	LocalSystem::activateWindow( this );

	toggleViewOnly( _view_only );
}




RemoteControlWidget::~RemoteControlWidget()
{
}




QString RemoteControlWidget::host( void ) const
{
	return "blah";
/*	return( m_vncView->m_connection ? m_vncView->m_connection->host() :
								QString::null );*/
}




void RemoteControlWidget::updateWindowTitle( void )
{
	const QString s = m_vncView->viewOnly() ?
			tr( "View live (%1 at host %2)" )
		:
			tr( "Remote control (%1 at host %2)" );
	QString u = m_icc->user();// = m_vncView->m_connection->user();
	if( u.isEmpty() )
	{
		u = tr( "unknown user" );
	}
	else
	{
		u = u.section( '(', 1 ).section( ')', 0, 0 );
	}
	setWindowTitle( s.arg( u ).arg( host() ) );
}




void RemoteControlWidget::resizeEvent( QResizeEvent * )
{
	m_vncView->resize( size() );
	m_toolBar->setFixedSize( width(), 52 );
}




void RemoteControlWidget::checkKeyEvent( Q_UINT32 _key, bool _pressed )
{
	if( _pressed && _key == XK_Escape/* &&
		m_vncView->m_connection->state() != ivsConnection::Connected*/ )
	{
		close();
	}
}




void RemoteControlWidget::lockStudent( bool _on )
{
	//m_vncView->m_connection->disableLocalInputs( _on );
}




void RemoteControlWidget::toggleFullScreen( bool _on )
{
	if( _on )
	{
		setWindowState( windowState() | Qt::WindowFullScreen );
	}
	else
	{
		setWindowState( windowState() & ~Qt::WindowFullScreen );
		setWindowState( windowState() | m_extraStates );
		m_extraStates = Qt::WindowNoState;
	}
}




void RemoteControlWidget::toggleViewOnly( bool _on )
{
	m_vncView->setViewOnly( _on );
	m_toolBar->update();
}




void RemoteControlWidget::takeSnapshot( void )
{
/*	m_vncView->m_connection->takeSnapshot();
	if( m_mainWindow )
	{
		m_mainWindow->reloadSnapshotList();
	}*/
}


