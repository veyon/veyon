/*
 *  LockWidget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006-2016 Tobias Junghans <tobydox@users.sf.net>
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

#include <veyonconfig.h>

#include "LockWidget.h"
#include "LocalSystem.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>


#ifdef VEYON_BUILD_WIN32

#include <windows.h>

static const UINT __ss_get_list[] = { SPI_GETLOWPOWERTIMEOUT,
						SPI_GETPOWEROFFTIMEOUT,
						SPI_GETSCREENSAVETIMEOUT };
static const UINT __ss_set_list[] = { SPI_SETLOWPOWERTIMEOUT,
						SPI_SETPOWEROFFTIMEOUT,
						SPI_SETSCREENSAVETIMEOUT };
static int __ss_val[3];

#endif



LockWidget::LockWidget( Mode mode, QWidget* parent ) :
	QWidget( parent, Qt::X11BypassWindowManagerHint ),
	m_background(),
	m_mode( mode ),
	m_inputDeviceBlocker()
{
	switch( mode )
	{
	case Black:
		m_background = QPixmap( QStringLiteral( ":/resources/locked_bg.png" ) );
		break;
	case DesktopVisible:
		QPixmap::grabWindow( qApp->desktop()->winId() );
		break;
	default:
		break;
	}

	setWindowTitle( tr( "screen lock" ) );
	showFullScreen();
	move( 0, 0 );
	setFixedSize( qApp->desktop()->size() );
	LocalSystem::activateWindow( this );
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	grabMouse();
	grabKeyboard();
	setCursor( Qt::BlankCursor );
	QGuiApplication::setOverrideCursor( Qt::BlankCursor );

	QCursor::setPos( mapToGlobal( QPoint( 0, 0 ) ) );

#ifdef VEYON_BUILD_WIN32
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
#ifdef VEYON_BUILD_WIN32
	// revert screensaver-settings
	for( int x = 0; x < 3; ++x )
	{
		SystemParametersInfo( __ss_set_list[x], __ss_val[x], NULL, 0 );
	}
#endif

	QGuiApplication::restoreOverrideCursor();
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

