/*
 * WindowsKeyboardShortcutTrapper.cpp - class for trapping shortcuts on Windows
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <windows.h>

#include "WindowsKeyboardShortcutTrapper.h"

// clazy:excludeall=non-pod-global-static

static QMutex __trappedShortcutsMutex;
static QList<WindowsKeyboardShortcutTrapper::Shortcut> __trappedShortcuts;

QMutex WindowsKeyboardShortcutTrapper::s_refCntMutex;
int WindowsKeyboardShortcutTrapper::s_refCnt = 0;


static HHOOK __lowLevelKeyboardHookHandle = nullptr; // hook handle


LRESULT CALLBACK TaskKeyHookLL( int nCode, WPARAM wp, LPARAM lp )
{
	auto pkh = reinterpret_cast<KBDLLHOOKSTRUCT *>( static_cast<intptr_t>( lp ) );
	static QList<KeyboardShortcutTrapper::Shortcut> pressed;

	if( nCode == HC_ACTION )
	{
		BOOL isCtrlKeyDown = GetAsyncKeyState( VK_CONTROL ) >> ( ( sizeof( SHORT ) * 8 ) - 1 );
		QMutexLocker m( &__trappedShortcutsMutex );

		KeyboardShortcutTrapper::Shortcut shortcut = KeyboardShortcutTrapper::NoShortcut;

		if( pkh->vkCode == VK_ESCAPE && isCtrlKeyDown )
		{
			shortcut = KeyboardShortcutTrapper::CtrlEsc;
		}
		else if( pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN )
		{
			shortcut = KeyboardShortcutTrapper::AltTab;
		}
		else if( pkh->vkCode == VK_ESCAPE && pkh->flags & LLKHF_ALTDOWN )
		{
			shortcut = KeyboardShortcutTrapper::AltEsc;
		}
		else if( pkh->vkCode == VK_SPACE && pkh->flags & LLKHF_ALTDOWN )
		{
			shortcut = KeyboardShortcutTrapper::AltSpace;
		}
		else if( pkh->vkCode == VK_F4 && pkh->flags & LLKHF_ALTDOWN )
		{
			shortcut = KeyboardShortcutTrapper::AltF4;
		}
		else if( pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN )
		{
			pressed.removeAll( KeyboardShortcutTrapper::SuperKeyDown );
			pressed.removeAll( KeyboardShortcutTrapper::SuperKeyUp );
			if( wp == WM_KEYDOWN )
			{
				shortcut = KeyboardShortcutTrapper::SuperKeyDown;
			}
			else
			{
				shortcut = KeyboardShortcutTrapper::SuperKeyUp;
			}
		}

		if( shortcut != KeyboardShortcutTrapper::NoShortcut )
		{
			if( pressed.contains( shortcut ) == false )
			{
				__trappedShortcuts.push_back( shortcut );
				pressed << shortcut;
			}
			else
			{
				pressed.removeAll( shortcut );
			}
			return 1;
		}
	}
	return CallNextHookEx( __lowLevelKeyboardHookHandle, nCode, wp, lp );
}



WindowsKeyboardShortcutTrapper::WindowsKeyboardShortcutTrapper( QObject* parent ) :
	KeyboardShortcutTrapper( parent ),
	m_enabled( false ),
	m_pollTimer( this )
{
	connect( &m_pollTimer, &QTimer::timeout,
				this, &WindowsKeyboardShortcutTrapper::forwardTrappedShortcuts );
}



WindowsKeyboardShortcutTrapper::~WindowsKeyboardShortcutTrapper()
{
	setEnabled( false );
}



void WindowsKeyboardShortcutTrapper::setEnabled( bool on )
{
	if( on == m_enabled )
	{
		return;
	}

	s_refCntMutex.lock();

	m_enabled = on;
	if( on )
	{
		if( !s_refCnt )
		{
			if( __lowLevelKeyboardHookHandle == nullptr )
			{
				HINSTANCE hAppInstance = GetModuleHandle( nullptr );
				// set lowlevel-keyboard-hook
				__lowLevelKeyboardHookHandle = SetWindowsHookEx( WH_KEYBOARD_LL, TaskKeyHookLL, hAppInstance, 0 );
			}
		}

		m_pollTimer.start( PollTimerInterval );
		++s_refCnt;
	}
	else
	{
		--s_refCnt;
		if( !s_refCnt )
		{
			UnhookWindowsHookEx( __lowLevelKeyboardHookHandle );
			__lowLevelKeyboardHookHandle = nullptr;
			m_pollTimer.stop();
		}
	}
	s_refCntMutex.unlock();
}



void WindowsKeyboardShortcutTrapper::forwardTrappedShortcuts()
{
	QMutexLocker m( &__trappedShortcutsMutex );

	while( __trappedShortcuts.isEmpty() == false )
	{
		emit shortcutTrapped( __trappedShortcuts.takeFirst() );
	}
}
