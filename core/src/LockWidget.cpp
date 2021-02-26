/*
 *  LockWidget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
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

#include "LockWidget.h"
#include "PlatformCoreFunctions.h"
#include "PlatformInputDeviceFunctions.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QScreen>
#include <QWindow>


LockWidget::LockWidget( Mode mode, const QPixmap& background, QWidget* parent ) :
	QWidget( parent, Qt::X11BypassWindowManagerHint ),
	m_background( background ),
	m_mode( mode )
{
	if( mode == DesktopVisible )
	{
		auto screen = QGuiApplication::primaryScreen();
		if( windowHandle() )
		{
			screen = windowHandle()->screen();
		}

		if( screen )
		{
			m_background = screen->grabWindow( 0 );
		}
	}


	VeyonCore::platform().coreFunctions().setSystemUiState( false );
	VeyonCore::platform().inputDeviceFunctions().disableInputDevices();

	setWindowTitle( {} );
	show();
	move( 0, 0 );
	setFixedSize( qApp->desktop()->size() );
	VeyonCore::platform().coreFunctions().raiseWindow( this, true );
	showFullScreen();
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	grabMouse();
	grabKeyboard();
	setCursor( Qt::BlankCursor );
	QGuiApplication::setOverrideCursor( Qt::BlankCursor );

	QCursor::setPos( mapToGlobal( QPoint( 0, 0 ) ) );
}



LockWidget::~LockWidget()
{
	VeyonCore::platform().inputDeviceFunctions().enableInputDevices();
	VeyonCore::platform().coreFunctions().setSystemUiState( true );

	QGuiApplication::restoreOverrideCursor();
}



void LockWidget::paintEvent( QPaintEvent* event )
{
	Q_UNUSED(event);

	QPainter p( this );
	switch( m_mode )
	{
	case DesktopVisible:
		p.drawPixmap( 0, 0, m_background );
		break;

	case BackgroundPixmap:
		p.fillRect( rect(), QColor( 64, 64, 64 ) );
		p.drawPixmap( ( width() - m_background.width() ) / 2,
					  ( height() - m_background.height() ) / 2,
					  m_background );
		break;

	default:
		break;
	}
}
