/*
 * InputDeviceBlocker.cpp - class for blocking all input devices
 *
 * Copyright (c) 2016-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "InputDeviceBlocker.h"
#include "PlatformServiceFunctions.h"

#ifdef VEYON_BUILD_LINUX
#include <X11/XKBlib.h>
#endif

#include <QProcess>


QMutex InputDeviceBlocker::s_refCntMutex;
int InputDeviceBlocker::s_refCnt = 0;

#ifdef VEYON_BUILD_WIN32
int interception_is_any(InterceptionDevice device)
{
	Q_UNUSED(device)

	return true;
}
#endif



InputDeviceBlocker::InputDeviceBlocker( bool enabled ) :
	m_enabled( false ),
	m_hidServiceName( "hidserv" ),
	m_hidServiceActivated( VeyonCore::platform().serviceFunctions()->isRunning( m_hidServiceName ) )
#ifdef VEYON_BUILD_LINUX
	, m_origKeyTable( nullptr ),
	m_keyCodeMin( 0 ),
	m_keyCodeMax( 0 ),
	m_keyCodeCount( 0 ),
	m_keySymsPerKeyCode( 0 )
#endif
{
	setEnabled( enabled );
}




InputDeviceBlocker::~InputDeviceBlocker()
{
	setEnabled( false );
}




void InputDeviceBlocker::setEnabled( bool on )
{
	if( on == m_enabled )
	{
		return;
	}

	s_refCntMutex.lock();

	m_enabled = on;
	if( on )
	{
		if( s_refCnt == 0 )
		{
			enableInterception();
			stopHIDService();
			setEmptyKeyMapTable();
		}
		++s_refCnt;
	}
	else
	{
		--s_refCnt;
		if( s_refCnt == 0 )
		{
			disableInterception();
			restoreHIDService();
			restoreKeyMapTable();
		}
	}
	s_refCntMutex.unlock();
}



void InputDeviceBlocker::enableInterception()
{
#ifdef VEYON_BUILD_WIN32
	m_interceptionContext = interception_create_context();

	interception_set_filter( m_interceptionContext,
								interception_is_any,
								INTERCEPTION_FILTER_KEY_ALL | INTERCEPTION_FILTER_MOUSE_ALL );
#endif
}



void InputDeviceBlocker::disableInterception()
{
#ifdef VEYON_BUILD_WIN32
	interception_destroy_context( m_interceptionContext );
#endif
}


void InputDeviceBlocker::restoreHIDService()
{
	if( m_hidServiceActivated )
	{
		VeyonCore::platform().serviceFunctions()->start( m_hidServiceName );
	}
}



void InputDeviceBlocker::stopHIDService()
{
	if( m_hidServiceActivated )
	{
		VeyonCore::platform().serviceFunctions()->stop( m_hidServiceName );
	}
}



void InputDeviceBlocker::setEmptyKeyMapTable()
{
#ifdef VEYON_BUILD_LINUX
	if( m_origKeyTable )
	{
		XFree( m_origKeyTable );
	}

	Display* display = XOpenDisplay( nullptr );
	XDisplayKeycodes( display, &m_keyCodeMin, &m_keyCodeMax );
	m_keyCodeCount = m_keyCodeMax - m_keyCodeMin;

	m_origKeyTable = XGetKeyboardMapping( display, m_keyCodeMin,
										  m_keyCodeCount, &m_keySymsPerKeyCode );

	KeySym* newKeyTable = XGetKeyboardMapping( display, m_keyCodeMin,
											   m_keyCodeCount, &m_keySymsPerKeyCode );

	for( int i = 0; i < m_keyCodeCount * m_keySymsPerKeyCode; i++ )
	{
		newKeyTable[i] = 0;
	}

	XChangeKeyboardMapping( display, m_keyCodeMin, m_keySymsPerKeyCode,
							newKeyTable, m_keyCodeCount );
	XFlush( display );
	XFree( newKeyTable );
	XCloseDisplay( display );
#endif
}



void InputDeviceBlocker::restoreKeyMapTable()
{
#ifdef VEYON_BUILD_LINUX
	Display* display = XOpenDisplay( nullptr );

	XChangeKeyboardMapping( display, m_keyCodeMin, m_keySymsPerKeyCode,
							static_cast<KeySym *>( m_origKeyTable ), m_keyCodeCount );

	XFlush( display );
	XCloseDisplay( display );

	XFree( m_origKeyTable );
	m_origKeyTable = nullptr;
#endif
}
