/*
 *  MainToolBar.cpp - MainToolBar for MainWindow
 *
 *  Copyright (c) 2007-2023 Tobias Junghans <tobydox@veyon.io>
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

#include <QGuiApplication>
#include <QMenu>

#include "MainToolBar.h"
#include "MainWindow.h"
#include "VeyonMaster.h"
#include "ToolButton.h"
#include "UserConfig.h"


MainToolBar::MainToolBar( QWidget* parent ) :
	QToolBar( tr( "Configuration" ), parent ),
	m_mainWindow( dynamic_cast<MainWindow *>( parent ) )
{
	setIconSize(QSize(48, 48) / qGuiApp->devicePixelRatio());

	ToolButton::setToolTipsDisabled( m_mainWindow->masterCore().userConfig().noToolTips() );
	ToolButton::setIconOnlyMode( m_mainWindow, m_mainWindow->masterCore().userConfig().toolButtonIconOnlyMode() );
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
