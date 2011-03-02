/*
 * LocalSystemIca.cpp - namespace LocalSystem, providing an interface for
 *                      transparent usage of operating-system-specific
 *                      functions
 *
 * Copyright (c) 2007-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef ITALC_BUILD_WIN32
#define _WIN32_WINNT 0x0501
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#ifdef ITALC_BUILD_WIN32

#include <windows.h>
#include <psapi.h>
#include <lm.h>
#include <shlobj.h>
#include <winable.h>

#if _WIN32_WINNT >= 0x500
#define SHUTDOWN_FLAGS (EWX_FORCE | EWX_FORCEIFHUNG)
#else
#define SHUTDOWN_FLAGS (EWX_FORCE)
#endif

#if _WIN32_WINNT >= 0x501
#include <reason.h>
#define SHUTDOWN_REASON SHTDN_REASON_MAJOR_OTHER
#else
#define SHUTDOWN_REASON 0
#endif

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include "LocalSystem.h"
#include "SystemKeyTrapper.h"

#else

#include "LocalSystem.h"

#ifdef ITALC_HAVE_X11
#include <X11/Xlib.h>
#else
#define KeySym int
#endif

#ifdef ITALC_HAVE_PWD_H
#include <pwd.h>
#endif

#include "rfb/rfb.h"

extern "C"
{
#include "keyboard.h"
}


extern rfbClientPtr __client;

static inline void pressKey( int _key, bool _down )
{
	if( !__client )
	{
		return;
	}
	keyboard( _down, _key, __client );
}


#endif


namespace LocalSystem
{


/*void disableLocalInputs( bool _disabled )
{
#if 0
#ifdef ITALC_BUILD_WIN32
	static systemKeyTrapper * __skt = NULL;
	if( __localInputsDisabled != _disabled )
	{
		__localInputsDisabled = _disabled;
		if( _disabled && __skt == NULL )
		{
			__skt = new systemKeyTrapper();
		}
		else
		{
			delete __skt;
			__skt = NULL;
		}
	}
#else
#warning TODO
#endif
#endif
}*/



void powerDown( void )
{
#ifdef ITALC_BUILD_WIN32
	enablePrivilege( SE_SHUTDOWN_NAME, TRUE );
	ExitWindowsEx( EWX_POWEROFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
	enablePrivilege( SE_SHUTDOWN_NAME, FALSE );
#else
	if( LocalSystem::User::loggedOnUser().name() == "root" )
	{
		QProcess::startDetached( "poweroff" );
	}
	else
	{
		QProcess::startDetached( "gdm-signal -h" ); // Gnome shutdown
		QProcess::startDetached( "gnome-session-save --silent --kill" ); // Gnome logout
		QProcess::startDetached( "dcop ksmserver ksmserver logout 0 2 0" ); // KDE shutdown
		QProcess::startDetached( "qdbus org.kde.ksmserver /KSMServer logout 0 2 0" ); // KDE4 shutdown
	}
#endif
}




void logoutUser( void )
{
#ifdef ITALC_BUILD_WIN32
	ExitWindowsEx( EWX_LOGOFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
#else
	QProcess::startDetached( "gnome-session-save --silent --kill" ); // Gnome logout
	QProcess::startDetached( "dcop ksmserver ksmserver logout 0 0 0" ); // KDE logout
	QProcess::startDetached( "qdbus org.kde.ksmserver /KSMServer logout 0 0 0" ); // KDE4 logout
#endif
}



void reboot( void )
{
#ifdef ITALC_BUILD_WIN32
	enablePrivilege( SE_SHUTDOWN_NAME, TRUE );
	ExitWindowsEx( EWX_REBOOT | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
	enablePrivilege( SE_SHUTDOWN_NAME, FALSE );
#else
	if( LocalSystem::User::loggedOnUser().name() == "root" )
	{
		QProcess::startDetached( "poweroff" );
	}
	else
	{
		QProcess::startDetached( "gdm-signal -r" ); // Gnome reboot
		QProcess::startDetached( "gnome-session-save --silent --kill" ); // Gnome logout
		QProcess::startDetached( "dcop ksmserver ksmserver logout 0 1 0" ); // KDE reboot
		QProcess::startDetached( "qdbus org.kde.ksmserver /KSMServer logout 0 1 0" ); // KDE4 reboot
	}
#endif
}




} // end of namespace LocalSystem

