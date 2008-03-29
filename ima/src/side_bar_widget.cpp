/*
 * side_bar_widget.cpp - implementation of base-widget for side-bar
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QPixmap>
#include <QtGui/QResizeEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QLayout>


#include "side_bar_widget.h"
#include "fast_qimage.h"



sideBarWidget::sideBarWidget( const QPixmap & _icon,
				const QString & _title,
				const QString & _description,
				mainWindow * _main_window, QWidget * _parent ) :
	QWidget( _parent ),
	m_mainWindow( _main_window ),
	m_icon( _icon ),
	m_title( _title ),
	m_description( _description )
{
	_parent->layout()->addWidget( this );

	m_contents = new QWidget( this );
	QVBoxLayout * contents_layout = new QVBoxLayout( m_contents );
	contents_layout->setSpacing( 5 );
	contents_layout->setMargin( 5 );
	contents_layout->addSpacing( 10 );
}




sideBarWidget::~sideBarWidget()
{
}




void sideBarWidget::paintEvent( QPaintEvent * )
{
	const int TITLE_FONT_HEIGHT = 16;

	QPainter p( this );
	p.fillRect( rect(), QColor( 255, 255, 255 ) );
	p.fillRect( 0, 0, width(), 27+32, QColor( 0, 64, 224 ) );

	p.setRenderHint( QPainter::Antialiasing, TRUE );
	p.setPen( Qt::white );
	p.setBrush( Qt::white );
	p.drawRoundRect( QRect( 0, 27, 64, 64 ), 1000, 1000 );
	p.fillRect( 32, 27, width()-32, 32, Qt::white );

	QFont f;
	f.setBold( TRUE );
	f.setPixelSize( TITLE_FONT_HEIGHT );

	p.setFont( f );
	p.setPen( QColor( 255, 216, 32 ) );
	const int tx = /*m_icon.width()*/48 + 8;
	const int ty = 2 + TITLE_FONT_HEIGHT;
	p.drawText( tx, ty, m_title );
	//p.drawLine( tx, ty + 4, width() - 4, ty + 4 );

	p.drawImage( 2, 2, fastQImage( m_icon ).scaled( 48, 48 ) );
}




void sideBarWidget::resizeEvent( QResizeEvent * )
{
	const int MARGIN = 6;
	m_contents->setGeometry( MARGIN, 40 + MARGIN, width() - MARGIN * 2,
						height() - MARGIN * 2 - 40 );
}

