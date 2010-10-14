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
#include "ItalcCore.h"

#include "DemoClientSlave.h"
#include "MessageBoxSlave.h"
#include "ScreenLockSlave.h"
#include "ScreenLockSlaveLauncher.h"
#include "SystemTrayIconSlave.h"


MasterProcess::MasterProcess() :
	Ipc::Master()
{
}




MasterProcess::~MasterProcess()
{
}




void MasterProcess::startDemo( const QString &masterHost, bool fullscreen )
{
	createSlave( ItalcCore::Ipc::IdDemoClient );
	sendMessage( ItalcCore::Ipc::IdDemoClient,
					Ipc::Msg( DemoClientSlave::StartDemo ).
						addArg( DemoClientSlave::MasterHost, masterHost ).
						addArg( DemoClientSlave::FullScreen, fullscreen ) );
}




void MasterProcess::stopDemo()
{
	stopSlave( ItalcCore::Ipc::IdDemoClient );
}




void MasterProcess::lockDisplay()
{
	createSlave( ItalcCore::Ipc::IdScreenLock, new ScreenLockSlaveLauncher );
}




void MasterProcess::unlockDisplay()
{
	stopSlave( ItalcCore::Ipc::IdScreenLock );
}




void MasterProcess::messageBox( const QString &msg )
{
	if( !isSlaveRunning( ItalcCore::Ipc::IdMessageBox ) )
	{
		createSlave( ItalcCore::Ipc::IdMessageBox );
	}
	sendMessage( ItalcCore::Ipc::IdMessageBox,
					Ipc::Msg( MessageBoxSlave::ShowMessageBox ).
						addArg( MessageBoxSlave::Text, msg ) );
}




void MasterProcess::systemTrayMessage( const QString &title,
										const QString &msg )
{
	if( !isSlaveRunning( ItalcCore::Ipc::IdSystemTrayIcon ) )
	{
		createSlave( ItalcCore::Ipc::IdSystemTrayIcon );
	}
	sendMessage( ItalcCore::Ipc::IdSystemTrayIcon,
					Ipc::Msg( SystemTrayIconSlave::ShowMessage ).
						addArg( SystemTrayIconSlave::Title, title ).
						addArg( SystemTrayIconSlave::Text, msg ) );
}




void MasterProcess::setSystemTrayToolTip( const QString &tooltip )
{
	if( !isSlaveRunning( ItalcCore::Ipc::IdSystemTrayIcon ) )
	{
		createSlave( ItalcCore::Ipc::IdSystemTrayIcon );
	}
	sendMessage( ItalcCore::Ipc::IdSystemTrayIcon,
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

