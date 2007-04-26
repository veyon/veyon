/*
 *  remote_control_widget.cpp - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include "remote_control_widget.h"
#include "vncview.h"
#include "local_system.h"
#include "tool_button.h"

#include <math.h>


#include <QtCore/QTimer>
#include <QtGui/QBitmap>
#include <QtGui/QDesktopWidget>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>





// toolbar for remote-control-widget
remoteControlWidgetToolBar::remoteControlWidgetToolBar(
					remoteControlWidget * _parent ) :
	QWidget( _parent ),
	m_parent( _parent ),
	m_disappear( FALSE ),
	m_connecting( FALSE ),
	m_icon( QImage( ":/resources/logo.png" ) ),
	m_iconGray( fastQImage( m_icon ).toGray().darken( 50 ) ),
	m_iconState()
{
	setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, 0 );
	show();
	startConnection();

	toolButton * ls_btn = new toolButton(
				QPixmap( ":/resources/mouse.png" ),
				tr( "Lock student" ),
				QString::null, QString::null, 0, 0,
				this );
	toolButton * fs_btn = new toolButton(
				QPixmap( ":/resources/fullscreen.png" ),
				tr( "Fullscreen" ),
				QString::null, QString::null, 0, 0,
				this );
	toolButton * quit_btn = new toolButton(
				QPixmap( ":/resources/quit.png" ),
				tr( "Quit" ),
				QString::null, QString::null, 0, 0,
				this );
	ls_btn->setCheckable( TRUE );
	fs_btn->setCheckable( TRUE );
	fs_btn->setChecked( TRUE );

	connect( fs_btn, SIGNAL( toggled( bool ) ),
			_parent, SLOT( toggleFullScreen( bool ) ) );
	connect( quit_btn, SIGNAL( clicked() ), _parent, SLOT( close() ) );

	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 1 );
	layout->setSpacing( 1 );
	layout->addStretch( 0 );
	layout->addWidget( ls_btn );
	layout->addWidget( fs_btn );
	layout->addWidget( quit_btn );
	layout->addSpacing( 5 );
	connect( m_parent->m_vncView, SIGNAL( startConnection() ),
					this, SLOT( startConnection() ) );
	connect( m_parent->m_vncView, SIGNAL( connectionEstablished() ),
					this, SLOT( connectionEstablished() ) );
}




remoteControlWidgetToolBar::~remoteControlWidgetToolBar()
{
}




void remoteControlWidgetToolBar::appear( void )
{
	m_disappear = FALSE;
	if( y() <= -height() )
	{
		updatePosition();
	}
}



void remoteControlWidgetToolBar::disappear( void )
{
	if( !m_connecting )
	{
		m_disappear = TRUE;
		if( y() == 0 )
		{
			updatePosition();
		}
		//m_parent->m_vncView->grabMouse();
	}
}




void remoteControlWidgetToolBar::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing, true );
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, QColor( 64, 128, 255 ) );
	lingrad.setColorAt( 0.38, QColor( 32, 64, 192 ) );
	lingrad.setColorAt( 0.42, QColor( 16, 32, 128 ) );
	lingrad.setColorAt( 1, QColor( 0, 16, 32 ) );
	p.fillRect( rect(), lingrad );

	p.drawImage( 5, 2, m_icon );

	QFont f = p.font();
	f.setPointSize( 12 );
	f.setBold( TRUE );
	p.setFont( f );

	p.setPen( QColor( 255, 212, 0 ) );
	p.drawText( 64, 22, m_parent->windowTitle() );

	p.setPen( QColor( 255, 255, 255 ) );
	f.setPointSize( 10 );
	p.setFont( f );

	if( m_connecting )
	{
		fastQImage tmp = m_iconGray;
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




void remoteControlWidgetToolBar::updatePosition( void )
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



void remoteControlWidgetToolBar::startConnection( void )
{
	m_connecting = TRUE;
	m_iconState.restart();
	appear();
	update();
}



void remoteControlWidgetToolBar::connectionEstablished( void )
{
	m_connecting = FALSE;
	QTimer::singleShot( 3000, this, SLOT( disappear() ) );
}





remoteControlWidget::remoteControlWidget( const QString & _host,
							bool _view_only ) :
	QWidget( 0 ),
	m_vncView( new vncView( _host, _view_only, this ) ),
	m_toolBar( new remoteControlWidgetToolBar( this ) ),
	m_extraStates( Qt::WindowMaximized )
{
	setWindowIcon( QPixmap( ":/resources/display.png" ) );
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
	m_vncView->move( 0, 0 );
	connect( m_vncView, SIGNAL( mouseAtTop() ), m_toolBar,
							SLOT( appear() ) );
	connect( m_vncView, SIGNAL( keyEvent( Q_UINT32, bool ) ),
				this, SLOT( checkKeyEvent( Q_UINT32, bool ) ) );
	//showMaximized();
	showFullScreen();
	localSystem::activateWindow( this );
	if( m_vncView->viewOnly() )
	{
		setWindowTitle( tr( "View live (host %1)" ).arg( host() ) );
	}
	else
	{
		setWindowTitle( tr( "Remote control (host %1)" ).
								arg( host() ) );
	}
	m_toolBar->update();
}




remoteControlWidget::~remoteControlWidget()
{
}




QString remoteControlWidget::host( void ) const
{
	return( m_vncView->m_connection ? m_vncView->m_connection->host() :
								QString::null );
}




void remoteControlWidget::resizeEvent( QResizeEvent * )
{
	m_vncView->resize( size() );
	m_toolBar->setFixedSize( width(), 52 );
}




void remoteControlWidget::checkKeyEvent( Q_UINT32 _key, bool _pressed )
{
	if( _pressed && _key == XK_Escape &&
		m_vncView->m_connection->state() != ivsConnection::Connected )
	{
		close();
	}
}




void remoteControlWidget::toggleFullScreen( bool _on )
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



#include "remote_control_widget.moc"

