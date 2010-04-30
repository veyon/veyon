/*
 * SideBarWidget.cpp - implementation of SideBarWidget
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include "SideBarWidget.h"
#include "FastQImage.h"



SideBarWidget::SideBarWidget( const QPixmap & _icon,
				const QString & _title,
				const QString & _description,
				MainWindow * _main_window, QWidget * _parent ) :
	QWidget( _parent ),
	m_mainWindow( _main_window ),
	m_icon( _icon ),
	m_title( _title ),
	m_description( _description )
{
	QPixmap bg( ":/resources/toolbar-background.png" );
	QPainter painter( &bg );
	painter.fillRect( bg.rect(), QColor( 255, 255, 255, 160 ) );

	QPalette pal = palette();
	pal.setBrush( backgroundRole(), bg );
	setPalette( pal );

	QVBoxLayout * layout = new QVBoxLayout( this );
	layout->setSpacing( 10 );
	layout->setMargin( 5 );
	layout->addSpacing( 60 );

	m_contents = new QWidget;
	layout->addWidget( m_contents );

	setMinimumWidth( 200 );
	setMaximumWidth( 330 );
}




SideBarWidget::~SideBarWidget()
{
}




void SideBarWidget::paintEvent( QPaintEvent * )
{
	const int TITLE_FONT_HEIGHT = 16;

	QPainter p( this );
	p.fillRect( rect(), palette().brush( backgroundRole() ) );

	QFont f;
	f.setBold( true );
	f.setPixelSize( TITLE_FONT_HEIGHT );

	p.setFont( f );
	p.setPen( Qt::black );
	const int tx = /*m_icon.width()*/48 + 8;
	const int ty = 2 + TITLE_FONT_HEIGHT;
	p.drawText( tx, ty, m_title );
	p.drawLine( tx, ty + 4, width() - 4, ty + 4 );

	p.drawImage( 2, 2, FastQImage( m_icon ).scaled( 48, 48 ) );
}


