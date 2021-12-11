/*
 * ComputerZoomWidget.cpp - fullscreen preview widget
 *
 * Copyright (c) 2021 Tobias Junghans <tobydox@veyon.io>
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

	currentScreen = -1;

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
		vCritical() << "KeyPress Event fired!";
		Q_EMIT keypressInComputerZoomWidget( );

		const auto screens = m_vncView->computerControlInterface()->screens();
		const auto key = dynamic_cast<QKeyEvent *>( event )->key();
		if ( screens.size() > 1 && ( key == Qt::Key_Tab || key == Qt::Key_Backtab ) )
		{
			if( key == Qt::Key_Tab )
			{
				vCritical() << "Qt::Key_Tab";
				if ( currentScreen < screens.size() - 1 )
				{
					currentScreen++;
					vCritical() << "Switching to next Screen: " << currentScreen << screens[currentScreen].name << screens[currentScreen].geometry;
				} else
				{
					currentScreen = -1;
					vCritical() << "Switching to Allscreen View Tab";
				}
			}

			if( key == Qt::Key_Backtab )
			{
				vCritical() << "Qt::Key_Backtab";
				if ( currentScreen == -1 )
				{
					currentScreen = screens.size()-1;
					vCritical() << "Switching to last Screen: " << currentScreen << screens[currentScreen].name << screens[currentScreen].geometry;
				} else if ( currentScreen > 0 )
				{
					currentScreen--;
					vCritical() << "Switching to previous Screen: " << currentScreen << screens[currentScreen].name << screens[currentScreen].geometry;
				} else
				{
					currentScreen = -1;
					vCritical() << "Switching to Allscreen View Backtab";
				}
			}

			showNormal();
			if ( currentScreen == -1)
			{
				m_vncView->setViewport( {} );
			}
			else
			{
				m_vncView->setViewport(screens[currentScreen].geometry);
			}
			setWindowState(Qt::WindowMaximized);
			return true;
		}

		vCritical() << "KeyPress Event forwarded";
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
		setWindowTitle( QStringLiteral( "%1 - %2" ).arg( m_vncView->computerControlInterface()->computer().name(),
														 VeyonCore::applicationName() ) );
	}
	else
	{
		setWindowTitle( QStringLiteral( "%1 - %2 - %3" ).arg( username,
															  m_vncView->computerControlInterface()->computer().name(),
															  VeyonCore::applicationName() ) );
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
