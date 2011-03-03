/*
 * ItalcSlaveManager.h - ItalcSlaveManager which manages (GUI) slave apps
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _ITALC_SLAVE_MANAGER_H
#define _ITALC_SLAVE_MANAGER_H

#include "Ipc/Master.h"
#include "DemoServerMaster.h"


class ItalcSlaveManager : protected Ipc::Master
{
	Q_OBJECT
public:
	ItalcSlaveManager();
	virtual ~ItalcSlaveManager();

	static const Ipc::Id IdCoreServer;
	static const Ipc::Id IdAccessDialog;
	static const Ipc::Id IdDemoClient;
	static const Ipc::Id IdDemoServer;
	static const Ipc::Id IdMessageBox;
	static const Ipc::Id IdScreenLock;
	static const Ipc::Id IdInputLock;
	static const Ipc::Id IdSystemTrayIcon;

	struct AccessDialog
	{
		static const Ipc::Command Ask;
		static const Ipc::Argument User;
		static const Ipc::Argument Host;
		static const Ipc::Argument ChoiceFlags;
		static const Ipc::Command ReportChoice;
	} ;

	struct SystemTrayIcon
	{
		static const Ipc::Command ShowMessage;
		static const Ipc::Argument Title;
		static const Ipc::Argument Text;
		static const Ipc::Command SetToolTip;
		static const Ipc::Argument ToolTipText;
	} ;

	struct DemoClient
	{
		static const Ipc::Command StartDemo;
		static const Ipc::Argument MasterHost;
		static const Ipc::Argument FullScreen;
	} ;

	struct DemoServer
	{
		static const Ipc::Command StartDemoServer;
		static const Ipc::Argument SourcePort;
		static const Ipc::Argument DestinationPort;
		static const Ipc::Argument CommonSecret;

		static const Ipc::Command UpdateAllowedHosts;
		static const Ipc::Argument AllowedHosts;
	} ;

	struct MessageBoxSlave
	{
		static const Ipc::Command ShowMessageBox;
		static const Ipc::Argument Text;
	} ;

	DemoServerMaster *demoServerMaster()
	{
		return &m_demoServerMaster;
	}

	void startDemo( const QString &masterHost, bool fullscreen);
	void stopDemo();

	void lockScreen();
	void unlockScreen();

	void lockInput();
	void unlockInput();

	void messageBox( const QString &msg );
	void setSystemTrayToolTip( const QString &tooltip );
	void systemTrayMessage( const QString &title, const QString &msg );

	int execAccessDialog( const QString &user, const QString &host,
							int choiceFlags );

	int slaveStateFlags();


private:
	virtual void createSlave( const Ipc::Id &id,
								Ipc::SlaveLauncher *slaveLauncher = NULL );

	virtual bool handleMessage( const Ipc::Id &slaveId, const Ipc::Msg &m );

	DemoServerMaster m_demoServerMaster;
	volatile int m_accessDialogChoice;

	friend class DemoServerMaster;

} ;


#endif
