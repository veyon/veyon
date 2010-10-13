/*
 * MasterProcess.cpp - MasterProcess which manages (GUI) slave apps
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#include "MasterProcess.h"

#include "DemoClientSlave.h"
#include "MessageBoxSlave.h"
#include "ScreenLockSlave.h"
#include "SystemTrayIconSlave.h"

const Ipc::Id MasterProcess::IdCoreServer = "CoreServer";
const Ipc::Id MasterProcess::IdDemoClient = "DemoClient";
const Ipc::Id MasterProcess::IdMessageBox = "MessageBox";
const Ipc::Id MasterProcess::IdScreenLock = "ScreenLock";
const Ipc::Id MasterProcess::IdSystemTrayIcon = "SystemTrayIcon";


MasterProcess::MasterProcess() :
	Ipc::Master()
{
}




MasterProcess::~MasterProcess()
{
}




void MasterProcess::startDemo( const QString &masterHost, bool fullscreen)
{
	createSlave( IdDemoClient );
	sendMessage( IdDemoClient,
					Ipc::Msg( DemoClientSlave::StartDemo ).
						addArg( DemoClientSlave::MasterHost, masterHost ).
						addArg( DemoClientSlave::FullScreen, fullscreen ) );
}




void MasterProcess::stopDemo()
{
	stopSlave( IdDemoClient );
}




void MasterProcess::lockDisplay()
{
	createSlave( IdScreenLock );
}




void MasterProcess::unlockDisplay()
{
	stopSlave( IdScreenLock );
}




void MasterProcess::messageBox( const QString &msg )
{
	if( !isSlaveRunning( IdMessageBox ) )
	{
		createSlave( IdMessageBox );
	}
	sendMessage( IdMessageBox,
					Ipc::Msg( MessageBoxSlave::ShowMessageBox ).
						addArg( MessageBoxSlave::Text, msg ) );
}




void MasterProcess::systemTrayMessage( const QString &title,
										const QString &msg )
{
	if( !isSlaveRunning( IdSystemTrayIcon ) )
	{
		createSlave( IdSystemTrayIcon );
	}
	sendMessage( IdSystemTrayIcon,
					Ipc::Msg( SystemTrayIconSlave::ShowMessage ).
						addArg( SystemTrayIconSlave::Title, title ).
						addArg( SystemTrayIconSlave::Text, msg ) );
}




void MasterProcess::setSystemTrayToolTip( const QString &tooltip )
{
	if( !isSlaveRunning( IdSystemTrayIcon ) )
	{
		createSlave( IdSystemTrayIcon );
	}
	sendMessage( IdSystemTrayIcon,
					Ipc::Msg( SystemTrayIconSlave::SetToolTip ).
						addArg( SystemTrayIconSlave::ToolTipText, tooltip ) );
}




MasterProcess::AccessDialogResult MasterProcess::showAccessDialog( const QString &host )
{
}




bool MasterProcess::handleMessage( const Ipc::Msg &m )
{
	return false;
}

