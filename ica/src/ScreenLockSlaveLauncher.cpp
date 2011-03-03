/*
 * ScreenLockSlaveLauncher.cpp - a SlaveLauncher for the ScreenLockSlave
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

#include <QtCore/QStringList>

#include "ScreenLockSlaveLauncher.h"
#include "LocalSystem.h"
#include "Ipc/QtSlaveLauncher.h"


ScreenLockSlaveLauncher::ScreenLockSlaveLauncher(
										const QString &applicationFilePath ) :
	Ipc::SlaveLauncher( applicationFilePath ),
#ifdef ITALC_BUILD_WIN32
	m_newDesktop( NULL ),
	m_origThreadDesktop( NULL ),
	m_origInputDesktop( NULL ),
	m_lockProcess( NULL )
#else
	m_launcher( NULL )
#endif
{
}



ScreenLockSlaveLauncher::~ScreenLockSlaveLauncher()
{
	// base class destructor calls stop()
}



void ScreenLockSlaveLauncher::start( const QStringList &arguments )
{
	stop();

#ifdef ITALC_BUILD_WIN32

	m_origThreadDesktop = GetThreadDesktop( GetCurrentThreadId() );
	m_origInputDesktop = OpenInputDesktop( 0, FALSE, DESKTOP_SWITCHDESKTOP );

	char *desktopName = qstrdup( LocalSystem::Desktop::screenLockDesktop().
												name().toUtf8().constData() );
	m_newDesktop = CreateDesktop( desktopName, NULL, NULL, 0, GENERIC_ALL, NULL );

	LocalSystem::User user = LocalSystem::User::loggedOnUser();
	LocalSystem::Process proc(
				LocalSystem::Process::findProcessId( QString(), -1, &user ) );

	m_lockProcess =
		proc.runAsUser( applicationFilePath() + " " + arguments.join( " " ),
							LocalSystem::Desktop::screenLockDesktop().name() );

	delete[] desktopName;

	// sleep a bit so switch to desktop with loaded screen locker runs smoothly
	Sleep( 2000 );

	SwitchDesktop( m_newDesktop );

#else

	m_launcher = new Ipc::QtSlaveLauncher( applicationFilePath() );
	m_launcher->start( arguments );

#endif
}



void ScreenLockSlaveLauncher::stop()
{
#ifdef ITALC_BUILD_WIN32
	if( m_lockProcess )
	{
		SwitchDesktop( m_origInputDesktop );
		if( WaitForSingleObject( m_lockProcess, 10000 ) == WAIT_TIMEOUT )
		{
			qWarning( "ScreenLockSlaveLauncher: calling TerminateProcess()" );
			TerminateProcess( m_lockProcess, 0 );
		}
		CloseDesktop( m_newDesktop );

		m_lockProcess = NULL;
	}
#else
	if( m_launcher )
	{
		m_launcher->stop();
		delete m_launcher;
		m_launcher = NULL;
	}
#endif
}



bool ScreenLockSlaveLauncher::isRunning()
{
#ifdef ITALC_BUILD_WIN32
	return true;		// TODO
#else
	return m_launcher && m_launcher->isRunning();
#endif
}

