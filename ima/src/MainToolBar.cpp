/*
 *  MainToolBar.cpp - MainToolBar for MainWindow
 *
 *  Copyright (c) 2007-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "MainToolBar.h"



MainToolBar::MainToolBar( QWidget * _parent ) :
	QToolBar( tr( "Actions" ), _parent )
{
	QPalette pal = palette();
	pal.setBrush( QPalette::Window, QPixmap( ":/resources/toolbar-background.png" ) );
	setPalette( pal );
}




MainToolBar::~MainToolBar()
{
}




void MainToolBar::contextMenuEvent( QContextMenuEvent * _e )
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




void MainToolBar::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.setPen( QColor( 48, 48, 48 ) );
	p.fillRect( _pe->rect(), palette().brush( QPalette::Window ) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, height()-1, width(), height()-1 );
}




void MainToolBar::toggleButton( QAction * _a )
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



