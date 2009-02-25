/*
 * demo_client.cpp - client for demo-server
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "demo_client.h"
#include "vncview.h"
#include "lock_widget.h"


demoClient::demoClient( const QString & _host, bool _fullscreen ) :
	QObject(),
	m_toplevel( _fullscreen ?
			new lockWidget( lockWidget::NoBackground )
			:
			new QWidget() )
{
	m_toplevel->setWindowTitle( tr( "iTALC Demo" ) );
	m_toplevel->setWindowIcon( QPixmap( ":/resources/display.png" ) );
	m_toplevel->setAttribute( Qt::WA_DeleteOnClose, TRUE );

	QVBoxLayout * toplevel_layout = new QVBoxLayout( m_toplevel );
	toplevel_layout->setMargin( 0 );
	toplevel_layout->setSpacing( 0 );
	toplevel_layout->addWidget( new vncView( _host, TRUE, m_toplevel ) );

	connect( m_toplevel, SIGNAL( destroyed( QObject * ) ),
			this, SLOT( viewDestroyed( QObject * ) ) );

	m_toplevel->showMaximized();
}




demoClient::~demoClient()
{
	delete m_toplevel;
}




void demoClient::viewDestroyed( QObject * _obj )
{
	if( m_toplevel == _obj )
	{
		m_toplevel = NULL;
	}
	deleteLater();
}



#include "demo_client.moc"
