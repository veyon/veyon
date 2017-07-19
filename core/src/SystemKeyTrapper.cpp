/*
 * SystemKeyTrapper.cpp - class for trapping system-keys and -key-sequences
 *                        such as Alt+Ctrl+Del, Alt+Tab etc.
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <veyonconfig.h>

#ifdef VEYON_BUILD_WIN32

#include <windows.h>

#endif

#define XK_KOREAN
#include "rfb/keysym.h"
#include "SystemKeyTrapper.h"

#include <QtCore/QList>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

// clazy:excludeall=non-pod-global-static

static QMutex __trapped_keys_mutex;
static QList<SystemKeyTrapper::TrappedKeys> __trapped_keys;

QMutex SystemKeyTrapper::s_refCntMutex;
int SystemKeyTrapper::s_refCnt = 0;


#ifdef VEYON_BUILD_WIN32


// some code for trapping system's hotkeys such as Alt+Ctrl+Del, Alt+Tab,
// Ctrl+Esc, Alt+Esc, Windows-key etc. - otherwise locking wouldn't be very
// effective... ;-)


HHOOK g_hHookKbdLL = NULL; // hook handle


LRESULT CALLBACK TaskKeyHookLL( int nCode, WPARAM wp, LPARAM lp )
{
	KBDLLHOOKSTRUCT *pkh = (KBDLLHOOKSTRUCT *) lp;
	static QList<SystemKeyTrapper::TrappedKeys> pressed;
	if( nCode == HC_ACTION )
	{
		BOOL bCtrlKeyDown = GetAsyncKeyState( VK_CONTROL ) >>
						( ( sizeof( SHORT ) * 8 ) - 1 );
		QMutexLocker m( &__trapped_keys_mutex );

		SystemKeyTrapper::TrappedKeys key = SystemKeyTrapper::None;
		if( pkh->vkCode == VK_ESCAPE && bCtrlKeyDown )
		{
			key = SystemKeyTrapper::CtrlEsc;
		}
		else if( pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN )
		{
			key = SystemKeyTrapper::AltTab;
		}
		else if( pkh->vkCode == VK_ESCAPE &&
						pkh->flags & LLKHF_ALTDOWN )
		{
			key = SystemKeyTrapper::AltEsc;
		}
		else if( pkh->vkCode == VK_SPACE && pkh->flags & LLKHF_ALTDOWN )
		{
			key = SystemKeyTrapper::AltSpace;
		}
		else if( pkh->vkCode == VK_F4 && pkh->flags & LLKHF_ALTDOWN )
		{
			key = SystemKeyTrapper::AltF4;
		}
		else if( pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN )
		{
			pressed.removeAll( SystemKeyTrapper::SuperKeyDown );
			pressed.removeAll( SystemKeyTrapper::SuperKeyUp );
			if( wp == WM_KEYDOWN )
			{
				key = SystemKeyTrapper::SuperKeyDown;
			}
			else
			{
				key = SystemKeyTrapper::SuperKeyUp;
			}
		}
		else if( pkh->vkCode == VK_DELETE && bCtrlKeyDown &&
						pkh->flags && LLKHF_ALTDOWN )
		{
			key = SystemKeyTrapper::AltCtrlDel;
		}
		if( key != SystemKeyTrapper::None )
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
			return 1;
		}
	}
	return CallNextHookEx( g_hHookKbdLL, nCode, wp, lp );
}

#endif


SystemKeyTrapper::SystemKeyTrapper( bool enabled, QObject* parent ) :
	QObject( parent ),
	m_enabled( false )
{
	setEnabled( enabled );
}




SystemKeyTrapper::~SystemKeyTrapper()
{
	setEnabled( false );
}



void SystemKeyTrapper::setEnabled( bool on )
{
	if( on == m_enabled )
	{
		return;
	}

	s_refCntMutex.lock();

	m_enabled = on;
	if( on )
	{
#ifdef VEYON_BUILD_WIN32
		if( !s_refCnt )
		{
			if( !g_hHookKbdLL )
			{
				HINSTANCE hAppInstance =
							GetModuleHandle( NULL );
				// set lowlevel-keyboard-hook
				g_hHookKbdLL =
					SetWindowsHookEx( WH_KEYBOARD_LL,
								TaskKeyHookLL,
								hAppInstance,
								0 );
			}
		}

		QTimer * t = new QTimer( this );
		connect( t, SIGNAL( timeout() ),
					this, SLOT( checkForTrappedKeys() ) );
		t->start( 10 );
#endif
		++s_refCnt;
	}
	else
	{
		--s_refCnt;
#ifdef VEYON_BUILD_WIN32
		if( !s_refCnt )
		{
			UnhookWindowsHookEx( g_hHookKbdLL );
			g_hHookKbdLL = NULL;
		}
#endif
	}
	s_refCntMutex.unlock();
}







void SystemKeyTrapper::checkForTrappedKeys()
{
	QMutexLocker m( &__trapped_keys_mutex );

	while( !__trapped_keys.isEmpty() )
	{
		unsigned int key = 0;
		bool toggle = true;
		bool stateToSet = false;
		switch( __trapped_keys.front() )
		{
			case None: break;
			case AltCtrlDel: key = XK_Delete; break;
			case AltTab: key = XK_Tab; break;
			case AltEsc: key = XK_Escape; break;
			case AltSpace: key = XK_KP_Space; break;
			case AltF4: key = XK_F4; break;
			case CtrlEsc: key = XK_Escape; break;
			case SuperKeyDown: key = XK_Super_L; toggle = false; stateToSet = true; break;
			case SuperKeyUp: key = XK_Super_L; toggle = false; stateToSet = false; break;
		}

		if( key )
		{
			if( toggle )
			{
				emit keyEvent( key, true );
				emit keyEvent( key, false );
			}
			else
			{
				emit keyEvent( key, stateToSet );
			}
		}

		__trapped_keys.removeFirst();

	}
}


