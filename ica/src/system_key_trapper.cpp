/*
 * system_key_trapper.cpp - class for trapping system-keys and -key-sequences
 *                          such as Alt+Ctrl+Del, Alt+Tab etc.
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#define XK_KOREAN
#include "rfb/keysym.h"
#include "system_key_trapper.h"

#include <QtCore/QList>
#include <QtCore/QTimer>

static QMutex __trapped_keys_mutex;
static QList<systemKeyTrapper::trappedKeys> __trapped_keys;

QMutex systemKeyTrapper::s_refCntMutex;
int systemKeyTrapper::s_refCnt;


#if BUILD_WIN32

// some code for disabling system's hotkeys such as Alt+Ctrl+Del, Alt+Tab,
// Ctrl+Esc, Alt+Esc, Windows-key etc. - otherwise locking wouldn't be very
// effective... ;-)
#define _WIN32_WINNT 0x0500 // for KBDLLHOOKSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winuser.h>

#include "inject.h"


HHOOK g_hHookKbdLL = NULL; // hook handle


LRESULT CALLBACK TaskKeyHookLL( int nCode, WPARAM wp, LPARAM lp )
{
	KBDLLHOOKSTRUCT *pkh = (KBDLLHOOKSTRUCT *) lp;
	static QList<systemKeyTrapper::trappedKeys> pressed;
	if( nCode == HC_ACTION )
	{
		BOOL bCtrlKeyDown = GetAsyncKeyState( VK_CONTROL ) >>
						( ( sizeof( SHORT ) * 8 ) - 1 );
		QMutexLocker m( &__trapped_keys_mutex );

		systemKeyTrapper::trappedKeys key = systemKeyTrapper::None;
		if( pkh->vkCode == VK_ESCAPE && bCtrlKeyDown )
		{
			key = systemKeyTrapper::CtrlEsc;
		}
		else if( pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN )
		{
			key = systemKeyTrapper::AltTab;
		}
		else if( pkh->vkCode == VK_ESCAPE && pkh->flags & LLKHF_ALTDOWN )
		{
			key = systemKeyTrapper::AltEsc;
		}
		else if( pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN )
		{
			key = systemKeyTrapper::MetaKey;
		}
		if( key != systemKeyTrapper::None )
		{
			if( !pressed.contains( key ) )
			{
				__trapped_keys.push_back( key );
				pressed << key;
			}
			else
			{
				pressed.removeAll( key );
			}
			return( 1 );
		}
	}
	return CallNextHookEx( g_hHookKbdLL, nCode, wp, lp );
}


#define HKCU HKEY_CURRENT_USER

extern HINSTANCE hAppInstance;

#if 0
// Magic registry key/value for "Remove Task Manager" policy.
LPCTSTR KEY_DisableTaskMgr =
	"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
LPCTSTR VAL_DisableTaskMgr = "DisableTaskMgr";

void DisableTaskKeys( BOOL bDisable )
{
	HKEY hk;
	if( RegOpenKey( HKCU, KEY_DisableTaskMgr, &hk ) != ERROR_SUCCESS )
	{
		RegCreateKey( HKCU, KEY_DisableTaskMgr, &hk );
	}

	if( bDisable )
	{
		if( !g_hHookKbdLL )
		{
			// set lowlevel-keyboard-hook
			g_hHookKbdLL = SetWindowsHookEx( WH_KEYBOARD_LL,
							TaskKeyHookLL,
							hAppInstance, 0 );
		}
/*		// set registry-entry to disable task-manager (Alt+Ctrl+Del)
		DWORD val = 1;
		RegSetValueEx( hk, VAL_DisableTaskMgr, 0L, REG_DWORD,
						(BYTE*) &val, sizeof( val ) );*/
		Inject();
	}
	else if( g_hHookKbdLL != NULL )
	{
		// remove keyboard-hook
		UnhookWindowsHookEx( g_hHookKbdLL );
		g_hHookKbdLL = NULL;
		// delete registry-entry
		//RegDeleteValue( hk, VAL_DisableTaskMgr );
		Eject();
	}

	// enable/disable task-bar
	EnableWindow( FindWindow( "Shell_traywnd", NULL ), !bDisable );
}

#endif
#endif


systemKeyTrapper::systemKeyTrapper( void ) :
	QObject()
{
	s_refCntMutex.lock();
#if BUILD_WIN32
	if( !s_refCnt )
	{
		if( !g_hHookKbdLL )
		{
			// set lowlevel-keyboard-hook
			g_hHookKbdLL = SetWindowsHookEx( WH_KEYBOARD_LL, TaskKeyHookLL,
								hAppInstance, 0 );
		}

		EnableWindow( FindWindow( "Shell_traywnd", NULL ), FALSE );

		Inject();
	}

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this, SLOT( checkForTrappedKeys() ) );
	t->start( 10 );
#endif
	++s_refCnt;
	s_refCntMutex.unlock();
}




systemKeyTrapper::~systemKeyTrapper()
{
	s_refCntMutex.lock();
	--s_refCnt;
#if BUILD_WIN32
	if( !s_refCnt )
	{
		UnhookWindowsHookEx( g_hHookKbdLL );
		g_hHookKbdLL = NULL;

		EnableWindow( FindWindow( "Shell_traywnd", NULL ), TRUE );

		Eject();
	}
#endif
	s_refCntMutex.unlock();
}


void systemKeyTrapper::checkForTrappedKeys( void )
{
	QMutexLocker m( &__trapped_keys_mutex );

	while( !__trapped_keys.isEmpty() )
	{
		int key = 0;
		switch( __trapped_keys.front() )
		{
			case None: break;
			case AltCtrlDel: key = XK_Delete; break;
			case AltTab: key = XK_Tab; break;
			case AltEsc: key = XK_Escape; break;
			case CtrlEsc: key = XK_Escape; break;
			case MetaKey: key = XK_Meta_L; break;
		}

		if( key )
		{
			emit keyEvent( key, TRUE );
			emit keyEvent( key, FALSE );
		}

		__trapped_keys.removeFirst();

	}
}


#include "system_key_trapper.moc"

