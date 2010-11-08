/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <italcconfig.h>

#include "Configuration/UiMapping.h"

#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "MainWindow.h"


MainWindow::MainWindow() :
	QMainWindow(),
	ui( new Ui::MainWindow )
{
	ui->setupUi( this );

	setWindowTitle( tr( "iTALC Management Console %1" ).arg( ITALC_VERSION ) );

	// reset all widget's values to current configuration
	reset();

	// connect widget signals to configuration property write methods
	FOREACH_ITALC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}




MainWindow::~MainWindow()
{
}



void MainWindow::reset()
{
	FOREACH_ITALC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY)
}




void MainWindow::apply()
{
}

