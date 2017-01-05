/*
 *  ProgressWidget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include "ProgressWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QPainter>



ProgressWidget::ProgressWidget(  const QString & _txt,
					const QString & _anim, int _frames,
					QWidget * _parent ) :
	QWidget( _parent ),
	m_txt( _txt ),
	m_anim( _anim ),
	m_frames( _frames ),
	m_curFrame( 0 )
{
	for( int i = 1; i <= m_frames; ++i )
	{
		m_pixmaps.push_back( QPixmap( m_anim.arg(
						QString::number( i ) ) ) );
	}

	QFont f = font();
	f.setPointSize( 12 );
	setFont( f );

	setFixedSize( 30 + m_pixmaps[0].width() +
			fontMetrics().width( m_txt ),
			m_pixmaps[0].height() * 5 / 4 );

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this, SLOT( nextAnim() ) );
	t->start( 150 );
}




ProgressWidget::~ProgressWidget()
{
}




void ProgressWidget::nextAnim()
{
	m_curFrame = ( m_curFrame+1 ) % m_frames;
	update();
}



const int ROUNDED = 2000;

void ProgressWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );
	p.setPen( Qt::black );

	QLinearGradient grad( 0, 0, 0, height() );
	QColor g1( 224, 224, 224 );
	//g1.setAlpha( 204 );
	QColor g2( 160, 160, 160 );
	//g2.setAlpha( 204 );
	grad.setColorAt( 0, g1 );
	grad.setColorAt( 1, g2 );
	p.setBrush( grad );
	p.drawRoundRect( 0, 0, width() - 1, height() - 1,
					ROUNDED / width(), ROUNDED / height() );
	p.drawPixmap( 6, ( height() - m_pixmaps[m_curFrame].height() ) / 2 - 1,
							m_pixmaps[m_curFrame] );
	p.setPen( Qt::black );
	p.drawText( 14 + m_pixmaps[m_curFrame].width(), 25, m_txt );
}


