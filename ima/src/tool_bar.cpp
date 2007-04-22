/*
 *  tool_bar.cpp - extended toolbar
 *
 *  Copyright (c) 2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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



#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include "tool_bar.h"




// toolbar for remote-control-widget
toolBar::toolBar( const QString & _title, QWidget * _parent ) :
	QToolBar( _title, _parent )
{
	setMovable( FALSE );
	//setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, 0 );
	show();

	setFixedHeight( 56 );
	QPixmap bg( 100, height() );
	QPainter p( &bg );
	p.setRenderHint( QPainter::Antialiasing, true );
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, QColor( 192, 224, 255 ) );
	lingrad.setColorAt( 0.04, QColor( 64, 128, 255 ) );
	lingrad.setColorAt( 0.38, QColor( 32, 64, 192 ) );
	lingrad.setColorAt( 0.42, QColor( 16, 32, 128 ) );
	lingrad.setColorAt( 1, QColor( 0, 16, 32 ) );
	p.fillRect( QRect( 0, 0, 100, height() ), lingrad );
	p.end();

	setAutoFillBackground( TRUE );
	QPalette pal = palette();
	pal.setBrush( backgroundRole(), bg );
	pal.setBrush( foregroundRole(), bg );
	setPalette( pal );
}




toolBar::~toolBar()
{
}



