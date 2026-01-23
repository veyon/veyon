/*
 * ComputerZoomWidget.cpp - fullscreen preview widget
 *
 * Copyright (c) 2021-2026 Tobias Junghans <tobydox@veyon.io>
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
#include <QKeyEvent>

#include "ComputerZoomWidget.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "PlatformCoreFunctions.h"
#include "VncViewWidget.h"


ComputerZoomWidget::ComputerZoomWidget( const ComputerControlInterface::Pointer& computerControlInterface ) :
	QWidget(),
	m_vncView( new VncViewWidget( computerControlInterface, {}, this ) )
{
	QApplication::setOverrideCursor(Qt::BlankCursor);

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
	connect( m_vncView->computerControlInterface().data(), &ComputerControlInterface::userChanged,
			 this, &ComputerZoomWidget::updateComputerZoomWidgetTitle );

	setWindowIcon( QPixmap( QStringLiteral(":/remoteaccess/kmag.png") ) );
	setAttribute( Qt::WA_DeleteOnClose, true );

	m_vncView->move( 0, 0 );
	m_vncView->installEventFilter( this );
	connect( m_vncView, &VncViewWidget::sizeHintChanged, this, &ComputerZoomWidget::updateSize );

	setWindowState(Qt::WindowMaximized);
	VeyonCore::platform().coreFunctions().raiseWindow( this, false );

	show();
}



ComputerZoomWidget::~ComputerZoomWidget()
{
	delete m_vncView;
}



bool ComputerZoomWidget::eventFilter( QObject* object, QEvent* event )
{
	if( event->type() == QEvent::KeyPress )
	{
		Q_EMIT keypressInComputerZoomWidget( );

		const auto screens = m_vncView->computerControlInterface()->screens();
		const auto key = static_cast<QKeyEvent *>( event )->key();
		if ( screens.size() > 1 && ( key == Qt::Key_Tab || key == Qt::Key_Backtab ) )
		{
			if( key == Qt::Key_Tab )
			{
				if ( m_currentScreen < screens.size() - 1 )
				{
					m_currentScreen++;
				} else
				{
					m_currentScreen = -1;
				}
			}

			if( key == Qt::Key_Backtab )
			{
				if ( m_currentScreen == -1 )
				{
					m_currentScreen = screens.size()-1;
				} else if ( m_currentScreen > 0 )
				{
					m_currentScreen--;
				} else
				{
					m_currentScreen = -1;
				}
			}

			showNormal();
			if ( m_currentScreen == -1)
			{
				m_vncView->setViewport( {} );
			}
			else
			{
				m_vncView->setViewport(screens[m_currentScreen].geometry);
			}
			setWindowState(Qt::WindowMaximized);
			return true;
		}

		return false;
	}

	return QObject::eventFilter(object, event);
}



void ComputerZoomWidget::updateComputerZoomWidgetTitle()
{
	const auto username = m_vncView->computerControlInterface()->userFullName().isEmpty() ?
							  m_vncView->computerControlInterface()->userLoginName() :
							  m_vncView->computerControlInterface()->userFullName();

	if (username.isEmpty())
	{
		setWindowTitle(QStringLiteral("%1 - Veyon").arg(m_vncView->computerControlInterface()->computerName()));
	}
	else
	{
		setWindowTitle(QStringLiteral( "%1 - %2 - Veyon").arg(username,
															  m_vncView->computerControlInterface()->computerName()));
	}
}



void ComputerZoomWidget::resizeEvent( QResizeEvent* event )
{
	m_vncView->resize( size() );

	QWidget::resizeEvent( event );
}



void ComputerZoomWidget::updateSize()
{
	if( !( windowState() & Qt::WindowFullScreen ) &&
			m_vncView->sizeHint().isEmpty() == false )
	{
		resize( m_vncView->sizeHint() );
	}
}



void ComputerZoomWidget::closeEvent( QCloseEvent* event )
{
	m_vncView->setViewport({});
	QApplication::restoreOverrideCursor();
}
