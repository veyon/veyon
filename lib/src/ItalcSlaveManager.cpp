/*
 * ItalcSlaveManager.cpp - ItalcSlaveManager which manages (GUI) slave apps
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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include "ItalcSlaveManager.h"
#include "ScreenLockSlaveLauncher.h"

const Ipc::Id ItalcSlaveManager::IdCoreServer = "CoreServer";
const Ipc::Id ItalcSlaveManager::IdDemoClient = "DemoClient";
const Ipc::Id ItalcSlaveManager::IdDemoServer = "DemoServer";
const Ipc::Id ItalcSlaveManager::IdMessageBox = "MessageBox";
const Ipc::Id ItalcSlaveManager::IdScreenLock = "ScreenLock";
const Ipc::Id ItalcSlaveManager::IdSystemTrayIcon = "SystemTrayIcon";

const Ipc::Command ItalcSlaveManager::DemoClient::StartDemo = "StartDemo";
const Ipc::Argument ItalcSlaveManager::DemoClient::MasterHost = "MasterHost";
const Ipc::Argument ItalcSlaveManager::DemoClient::FullScreen = "FullScreen";

const Ipc::Command ItalcSlaveManager::DemoServer::StartDemoServer = "StartDemoServer";
const Ipc::Argument ItalcSlaveManager::DemoServer::UserRole = "UserRole";
const Ipc::Argument ItalcSlaveManager::DemoServer::SourcePort = "SourcePort";
const Ipc::Argument ItalcSlaveManager::DemoServer::DestinationPort = "DestinationPort";

const Ipc::Command ItalcSlaveManager::DemoServer::UpdateAllowedHosts = "UpdateAllowedHosts";
const Ipc::Argument ItalcSlaveManager::DemoServer::AllowedHosts = "AllowedHosts";

const Ipc::Command ItalcSlaveManager::MessageBoxSlave::ShowMessageBox = "ShowMessageBox";
const Ipc::Argument ItalcSlaveManager::MessageBoxSlave::Text = "Text";

const Ipc::Command ItalcSlaveManager::SystemTrayIcon::SetToolTip = "SetToolTip";
const Ipc::Argument ItalcSlaveManager::SystemTrayIcon::ToolTipText = "ToolTipText";

const Ipc::Command ItalcSlaveManager::SystemTrayIcon::ShowMessage = "ShowMessage";
const Ipc::Argument ItalcSlaveManager::SystemTrayIcon::Title = "Title";
const Ipc::Argument ItalcSlaveManager::SystemTrayIcon::Text = "Text";



ItalcSlaveManager::ItalcSlaveManager() :
	Ipc::Master( QCoreApplication::applicationDirPath() +
					QDir::separator() + "ica" ),
	m_demoServerMaster( this )
{
}




ItalcSlaveManager::~ItalcSlaveManager()
{
}




void ItalcSlaveManager::startDemo( const QString &masterHost, bool fullscreen )
{
	createSlave( IdDemoClient );
	sendMessage( IdDemoClient,
					Ipc::Msg( DemoClient::StartDemo ).
						addArg( DemoClient::MasterHost, masterHost ).
						addArg( DemoClient::FullScreen, fullscreen ) );
}




void ItalcSlaveManager::stopDemo()
{
	stopSlave( IdDemoClient );
}




void ItalcSlaveManager::lockDisplay()
{
	createSlave( IdScreenLock,
					new ScreenLockSlaveLauncher( applicationFilePath() ) );
}




void ItalcSlaveManager::unlockDisplay()
{
	stopSlave( IdScreenLock );
}




void ItalcSlaveManager::messageBox( const QString &msg )
{
	if( !isSlaveRunning( IdMessageBox ) )
	{
		createSlave( IdMessageBox );
	}
	sendMessage( IdMessageBox,
					Ipc::Msg( MessageBoxSlave::ShowMessageBox ).
						addArg( MessageBoxSlave::Text, msg ) );
}




void ItalcSlaveManager::systemTrayMessage( const QString &title,
										const QString &msg )
{
	if( !isSlaveRunning( IdSystemTrayIcon ) )
	{
		createSlave( IdSystemTrayIcon );
	}
	sendMessage( IdSystemTrayIcon,
					Ipc::Msg( SystemTrayIcon::ShowMessage ).
						addArg( SystemTrayIcon::Title, title ).
						addArg( SystemTrayIcon::Text, msg ) );
}




void ItalcSlaveManager::setSystemTrayToolTip( const QString &tooltip )
{
	if( !isSlaveRunning( IdSystemTrayIcon ) )
	{
		createSlave( IdSystemTrayIcon );
	}
	sendMessage( IdSystemTrayIcon,
					Ipc::Msg( SystemTrayIcon::SetToolTip ).
						addArg( SystemTrayIcon::ToolTipText, tooltip ) );
}




ItalcSlaveManager::AccessDialogResult ItalcSlaveManager::showAccessDialog( const QString &host )
{
}




bool ItalcSlaveManager::handleMessage( const Ipc::Msg &m )
{
	return false;
}

