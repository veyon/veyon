/*
 * italc_side_bar.cpp - implementation iTALC's side-bar
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include "italc_side_bar.h"
#include "fast_qimage.h"

#include <QtGui/QPainter>

#include <config.h>



italcSideBar::italcSideBar( KMultiTabBarStyle _style, QWidget * _parent,
							QWidget * _splitter ) :
	KMultiTabBar( Left, _parent ),
	m_openedTab( -1 )
{
	setAttribute( Qt::WA_OpaquePaintEvent, TRUE );
	setStyle( _style );
	m_tabWidgetParent = new QWidget( _splitter );
	m_tabWidgetParent->hide();
	QVBoxLayout * vl = new QVBoxLayout( m_tabWidgetParent );
	vl->setSpacing( 0 );
	vl->setMargin( 0 );
}




italcSideBar::~italcSideBar()
{
}




void italcSideBar::setTab( const int _id, const bool _state )
{
	if( m_widgets[_id] != NULL )
	{
		KMultiTabBar::setTab( _id, _state );
		if( _state )
		{
			m_widgets[_id]->show();
			m_openedTab = _id;
			m_tabWidgetParent->show();
		}
		else
		{
			m_widgets[_id]->hide();
			m_openedTab = -1;
		}
	}
}




int italcSideBar::appendTab( sideBarWidget * _sbw, const int _id )
{
	int ret = KMultiTabBar::appendTab( _sbw->icon(), _id,
						_sbw->title(),
						_sbw->description()
						);
	m_widgets[_id] = _sbw;
	_sbw->hide();
	_sbw->setMinimumWidth( 300 );
	connect( tab( _id ), SIGNAL( clicked( int ) ), this,
					SLOT( tabClicked( int ) ) );
	return( ret );
}



void italcSideBar::tabClicked( int _id )
{
	// disable all other tabbar-buttons 
	QMap<int, QWidget *>::Iterator it;
	for( it = m_widgets.begin(); it != m_widgets.end(); ++it )
	{
		if( it.key() != _id )
		{
			setTab( it.key(), FALSE );
		}
		if( m_widgets[it.key()] != NULL )
		{
			m_widgets[it.key()]->hide();
		}
	}

	setTab( _id, isTabRaised( _id ) );

	if( m_openedTab == -1 )
	{
		m_tabWidgetParent->hide();
	}
	else
	{
		m_tabWidgetParent->show();
	}
}




void italcSideBar::paintEvent( QPaintEvent * _pe )
{
	QWidget::paintEvent( _pe );

	QPainter p( this );
	QLinearGradient linearGrad( QPoint( 0, 0 ), QPoint( 0, height() ) );
	linearGrad.setColorAt( 0.1, QColor( 0, 64, 192 ) );
	linearGrad.setColorAt( 1, Qt::black );
	p.fillRect( 0, 0, width(), height(), linearGrad  );

	p.rotate( 270 );
	p.drawImage( -height()+3, 3, fastQImage(
			QImage( ":/resources/logo.png" ) ).scaled( 24, 24 ) );
	QFont f = p.font();
	f.setBold( TRUE );
	f.setPointSize( 15 );
	p.setFont( f );
	p.setPen( QColor( 255, 216, 0 ) );
	p.drawText( -height()+32, 22, QString( "iTALC %1" ).arg( PACKAGE_VERSION ) );
}


