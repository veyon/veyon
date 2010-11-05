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

#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "MainWindow.h"


MainWindow::MainWindow() :
	QMainWindow(),
	ui( new Ui::MainWindow )
{
	ui->setupUi( this );

	setWindowTitle( tr( "iTALC Management Console %1" ).arg( ITALC_VERSION ) );

#define LOAD_AND_CONNECT_PROPERTY(property,type,widget,setvalue,signal,slot)		\
			ui->widget->setvalue( ImcCore::config->property() );  					\
			connect( ui->widget, SIGNAL(signal(type)),									\
						ImcCore::config, SLOT(slot(type)) );

#define LOAD_AND_CONNECT_PROPERTY_BUTTON(property,slot)								\
			LOAD_AND_CONNECT_PROPERTY(property,bool,property,setChecked,toggled,slot)

#define LOAD_AND_CONNECT_PROPERTY_LINEEDIT(property,slot)							\
			LOAD_AND_CONNECT_PROPERTY(property,const QString &,property,setText,toggled,slot)

#define LOAD_AND_CONNECT_PROPERTY_SPINBOX(property,slot)							\
			LOAD_AND_CONNECT_PROPERTY(property,int,property,setValue,toggled,slot)

#define LOAD_AND_CONNECT_PROPERTY_COMBOBOX(property,slot)							\
			LOAD_AND_CONNECT_PROPERTY(property,int,property,setCurrentIndex,toggled,slot)

	// iTALC Service
	LOAD_AND_CONNECT_PROPERTY_BUTTON(isTrayIconHidden,setTrayIconHidden);
	LOAD_AND_CONNECT_PROPERTY_BUTTON(autostartService,setServiceAutostart);

	LOAD_AND_CONNECT_PROPERTY_LINEEDIT(serviceArguments,setServiceArguments);

	// Logging
	LOAD_AND_CONNECT_PROPERTY_COMBOBOX(logLevel,setLogLevel);
	LOAD_AND_CONNECT_PROPERTY_BUTTON(limittedLogFileSize,setLimittedLogFileSize);
	LOAD_AND_CONNECT_PROPERTY_SPINBOX(logFileSizeLimit,setLogFileSizeLimit);
	LOAD_AND_CONNECT_PROPERTY_LINEEDIT(logFileDirectory,setLogFileDirectory);

	// VNC Server
	LOAD_AND_CONNECT_PROPERTY_BUTTON(vncCaptureLayeredWindows,setVncCaptureLayeredWindows);
	LOAD_AND_CONNECT_PROPERTY_BUTTON(vncPollFullScreen,setVncPollFullScreen);
	LOAD_AND_CONNECT_PROPERTY_BUTTON(vncLowAccuracy,setVncLowAccuracy);

	// Demo server
	LOAD_AND_CONNECT_PROPERTY_BUTTON(isDemoServerMultithreaded,setDemoServerMultithreaded);

	// Network
	LOAD_AND_CONNECT_PROPERTY_SPINBOX(coreServerPort,setCoreServerPort);
	LOAD_AND_CONNECT_PROPERTY_SPINBOX(demoServerPort,setDemoServerPort);
	LOAD_AND_CONNECT_PROPERTY_BUTTON(isFirewallExceptionEnabled,setFirewallExceptionEnabled);
}




MainWindow::~MainWindow()
{
}

