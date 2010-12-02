/*
 * DemoClient.cpp - client widget for demo mode
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtGui/QIcon>
#include <QtGui/QLayout>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

#include "DemoClient.h"
#include "LocalSystem.h"
#include "LockWidget.h"
#include "RfbLZORLE.h"
#include "RfbItalcCursor.h"
#include "VncView.h"


DemoClient::DemoClient( const QString &host, bool fullscreen ) :
	QObject(),
	m_toplevel( fullscreen ? new LockWidget( LockWidget::NoBackground )
							: new QWidget() )
{
	m_toplevel->setWindowTitle( tr( "iTALC Demo" ) );
	m_toplevel->setWindowIcon( QPixmap( ":/resources/display.png" ) );
	m_toplevel->setAttribute( Qt::WA_DeleteOnClose, false );
	m_toplevel->setWindowFlags( Qt::Window |
								Qt::CustomizeWindowHint |
								Qt::WindowTitleHint |
								Qt::WindowMinMaxButtonsHint );
	m_toplevel->resize( QApplication::desktop()->
					availableGeometry( m_toplevel ).size() - QSize( 10, 30 ) );

	// initialize extended protocol handlers
	RfbLZORLE();
	RfbItalcCursor();

	m_vncView = new VncView( host, m_toplevel, VncView::DemoMode );

	QVBoxLayout * toplevelLayout = new QVBoxLayout;
	toplevelLayout->setMargin( 0 );
	toplevelLayout->setSpacing( 0 );
	toplevelLayout->addWidget( m_vncView );

	m_toplevel->setLayout( toplevelLayout );

	connect( m_toplevel, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( viewDestroyed( QObject * ) ) );
	connect( m_vncView, SIGNAL( sizeHintChanged() ),
				this, SLOT( resizeToplevelWidget() ) );

	m_toplevel->move( 0, 0 );
	if( !fullscreen )
	{
		m_toplevel->show();
	}
	else
	{
		m_toplevel->showFullScreen();
	}
	LocalSystem::activateWindow( m_toplevel );
}




DemoClient::~DemoClient()
{
	delete m_toplevel;
}




void DemoClient::viewDestroyed( QObject * _obj )
{
	if( m_toplevel == _obj )
	{
		m_toplevel = NULL;
	}
	deleteLater();
}




void DemoClient::resizeToplevelWidget()
{
	if( !( m_toplevel->windowState() & Qt::WindowFullScreen ) )
	{
		m_toplevel->resize( m_vncView->sizeHint() );
	}
	else
	{
		m_vncView->resize( m_toplevel->size() );
	}
}

