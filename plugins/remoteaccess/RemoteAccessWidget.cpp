/*
 *  RemoteAccessWidget.cpp - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "RemoteAccessWidget.h"
#include "VncView.h"
#include "ItalcCoreConnection.h"
#include "Computer.h"
#include "ComputerControlInterface.h"
#include "LocalSystem.h"
#include "ToolButton.h"
#include "Snapshot.h"

#include <math.h>

#include <QtCore/QTimer>
#include <QtGui/QBitmap>
#include <QLayout>
#include <QtGui/QPainter>
#include <QPaintEvent>



// toolbar for remote-control-widget
RemoteAccessWidgetToolBar::RemoteAccessWidgetToolBar(
			RemoteAccessWidget * _parent, bool viewOnly ) :
	QWidget( _parent ),
	m_parent( _parent ),
	m_showHideTimeLine(),
	m_iconStateTimeLine(),
	m_connecting( false ),
	m_icon( QPixmap( ":/resources/icon128.png" ).scaled( QSize( 48, 48 ),
					Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) )
{
	QPalette pal = palette();
	pal.setBrush( QPalette::Window, QPixmap( ":/resources/toolbar-background.png" ) );
	setPalette( pal );

	setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, 0 );
	show();
	startConnection();

	ToolButton * vo_btn = new ToolButton(
				QPixmap( ":/remoteaccess/kmag.png" ),
				tr( "View only" ), tr( "Remote control" ) );
	ToolButton * ls_btn = new ToolButton(
				QPixmap( ":/remoteaccess/kr_unselect.png" ),
				tr( "Lock student" ), tr( "Unlock student" ) );
	ToolButton * ss_btn = new ToolButton(
				QPixmap( ":/remoteaccess/camera-photo.png" ),
				tr( "Snapshot" ) );
	ToolButton * fs_btn = new ToolButton(
				QPixmap( ":/remoteaccess/view-fullscreen.png" ),
				tr( "Fullscreen" ), tr( "Window" ) );
	ToolButton * quit_btn = new ToolButton(
				QPixmap( ":/remoteaccess/application-exit.png" ),
				tr( "Quit" ) );
	vo_btn->setCheckable( true );
	ls_btn->setCheckable( true );
	fs_btn->setCheckable( true );
	vo_btn->setChecked( viewOnly );
	fs_btn->setChecked( false );

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

	setFixedHeight( 52 );

	m_showHideTimeLine.setFrameRange( 0, height() );
	m_showHideTimeLine.setDuration( 800 );
	m_showHideTimeLine.setCurveShape( QTimeLine::EaseInCurve );
	connect( &m_showHideTimeLine, SIGNAL( valueChanged( qreal ) ),
				this, SLOT( updatePosition() ) );

	m_iconStateTimeLine.setFrameRange( 0, 100 );
	m_iconStateTimeLine.setDuration( 1500 );
	m_iconStateTimeLine.setUpdateInterval( 60 );
	m_iconStateTimeLine.setCurveShape( QTimeLine::SineCurve );
	connect( &m_iconStateTimeLine, SIGNAL( valueChanged( qreal ) ),
				this, SLOT( updateConnectionAnimation() ) );
	connect( &m_iconStateTimeLine, SIGNAL( finished() ),
				&m_iconStateTimeLine, SLOT( start() ) );
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

	p.setPen( Qt::white );
	p.drawText( 64, 22, m_parent->windowTitle() );

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
	update( 0, 0, 200, 63 );
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
	QWidget( 0 ),
	m_computerControlInterface( computerControlInterface ),
	m_vncView( new VncView( computerControlInterface.computer().hostAddress(), this, VncView::RemoteControlMode ) ),
	m_coreConnection( new ItalcCoreConnection( m_vncView->vncConnection() ) ),
	m_toolBar( new RemoteAccessWidgetToolBar( this, viewOnly ) )
{
	setWindowIcon( QPixmap( ":/remoteaccess/kmag.png" ) );
	setAttribute( Qt::WA_DeleteOnClose, true );

	m_vncView->move( 0, 0 );
	connect( m_vncView, SIGNAL( mouseAtTop() ), m_toolBar,
							SLOT( appear() ) );
	connect( m_vncView, SIGNAL( keyEvent( int, bool ) ),
				this, SLOT( checkKeyEvent( int, bool ) ) );
	connect( m_vncView, SIGNAL( sizeHintChanged() ),
					this, SLOT( updateSize() ) );

	// TODO: read from ComputerControlInterface after making VncView use existing connection
	connect( m_coreConnection, SIGNAL( receivedUserInfo( QString, QString ) ),
			 this, SLOT( updateWindowTitle() ) );

	show();
	LocalSystem::activateWindow( this );

	toggleViewOnly( viewOnly );
}




RemoteAccessWidget::~RemoteAccessWidget()
{
	delete m_coreConnection;
	delete m_vncView;
}




void RemoteAccessWidget::updateWindowTitle()
{
	const QString s = m_vncView->isViewOnly() ?
			tr( "View live (%1 @ %2)" )
		:
			tr( "Remote control (%1 @ %2)" );
	QString u = m_computerControlInterface.user();
	if( u.isEmpty() )
	{
		u = tr( "unknown user" );
	}
	else
	{
		if( u.contains( '(' ) && u.contains( ')' ) )
		{
			u = u.section( '(', 1 ).section( ')', 0, 0 );
		}
	}

	setWindowTitle( s.arg( u ).arg( m_computerControlInterface.computer().hostAddress() ) );
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




void RemoteAccessWidget::lockStudent( bool _on )
{
	//m_vncView->m_connection->disableLocalInputs( _on );
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




void RemoteAccessWidget::toggleViewOnly( bool _on )
{
	m_vncView->setViewOnly( _on );
	m_toolBar->update();
}




void RemoteAccessWidget::takeSnapshot()
{
	Snapshot().take( m_computerControlInterface );
}


