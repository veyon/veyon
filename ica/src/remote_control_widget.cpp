/*
 *  remote_control_widget.cpp - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <math.h>


#include <QtCore/QTimer>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>





// toolbar for remote-control-widget
remoteControlWidgetToolBar::remoteControlWidgetToolBar(
					remoteControlWidget * _parent ) :
	QWidget( _parent ),
	m_parent( _parent ),
	m_opacity( 0 ),
	m_disappear( FALSE )
{
	setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, -200 );
	show();

	fxToolButton * fs_btn = new fxToolButton( tr( "Fullscreen/window" ),
				QImage( ":/resources/fullscreen.png" ), this );
	fxToolButton * quit_btn = new fxToolButton( tr( "Quit" ),
				QImage( ":/resources/quit.png" ), this );

	connect( fs_btn, SIGNAL( clicked() ), _parent,
						SLOT( toggleFullScreen() ) );
	connect( quit_btn, SIGNAL( clicked() ), _parent, SLOT( close() ) );

	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 1 );
	layout->setSpacing( 5 );
	layout->addStretch( 0 );
	layout->addWidget( fs_btn );
	layout->addWidget( quit_btn );
	layout->addSpacing( 10 );
}




remoteControlWidgetToolBar::~remoteControlWidgetToolBar()
{
}




void remoteControlWidgetToolBar::appear( void )
{
	m_disappear = FALSE;
	if( m_opacity == 0 )
	{
		// as there's no possibility to force Qt not to erase
		// background of newly exposed areas where children are
		// located, simply set an according background-pixmap
		QPalette pal = palette();
		pal.setBrush( backgroundRole(),
			QPixmap::grabWidget( m_parent->m_vncView, rect() ) );
		setPalette( pal );
		move( 0, 0 );
		updateOpacity();
	}
	//m_parent->m_vncView->releaseMouse();
}


static const int MaxOpac = 208;


void remoteControlWidgetToolBar::disappear( void )
{
	m_disappear = TRUE;
	if( m_opacity == MaxOpac )
	{
		updateOpacity();
	}
	//m_parent->m_vncView->grabMouse();
}




void remoteControlWidgetToolBar::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, QColor( 96, 96, 96 ) );
	lingrad.setColorAt( 1, QColor( 0, 0, 0 ) );
	p.fillRect( rect(), lingrad );

	const int fsize = 16;

	QFont f = p.font();
	f.setPointSize( fsize );
	f.setBold( TRUE );
	p.setFont( f );

	p.setPen( QColor( 0, 255, 0 ) );
	p.drawText( 10, ( height() + fsize ) / 2 - 2, m_parent->windowTitle() );

	fastQImage bg_img = QPixmap::grabWidget( m_parent->m_vncView,
								_pe->rect() ).
						toImage().
				convertToFormat( QImage::Format_ARGB32 );
	p.drawImage( _pe->rect().topLeft(),
					bg_img.alphaFill( 255 - m_opacity ) );
}




void remoteControlWidgetToolBar::updateOpacity( void )
{
	bool again;
	if( m_disappear )
	{
		m_opacity = qMax( 0, m_opacity - 8 );
		again = m_opacity > 0;
	}
	else
	{
		m_opacity = qMin( MaxOpac, m_opacity + 8 );
		again = m_opacity < MaxOpac;
	}

	update();

	if( again )
	{
		QTimer::singleShot( 15, this, SLOT( updateOpacity() ) );
	}
	if( m_opacity == 0 && m_disappear )
	{
		move( 0, -height() );
	}
}




fxToolButton::fxToolButton( const QString & _hint_label, const QImage & _img,
					remoteControlWidgetToolBar * _parent ) :
	QPushButton( _parent ),
	m_parent( _parent ),
	m_hintLabel( _hint_label ),
	m_img( _img ),
	m_imgGray( _img ),
	m_colorizeLevel( 0 ),
	m_fadeBack( FALSE ),
	m_widthExpansion( 0 )
{
	m_imgGray.toGray();
	setFixedSize( m_img.size() );
	QFont f = font();
	f.setPointSize( 12 );
	setFont( f );
	m_widthExpansion = fontMetrics().width( m_hintLabel ) + 5;
	setFixedWidth( m_img.width() + m_widthExpansion );
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

	fastQImage tmp = m_imgGray;
	p.drawImage( _pe->rect().topLeft(),
			tmp.alphaFillMax( m_parent->opacity() ), _pe->rect() );

		const Q_UINT16 coeff = isDown() ? 160 : 256;
	if( m_colorizeLevel )
	{
		tmp = m_img;
		tmp.alphaFillMax( m_colorizeLevel ).darken( coeff );

		p.drawImage( _pe->rect().topLeft(), tmp, _pe->rect() );
	}

/*	p.setPen( QColor( 128+m_colorizeLevel/2, 128-16+m_colorizeLevel/2,
						0, 191+m_colorizeLevel/4 ) );
		*/
	p.setPen( Qt::black );
	p.drawText( m_img.width() + 3, ( height() + p.font().pointSize() ) / 2 -
							1, m_hintLabel );
	p.setPen( QColor( 255-m_colorizeLevel, 255, 255-m_colorizeLevel ) );
	p.drawText( m_img.width() + 2, ( height() + p.font().pointSize() ) / 2 -
							2, m_hintLabel );

}




inline int calcWidth( const int _w, const int _level )
{
	return( (int)( _w * ( 1.0f - cosf( _level / 255.0f * 3.14159f ) ) *
								0.5f ) );
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
	//setFixedWidth( m_img.width() + m_widthExpansion );
	/* calcWidth( m_widthExpansion,
							m_colorizeLevel ) );*/
	parentWidget()->updateGeometry();
	update();
	if( again )
	{
		QTimer::singleShot( 10, this, SLOT( updateColorLevel() ) );
	}
}








remoteControlWidget::remoteControlWidget( const QString & _host,
							bool _view_only ) :
	QWidget(),
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

	activateWindow();
	raise();
	showMaximized();
	toggleFullScreen();
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
	m_toolBar->setFixedSize( width(), 34 );
}




void remoteControlWidget::updateUser( void )
{
	if( m_vncView->m_connection == NULL )
	{
		QTimer::singleShot( 100, this, SLOT( updateUser() ) );
		return;
	}
	m_user = m_vncView->m_connection->user();
	if( m_user.isEmpty() )
	{
		setWindowTitle( tr( "iTALC remote control (host %1)" ).
								arg( host() ) );
		//m_vncView->m_connection->sendGetUserInformationRequest();
		QTimer::singleShot( 100, this, SLOT( updateUser() ) );
	}
	else
	{
		setWindowTitle( tr( "User %1 at host %2" ).
						arg( m_user ).arg( host() ) );
	}
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




void remoteControlWidget::toggleFullScreen( void )
{
	setWindowState( windowState() ^ Qt::WindowFullScreen );
}




#include "remote_control_widget.moc"

