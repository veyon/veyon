/*
 * ItalcSlaveManager.h - ItalcSlaveManager which manages (GUI) slave apps
 *
 * Copyright (c) 2010-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef ITALC_SLAVE_MANAGER_H
#define ITALC_SLAVE_MANAGER_H

#include "Ipc/Master.h"


class ItalcSlaveManager : protected Ipc::Master
{
	Q_OBJECT
public:
	ItalcSlaveManager();
	virtual ~ItalcSlaveManager();

	static const Ipc::Id IdCoreServer;
	static const Ipc::Id IdAccessDialog;
	static const Ipc::Id IdInputLock;

	struct AccessDialog
	{
		static const Ipc::Command Ask;
		static const Ipc::Argument User;
		static const Ipc::Argument Host;
		static const Ipc::Argument ChoiceFlags;
		static const Ipc::Command ReportChoice;
	} ;

	void lockInput();
	void unlockInput();

	int execAccessDialog( const QString &user, const QString &host,
							int choiceFlags );

	int slaveStateFlags();


private:
	virtual void createSlave( const Ipc::Id &id,
								Ipc::SlaveLauncher *slaveLauncher = NULL );

	virtual bool handleMessage( const Ipc::Id &slaveId, const Ipc::Msg &m );

	volatile int m_accessDialogChoice;

} ;


#endif
