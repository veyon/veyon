/*
 * ScreenLockSlaveLauncher.cpp - a SlaveLauncher for the ScreenLockSlave
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
#include <QtCore/QStringList>

#include "ScreenLockSlaveLauncher.h"


ScreenLockSlaveLauncher::ScreenLockSlaveLauncher() :
	Ipc::SlaveLauncher(),
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

	char desktopName[] = "ScreenLockSlaveDesktop";
	m_newDesktop = CreateDesktop( desktopName, NULL, NULL, 0, GENERIC_ALL, NULL );
	SetThreadDesktop( m_newDesktop );

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.lpDesktop = desktopName;
	ZeroMemory( &pi, sizeof(pi) );

	char * cmdline = qstrdup( QString( QCoreApplication::applicationFilePath() +
						" " + arguments.join( " " ) ).toAscii().constData() );
	CreateProcess( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
	delete[] cmdline;

	m_lockProcess = pi.hProcess;

	// sleep a bit so switch to desktop with loaded screen locker runs smoothly
	Sleep( 2000 );

	SwitchDesktop( m_newDesktop );
#else
	m_launcher = new Ipc::QtSlaveLauncher;
	m_launcher->start( arguments );
#endif
}



void ScreenLockSlaveLauncher::stop()
{
#ifdef ITALC_BUILD_WIN32
	SwitchDesktop( m_origInputDesktop );
	SetThreadDesktop( m_origThreadDesktop );

	TerminateProcess( m_lockProcess, 0 );
	CloseDesktop( m_newDesktop );
#else
	m_launcher->stop();
	delete m_launcher;
#endif
}



bool ScreenLockSlaveLauncher::isRunning() const
{
#ifdef ITALC_BUILD_WIN32
	return true;		// TODO
#else
	return m_launcher && m_launcher->isRunning();
#endif
}

