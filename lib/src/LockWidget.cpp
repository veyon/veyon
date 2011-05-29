/*
 *  LockWidget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <italcconfig.h>
#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include "LockWidget.h"
#include "LocalSystem.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QIcon>
#include <QtGui/QPainter>



#ifdef ITALC_BUILD_LINUX

#include <X11/Xlib.h>

#elif ITALC_BUILD_WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static const UINT __ss_get_list[] = { SPI_GETLOWPOWERTIMEOUT,
						SPI_GETPOWEROFFTIMEOUT,
						SPI_GETSCREENSAVETIMEOUT };
static const UINT __ss_set_list[] = { SPI_SETLOWPOWERTIMEOUT,
						SPI_SETPOWEROFFTIMEOUT,
						SPI_SETSCREENSAVETIMEOUT };
static int __ss_val[3];

#endif



LockWidget::LockWidget( Modes _mode ) :
	QWidget( 0, Qt::X11BypassWindowManagerHint ),
	m_background(
		_mode == Black ?
			QPixmap( ":/resources/locked_bg.png" )
		:
			_mode == DesktopVisible ?
				QPixmap::grabWindow( qApp->desktop()->winId() )
			:
				QPixmap() ),
	m_mode( _mode ),
	m_sysKeyTrapper()
{
	m_sysKeyTrapper.setAllKeysDisabled( true );
	m_sysKeyTrapper.setTaskBarHidden( true );

	setWindowTitle( tr( "screen lock" ) );
	setWindowIcon( QIcon( ":/resources/icon32.png" ) );
	showFullScreen();
	move( 0, 0 );
	//setFixedSize( QApplication::desktop()->screenGeometry( this ).size() );
	setFixedSize( qApp->desktop()->size() );
	setCursor( Qt::BlankCursor );
	LocalSystem::activateWindow( this );
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	grabMouse();
	grabKeyboard();
	setCursor( Qt::BlankCursor );

#ifdef ITALC_BUILD_WIN32
	// disable screensaver
	for( int x = 0; x < 3; ++x )
	{
		SystemParametersInfo( __ss_get_list[x], 0, &__ss_val[x], 0 );
		SystemParametersInfo( __ss_set_list[x], 0, NULL, 0 );
	}
#endif
}




LockWidget::~LockWidget()
{
#ifdef ITALC_BUILD_WIN32
	// revert screensaver-settings
	for( int x = 0; x < 3; ++x )
	{
		SystemParametersInfo( __ss_set_list[x], __ss_val[x], NULL, 0 );
	}
#endif
	m_sysKeyTrapper.setAllKeysDisabled( false );
}




void LockWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	switch( m_mode )
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



#ifdef ITALC_BUILD_LINUX
bool LockWidget::x11Event( XEvent * _e )
{
	switch( _e->type )
	{
		case KeyPress:
		case ButtonPress:
		case MotionNotify:
			return true;
		default:
			break;
	}
	return false;
}
#endif


