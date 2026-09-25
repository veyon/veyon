/*
 *  LockWidget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006-2024 Tobias Junghans <tobydox@veyon.io>
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
#include <QPainter>
#include <QScreen>
#include <QWindow>
#include <QLinearGradient>
#include <QFont>


LockWidget::LockWidget( Mode mode, const QPixmap& background, QWidget* parent ) :
	QWidget( parent, Qt::X11BypassWindowManagerHint ),
	m_background( background ),
	m_mode( mode )
{
	auto leftMostScreen = QGuiApplication::primaryScreen();
	int minimumX = 0;
	const auto screens = QGuiApplication::screens();
	for (auto* screen : screens)
	{
		if (screen->geometry().x() < minimumX)
		{
			minimumX = screen->geometry().x();
			leftMostScreen = screen;
		}
	}

	if (mode == DesktopVisible)
	{
		m_background = leftMostScreen->grabWindow(0);
	}

	VeyonCore::platform().coreFunctions().setSystemUiState( false );
	VeyonCore::platform().inputDeviceFunctions().disableInputDevices();

	setWindowTitle( {} );

#ifdef Q_OS_LINUX
	show();
#endif
	move(leftMostScreen->geometry().topLeft());
#ifndef Q_OS_LINUX
	showFullScreen();
#endif
	windowHandle()->setScreen(leftMostScreen);
	setFixedSize(leftMostScreen->virtualSize());

	VeyonCore::platform().coreFunctions().raiseWindow(this, true);
#ifdef Q_OS_LINUX
	showFullScreen();
#endif

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
	{
		p.setRenderHint( QPainter::Antialiasing );
		p.setRenderHint( QPainter::TextAntialiasing );

		// Elegant background gradient (Dark Slate & Muhammadiyah Green)
		QLinearGradient gradient( 0, 0, 0, height() );
		gradient.setColorAt( 0.0, QColor( 6, 44, 26 ) );   // Deep dark green #062C1A
		gradient.setColorAt( 0.6, QColor( 10, 86, 51 ) );  // Rich Muhammadiyah green #0A5633
		gradient.setColorAt( 1.0, QColor( 4, 30, 18 ) );   // Bottom dark accent #041E12
		p.fillRect( rect(), gradient );

		const int centerX = width() / 2;
		const int centerY = height() / 2;

		// Draw icon centered slightly above middle
		if( !m_background.isNull() )
		{
			const int iconW = qMin( m_background.width(), 128 );
			const int iconH = qMin( m_background.height(), 128 );
			const int iconX = centerX - ( iconW / 2 );
			const int iconY = centerY - 150;
			p.drawPixmap( iconX, iconY, iconW, iconH, m_background );
		}

		// Title: LAYAR TERKUNCI (Gold)
		QFont titleFont = p.font();
		titleFont.setPointSize( 26 );
		titleFont.setBold( true );
		p.setFont( titleFont );
		p.setPen( QColor( 245, 158, 11 ) );
		QRect titleRect( 0, centerY - 10, width(), 45 );
		p.drawText( titleRect, Qt::AlignCenter, QStringLiteral( "LAYAR TERKUNCI" ) );

		// Subtitle: Harap Perhatikan Instruksi Guru
		QFont subFont = p.font();
		subFont.setPointSize( 14 );
		subFont.setBold( false );
		p.setFont( subFont );
		p.setPen( QColor( 255, 255, 255 ) );
		QRect subRect( 0, centerY + 42, width(), 35 );
		p.drawText( subRect, Qt::AlignCenter, QStringLiteral( "Harap Perhatikan Instruksi Guru di Depan Kelas" ) );

		// School: SMA MUHAMMADIYAH 1 PALEMBANG
		QFont schoolFont = p.font();
		schoolFont.setPointSize( 12 );
		schoolFont.setBold( true );
		p.setFont( schoolFont );
		p.setPen( QColor( 167, 243, 208 ) );
		QRect schoolRect( 0, centerY + 85, width(), 30 );
		p.drawText( schoolRect, Qt::AlignCenter, QStringLiteral( "SMA MUHAMMADIYAH 1 PALEMBANG" ) );

		// System Footer
		QFont sysFont = p.font();
		sysFont.setPointSize( 9 );
		sysFont.setBold( false );
		p.setFont( sysFont );
		p.setPen( QColor( 148, 163, 184, 180 ) );
		QRect sysRect( 0, height() - 50, width(), 30 );
		p.drawText( sysRect, Qt::AlignCenter, QStringLiteral( "Insight Teacher - Sistem Manajemen Laboratorium Komputer" ) );
		break;
	}

	default:
		break;
	}
}
