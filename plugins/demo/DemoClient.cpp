/*
 * DemoClient.cpp - client widget for demo mode
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QLayout>

#include "DemoClient.h"
#include "VeyonConfiguration.h"
#include "LockWidget.h"
#include "PlatformCoreFunctions.h"
#include "VncView.h"


DemoClient::DemoClient( const QString& host, bool fullscreen, QObject* parent ) :
	QObject( parent ),
	m_toplevel( nullptr )
{
	if( fullscreen )
	{
		m_toplevel = new LockWidget( LockWidget::NoBackground );
	}
	else
	{
		m_toplevel = new QWidget();
	}

	m_toplevel->setWindowTitle( tr( "%1 Demo" ).arg( VeyonCore::applicationName() ) );
	m_toplevel->setWindowIcon( QPixmap( QStringLiteral(":/core/icon64.png") ) );
	m_toplevel->setAttribute( Qt::WA_DeleteOnClose, false );

	if( fullscreen == false )
	{
		m_toplevel->setWindowFlags( Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint );
		m_toplevel->resize( QApplication::desktop()->availableGeometry( m_toplevel ).size() - QSize( 10, 30 ) );
	}

	m_vncView = new VncView( host, VeyonCore::config().demoServerPort(), m_toplevel, VncView::DemoMode );

	auto toplevelLayout = new QVBoxLayout;
	toplevelLayout->setMargin( 0 );
	toplevelLayout->setSpacing( 0 );
	toplevelLayout->addWidget( m_vncView );

	m_toplevel->setLayout( toplevelLayout );

	connect( m_toplevel, &QObject::destroyed, this, &DemoClient::viewDestroyed );
	connect( m_vncView, &VncView::sizeHintChanged, this, &DemoClient::resizeToplevelWidget );

	m_toplevel->move( 0, 0 );
	if( fullscreen )
	{
		m_toplevel->showFullScreen();
	}
	else
	{
		m_toplevel->show();
	}

	VeyonCore::platform().coreFunctions().raiseWindow( m_toplevel );

	VeyonCore::platform().coreFunctions().disableScreenSaver();
}



DemoClient::~DemoClient()
{
	VeyonCore::platform().coreFunctions().restoreScreenSaverSettings();

	delete m_toplevel;
}



void DemoClient::viewDestroyed( QObject* obj )
{
	// prevent double deletion of toplevel widget
	if( m_toplevel == obj )
	{
		m_toplevel = nullptr;
	}

	deleteLater();
}



void DemoClient::resizeToplevelWidget()
{
	if( m_toplevel->windowState() & Qt::WindowFullScreen )
	{
		m_vncView->resize( m_toplevel->size() );
	}
	else
	{
		m_toplevel->resize( m_vncView->sizeHint() );
	}
}
