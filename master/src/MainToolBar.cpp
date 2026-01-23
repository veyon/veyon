/*
 *  MainToolBar.cpp - MainToolBar for MainWindow
 *
 *  Copyright (c) 2007-2026 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
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

#include <QContextMenuEvent>
#include <QLayout>
#include <QMenu>
#include <QToolButton>

#include "MainToolBar.h"
#include "MainWindow.h"
#include "VeyonMaster.h"
#include "ToolButton.h"
#include "UserConfig.h"


MainToolBar::MainToolBar( QWidget* parent ) :
	QToolBar( tr( "Configuration" ), parent ),
	m_mainWindow(qobject_cast<MainWindow *>(parent)),
	m_layout(findChild<QLayout *>())
{
	setExpanded();
	setIconSize(QSize(32, 32));

	ToolButton::setToolTipsDisabled( m_mainWindow->masterCore().userConfig().noToolTips() );
	ToolButton::setIconOnlyMode( m_mainWindow, m_mainWindow->masterCore().userConfig().toolButtonIconOnlyMode() );

	connect(this, &QToolBar::orientationChanged, this, &MainToolBar::setExpanded);
}



bool MainToolBar::event(QEvent* event)
{
	if (event->type() == QEvent::Leave)
	{
		return true;
	}

	if (event->type() == QEvent::Resize)
	{
		updateMinimumSize();
	}

	return QToolBar::event(event);
}



void MainToolBar::setExpanded()
{
	if (m_layout)
	{
		QMetaObject::invokeMethod(m_layout, "setExpanded", Qt::DirectConnection, Q_ARG(bool, true));

		const auto button = findChild<QToolButton *>(QStringLiteral("qt_toolbar_ext_button"));
		if (button)
		{
			button->setFixedSize(0, 0);
		}
	}
}



void MainToolBar::updateMinimumSize()
{
	if (m_layout)
	{
		const auto newSize = m_layout->contentsRect().size();
		if (orientation() == Qt::Horizontal)
		{
			setMinimumSize(0, newSize.height());
		}
		else
		{
			setMinimumSize(newSize.width(), 0);
		}
	}
}



void MainToolBar::contextMenuEvent( QContextMenuEvent* event )
{
	QMenu menu( this );

	auto toolTipAction = menu.addAction(tr("Disable tooltips"), this, &MainToolBar::toggleToolTips);
	toolTipAction->setCheckable( true );
	toolTipAction->setChecked( m_mainWindow->masterCore().userConfig().noToolTips() );

	auto iconModeAction = menu.addAction( tr( "Show icons only" ), this, &MainToolBar::toggleIconMode );
	iconModeAction->setCheckable( true );
	iconModeAction->setChecked( m_mainWindow->masterCore().userConfig().toolButtonIconOnlyMode() );

	menu.exec( event->globalPos() );
}



void MainToolBar::toggleToolTips()
{
	bool newToolTipState = !m_mainWindow->masterCore().userConfig().noToolTips();

	ToolButton::setToolTipsDisabled( newToolTipState );
	m_mainWindow->masterCore().userConfig().setNoToolTips( newToolTipState );
}



void MainToolBar::toggleIconMode()
{
	bool newToolButtonIconMode = !m_mainWindow->masterCore().userConfig().toolButtonIconOnlyMode();

	ToolButton::setIconOnlyMode( m_mainWindow, newToolButtonIconMode );
	m_mainWindow->masterCore().userConfig().setToolButtonIconOnlyMode( newToolButtonIconMode );
}
