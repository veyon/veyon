/*
 *  tool_bar.cpp - extended toolbar
 *
 *  Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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



#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include "tool_bar.h"
#include "tool_button.h"




// toolbar for remote-control-widget
toolBar::toolBar( const QString & _title, QWidget * _parent ) :
	QToolBar( _title, _parent )
{
	setMovable( FALSE );

	move( 0, 0 );
	show();

	setFixedHeight( 54 );

	QPixmap bg( 100, height() );
	QPainter p( &bg );
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, QColor( 224, 224, 255 ) );
	lingrad.setColorAt( 0.04, QColor( 80, 160, 255 ) );
	lingrad.setColorAt( 0.40, QColor( 32, 64, 192 ) );
	lingrad.setColorAt( 0.42, QColor( 8, 16, 96 ) );
	lingrad.setColorAt( 1, QColor( 0, 64, 224 ) );
	p.fillRect( QRect( 0, 0, 100, height() ), lingrad );
	p.end();

	QPalette pal = palette();
	pal.setBrush( backgroundRole(), bg );
	setPalette( pal );
}




toolBar::~toolBar()
{
}




void toolBar::contextMenuEvent( QContextMenuEvent * _e )
{
	QMenu m( this );
	foreach( QAction * a, actions() )
	{
		QAction * ma = m.addAction( a->text() );
		ma->setCheckable( TRUE );
		ma->setChecked( a->isVisible() );
	}
	connect( &m, SIGNAL( triggered( QAction * ) ),
				this, SLOT( toggleButton( QAction * ) ) );
	m.exec( _e->globalPos() );
}




void toolBar::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.setPen( Qt::black );
	p.fillRect( rect(), palette().brush( backgroundRole() ) );
	p.drawLine( 0, 0, width(), 0 );
}




void toolBar::toggleButton( QAction * _a )
{
	foreach( QAction * a, actions() )
	{
		if( a->text() == _a->text() )
		{
			a->setVisible( !a->isVisible() );
			break;
		}
	}
}


#include "tool_bar.moc"


