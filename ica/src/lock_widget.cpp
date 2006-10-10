/*
 *  lock_widget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *  
 *  This file is part of iTALC - http://italc.sourceforge.net
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */


#include "lock_widget.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPainter>


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


LRESULT CALLBACK TaskKeyHookLL(int nCode, WPARAM wp, LPARAM lp)
{
	KBDLLHOOKSTRUCT *pkh = (KBDLLHOOKSTRUCT *) lp;

	if( nCode == HC_ACTION )
	{
		BOOL bCtrlKeyDown = GetAsyncKeyState( VK_CONTROL ) >>
						( ( sizeof( SHORT ) * 8 ) - 1 );

		if(
			// Ctrl+Esc
			( pkh->vkCode == VK_ESCAPE && bCtrlKeyDown )
			||
			// Alt+Tab
			( pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN )
			||
			// Alt+Esc
			( pkh->vkCode == VK_ESCAPE && pkh->flags & LLKHF_ALTDOWN )
			||
			// Start Menu
			( pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN ) )
		{
			return( 1 );
		}
	}
	return CallNextHookEx( g_hHookKbdLL, nCode, wp, lp );
}


#define HKCU HKEY_CURRENT_USER

extern HINSTANCE hAppInstance;

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


#ifdef BUILD_LINUX

#include <X11/Xlib.h>

#endif



lockWidget::lockWidget( types _type ) :
	QWidget( 0, Qt::X11BypassWindowManagerHint ),
	m_background(
		_type == Black ?
			QPixmap( ":/resources/locked.png" )
		:
			_type == DesktopVisible ?
				QPixmap::grabWindow( qApp->desktop()->winId() )
			:
				QPixmap() ),
	m_type( _type )
{
	activateWindow();
	setFixedSize( qApp->desktop()->size() );
	move( 0, 0 );
	showFullScreen();
	raise();
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	grabMouse();
	grabKeyboard();
	setCursor( Qt::BlankCursor );
#ifdef BUILD_WIN32
	DisableTaskKeys( TRUE );
#endif
}




lockWidget::~lockWidget()
{
#ifdef BUILD_WIN32
	DisableTaskKeys( FALSE );
#endif
}




void lockWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	switch( m_type )
	{
		case DesktopVisible:
			p.drawPixmap( 0, 0, m_background );
			break;

		case Black:
			p.fillRect( rect(), QColor( 0, 0, 0 ) );
			p.drawPixmap( ( width() - m_background.width() ) / 2, 
					( height() - m_background.height() ) / 2,
								m_background );
			break;

		default:
			break;
	}
}



#ifdef BUILD_LINUX
bool lockWidget::x11Event( XEvent * _e )
{
	switch( _e->type )
	{
		case KeyPress:
		case ButtonPress:
		case MotionNotify:
			return( TRUE );
		default:
			break;
	}
	return( FALSE );
}
#endif



/*
#ifdef BUILD_WIN32
bool lockWidget::winEvent( MSG * _message, long * _result )
{
	switch( _message->message )
	{
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_NCMOUSEMOVE:
#if _WIN32_WINNT >= 0x0500
		case WM_NCXBUTTONDOWN:
		case WM_NCXBUTTONUP:
#endif
			return( TRUE );
		default:
			break;
	}
	return( FALSE );
}
#endif
*/
