/*
 * ComputerZoomWidget.cpp - fullscreen preview widget
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
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
#include <QIcon>
#include <QLayout>

#include "ComputerControlInterface.h"
#include "ComputerZoomWidget.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "PlatformCoreFunctions.h"
#include "VncViewWidget.h"


ComputerZoomWidget::ComputerZoomWidget( const ComputerControlInterface::Pointer& computerControlInterface ) :
	QWidget( nullptr ),
	m_vncView( new VncViewWidget( computerControlInterface, {}, this ) )
{
	const auto openOnMasterScreen = VeyonCore::config().showFeatureWindowsOnSameScreen();
	const auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	const auto masterWindow = master->mainWindow();
	if( master && openOnMasterScreen )
	{
		move( masterWindow->x(), masterWindow->y() );
	} else {
		move( 0, 0 );
	}

	updateComputerZoomWidgetTitle();
	connect( m_computerControlInterface.data(), &ComputerControlInterface::userChanged, this, &ComputerZoomWidget::updateComputerZoomWidgetTitle );

	setWindowIcon( QPixmap( QStringLiteral(":/remoteaccess/kmag.png") ) );
	setAttribute( Qt::WA_DeleteOnClose, true );

	m_vncView->move( 0, 0 );

	showMaximized();
	VeyonCore::platform().coreFunctions().raiseWindow( this, false );
}



ComputerZoomWidget::~ComputerZoomWidget()
{
	delete m_vncView;
}



void ComputerZoomWidget::updateComputerZoomWidgetTitle()
{
	if ( m_computerControlInterface->userFullName().isEmpty() )
	{
		setWindowTitle( tr( "%1 - %2 Computer Zoom Widget" ).arg( m_computerControlInterface->computer().name(),
														   VeyonCore::applicationName() ) );
	} else
	{
		setWindowTitle( tr( "%1 - %2 - %3 Computer Zoom Widget" ).arg( m_computerControlInterface->userFullName(),
																  m_computerControlInterface->computer().name(),
																  VeyonCore::applicationName() ) );
	}
}
