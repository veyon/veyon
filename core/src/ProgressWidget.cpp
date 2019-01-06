/*
 *  ProgressWidget.cpp - widget with animated progress indicator
 *
 *  Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "ProgressWidget.h"

#include <QTimer>
#include <QPainter>

// clazy:excludeall=detaching-member

ProgressWidget::ProgressWidget( const QString& text, const QString& animationPixmapBase, int frames, QWidget* parent ) :
	QWidget( parent ),
	m_text( text ),
	m_frames( frames ),
	m_curFrame( 0 )
{
	m_pixmaps.reserve( m_frames );

	for( int i = 0; i < m_frames; ++i )
	{
		m_pixmaps.push_back( QPixmap( animationPixmapBase.arg( QString::number( i+1 ) ) ) );
	}

	QFont f = font();
	f.setPointSize( 12 );
	setFont( f );

	setFixedSize( 30 + m_pixmaps[0].width() + fontMetrics().width( m_text ),
			m_pixmaps[0].height() * 5 / 4 );

	auto t = new QTimer( this );
	connect( t, &QTimer::timeout, this, &ProgressWidget::nextFrame );
	t->start( 150 );
}



void ProgressWidget::nextFrame()
{
	m_curFrame = ( m_curFrame+1 ) % m_frames;

	update();
}



void ProgressWidget::paintEvent( QPaintEvent* event )
{
	Q_UNUSED(event);

	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );
	p.setPen( QColor( 0x55, 0x55, 0x55 ) );

	p.setBrush( Qt::white );
	p.drawRect( 0, 0, width() - 1, height() - 1 );
	p.drawPixmap( 6, ( height() - m_pixmaps[m_curFrame].height() ) / 2 - 1, m_pixmaps[m_curFrame] );

	p.drawText( 14 + m_pixmaps[m_curFrame].width(), 25, m_text );
}
