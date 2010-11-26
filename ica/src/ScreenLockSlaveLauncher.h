/*
 * ScreenLockSlaveLauncher.h - a SlaveLauncher for the ScreenLockSlave
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

#ifndef _SCREEN_LOCK_SLAVE_LAUNCHER_H
#define _SCREEN_LOCK_SLAVE_LAUNCHER_H

#include <italcconfig.h>

#include "Ipc/SlaveLauncher.h"

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#else
#include "Ipc/QtSlaveLauncher.h"
#endif

class ScreenLockSlaveLauncher : public Ipc::SlaveLauncher
{
public:
	ScreenLockSlaveLauncher( const QString &applicationFilePath );
	~ScreenLockSlaveLauncher();

	virtual void start( const QStringList &arguments );
	virtual void stop();
	virtual bool isRunning();


private:
#ifdef ITALC_BUILD_WIN32
	HDESK m_newDesktop;
	HDESK m_origThreadDesktop;
	HDESK m_origInputDesktop;
	HANDLE m_lockProcess;
#else
	Ipc::QtSlaveLauncher *m_launcher;
#endif
};


#endif

