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


	QFont f = p.font();
	f.setPointSize( 14 );
	f.setBold( TRUE );
	p.setFont( f );
	QFont f2 = f;
	f2.setPointSize( 10 );

	p.setPen( QColor( 255, 212, 0 ) );

	const QString wt = m_parent->windowTitle();
	const QString title = wt.section( '(', 0, 0 );
	const QString host = wt.section( '(', 1, 1 ).section( ')', 0, 0 );
	const int px = width() - layout()->minimumSize().width() -
				qMax( p.fontMetrics().width( title ),
					QFontMetrics( f2 ).width( host ) ) - 10;
	p.drawText( px, 22, title );
	p.setPen( QColor( 255, 255, 255 ) );
	p.setFont( f2 );
	p.drawText( px, 38, host );

	p.setFont( f );
	p.drawImage( 5, 2, m_icon );
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
		p.drawText( 64, 32, tr( "Connecting %1" ).arg( dots ) );
		QTimer::singleShot( 50, this, SLOT( update() ) );
	}
	else
	{
		p.drawText( 64, 32, tr( "Connected." ) );
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




#if 0
ToolButton::fxToolButton( const QString & _hint_label, const QImage & _img,
					remoteControlWidgetToolBar * _parent ) :
	QPushButton( _parent ),
	m_parent( _parent ),
	m_hintLabel( _hint_label ),
	m_img( _img ),
	m_imgGray( _img ),
	m_colorizeLevel( 0 ),
	m_fadeBack( FALSE )
{
	setAttribute( Qt::WA_NoSystemBackground, true );
	m_imgGray.toGray();
	QFont f = font();
	f.setPointSize( 7 );
	setFont( f );
	//setFixedWidth( qMax<int>( m_img.width(), fontMetrics().width( m_hintLabel ) )+20 );
	//setFixedHeight( 48 );
	setFixedSize( 90, 48 );

	// set mask
	QBitmap b( size() );
	b.clear();

	QPainter p( &b );
	p.setBrush( Qt::color1 );
	p.setPen( Qt::color1 );
	p.drawRoundRect( 0, 0, width() - 1, height() - 1,
					1000 / width(), 1000 / height() );
	setMask( b );
}




fxToolButton::~fxToolButton()
{
}




void fxToolButton::enterEvent( QEvent * )
{
	m_fadeBack = FALSE;
	if( m_colorizeLevel == 0 )
	{
		updateColorLevel();
	}
}




void fxToolButton::leaveEvent( QEvent * )
{
	m_fadeBack = TRUE;
	if( m_colorizeLevel == 255 )
	{
		updateColorLevel();
	}
}



void fxToolButton::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( rect(), Qt::black );
	p.setRenderHint( QPainter::Antialiasing );
	const QColor ctbl[2][4] = {
				{
					QColor( 64, 128, 255 ),
					QColor( 32, 64, 192 ),
					QColor( 16, 32, 128 ),
					QColor( 0, 16, 64 )
				},
				{
					QColor( 255, 224, 0, m_colorizeLevel ),
					QColor( 224, 192, 0, m_colorizeLevel ),
					QColor( 192, 160, 0, m_colorizeLevel ),
					QColor( 96, 48, 0, m_colorizeLevel )
				}
				} ;
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, ctbl[0][0] );
	lingrad.setColorAt( 0.38, ctbl[0][1] );
	lingrad.setColorAt( 0.42, ctbl[0][2] );
	lingrad.setColorAt( 1, ctbl[0][3] );
	p.setBrush( lingrad );
	p.drawRoundRect( 1, 1, width()-2, height()-2, 1000 / width(),
							1000 / height() );

	if( m_colorizeLevel )
	{
		lingrad = QLinearGradient( 0, 0, 0, height() );
		lingrad.setColorAt( 0, ctbl[1][0] );
		lingrad.setColorAt( 0.38, ctbl[1][1] );
		lingrad.setColorAt( 0.42, ctbl[1][2] );
		lingrad.setColorAt( 1, ctbl[1][3] );
		p.setBrush( lingrad );
		p.drawRoundRect( 1, 1, width()-2, height()-2, 1000 / width(),
							1000 / height() );
	}
	p.setBrush( QBrush() );

	p.fillRect( rect(), QColor( 0, 0, 0, isDown() ? 128 : 0 ) );

	QPen pen( QColor( 255, 255, 255, 128 ) );
	pen.setWidthF( 1.3f );
	p.setPen( pen );
	p.drawRoundRect( 0, 0, width()-1, height()-1, 1000 / width(),
							1000 / height() );
	QPen pen2 = pen;
	pen.setColor( QColor( 0, 0, 0, 128 ) );
	p.setPen( pen );
	p.drawRoundRect( 1, 1, width()-3, height()-3, 1000 / width(),
							1000 / height() );
	p.setPen( pen2 );
	p.drawRoundRect( 2, 2, width()-2, height()-2, 1000 / width(),
							1000 / height() );

	const int dd = isDown() ? 1 : 0;
	const QPoint pt = QPoint( (width() - m_img.width() ) / 2 + dd, 3 + dd);
	p.drawImage( pt, m_img );

	const int w = p.fontMetrics().width( m_hintLabel );
	p.setPen( Qt::black );
	p.drawText( ( width() -w ) / 2 + 1+dd, height() - 4+dd, m_hintLabel );
	p.setPen( Qt::white );
	p.drawText( ( width() - w ) / 2+dd, height() - 5+dd, m_hintLabel );

}





void fxToolButton::updateColorLevel( void )
{
	bool again;
	if( m_fadeBack )
	{
		m_colorizeLevel = qMax( 0, m_colorizeLevel - 10 );
		again = m_colorizeLevel > 0;
	}
	else
	{
		m_colorizeLevel = qMin( 255, m_colorizeLevel + 10 );
		again = m_colorizeLevel < 255;
	}
	update();
	if( again )
	{
		QTimer::singleShot( 10, this, SLOT( updateColorLevel() ) );
	}
}




#endif




remoteControlWidget::remoteControlWidget( const QString & _host,
							bool _view_only ) :
	QWidget( 0/*, Qt::X11BypassWindowManagerHint*/ ),
	m_vncView( new vncView( _host, _view_only, this ) ),
	m_toolBar( new remoteControlWidgetToolBar( this ) )
{
	setWindowIcon( QPixmap( ":/resources/display.png" ) );
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
	m_vncView->move( 0, 0 );
	connect( m_vncView, SIGNAL( mouseAtTop() ), m_toolBar,
							SLOT( appear() ) );
	connect( m_vncView, SIGNAL( mouseAtTop() ), this,
							SLOT( updateUser() ) );
	connect( m_vncView, SIGNAL( keyEvent( Q_UINT32, bool ) ),
				this, SLOT( checkKeyEvent( Q_UINT32, bool ) ) );
	//resize( QDesktopWidget().screenGeometry( this ).size() );
	showMaximized();
	showFullScreen();
	move( 0, 0 );
	localSystem::activateWindow( this );
	//toggleFullScreen();
	updateUser();
#ifdef BUILD_LINUX
	// for some reason we have to grab mouse and then release again to
	// make complete keyboard-grabbing working ... ??
	m_vncView->grabMouse();
	m_vncView->releaseMouse();
#endif
	m_vncView->grabKeyboard();
	m_vncView->setFocusPolicy( Qt::StrongFocus );
	m_vncView->setFocus();
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




void remoteControlWidget::updateUser( void )
{
	if( m_vncView->m_connection == NULL )
	{
		QTimer::singleShot( 100, this, SLOT( updateUser() ) );
		return;
	}
/*	m_user = m_vncView->m_connection->user();
	if( m_user.isEmpty() )
	{*/
	if( m_vncView->viewOnly() )
	{
		setWindowTitle( tr( "View live (host %1)" ).arg( host() ) );
	}
	else
	{
		setWindowTitle( tr( "Remote control (host %1)" ).
								arg( host() ) );
	}
		//m_vncView->m_connection->sendGetUserInformationRequest();
/*		QTimer::singleShot( 100, this, SLOT( updateUser() ) );
	}
	else
	{
		setWindowTitle( tr( "User %1 at host %2" ).
						arg( m_user ).arg( host() ) );
	}*/
	m_toolBar->update();
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
	}
/*	if( windowState() & Qt::WindowFullScreen )
	{
		setWindowFlags( windowFlags() | Qt::X11BypassWindowManagerHint );
	}
	else
	{
		printf("toggle\n");
		setWindowFlags( windowFlags() & ~Qt::X11BypassWindowManagerHint );
	}*/
}



#include "remote_control_widget.moc"

