/*
 *  lock_widget.cpp - widget for locking a client
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


#include "lock_widget.h"
#include "local_system.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QIcon>
#include <QtGui/QPainter>



#ifdef BUILD_LINUX

#include <X11/Xlib.h>

#endif



lockWidget::lockWidget( types _type ) :
	QWidget( 0, Qt::X11BypassWindowManagerHint ),
	m_background(
		_type == Black ?
			QPixmap( ":/resources/locked_bg.png" )
		:
			_type == DesktopVisible ?
				QPixmap::grabWindow( qApp->desktop()->winId() )
			:
				QPixmap() ),
	m_type( _type ),
	m_sysKeyTrapper()
{
	m_sysKeyTrapper.disableAllKeys( TRUE );
	setWindowTitle( tr( "screen lock" ) );
	setWindowIcon( QIcon( ":/resources/icon32.png" ) );
	showFullScreen();
	move( 0, 0 );
	setFixedSize( QApplication::desktop()->screenGeometry( this ).size() );
	localSystem::activateWindow( this );
	//setFixedSize( qApp->desktop()->size() );
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	grabMouse();
	grabKeyboard();
	setCursor( Qt::BlankCursor );
}




lockWidget::~lockWidget()
{
}




void lockWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	switch( m_type )
	{
		case DesktopVisible:
			p.drawPixmap( 0, 0, m_background );
			break;

		case Black:
			p.fillRect( rect(), QColor( 64, 64, 64 ) );
			p.drawPixmap( ( width() - m_background.width() ) / 2, 
				( height() - m_background.height() ) / 2,
								m_background );
			break;

		default:
			break;
	}
}



#ifdef BUILD_LINUX
bool lockWidget::x11Event( XEvent * _e )
{
	switch( _e->type )
	{
		case KeyPress:
		case ButtonPress:
		case MotionNotify:
			return( TRUE );
		default:
			break;
	}
	return( FALSE );
}
#endif


