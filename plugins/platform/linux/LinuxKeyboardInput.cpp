/*
 * LinuxKeyboardInput.cpp - implementation of LinuxKeyboardInput class
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "LinuxKeyboardInput.h"

#include <X11/Xlib.h>

#include <fakekey/fakekey.h>


LinuxKeyboardInput::LinuxKeyboardInput() :
	m_display( XOpenDisplay( nullptr ) ),
	m_fakeKeyHandle( m_display ? fakekey_init( m_display ) : nullptr )
{
}



LinuxKeyboardInput::~LinuxKeyboardInput()
{
	if( m_fakeKeyHandle )
	{
		free( m_fakeKeyHandle );
	}

	if( m_display )
	{
		XCloseDisplay( m_display );
	}
}



void LinuxKeyboardInput::pressKey( KeySym keysym )
{
	fakekey_press_keysym( m_fakeKeyHandle, keysym, 0 );
}



void LinuxKeyboardInput::releaseKey( KeySym keysym )
{
	Q_UNUSED(keysym)

	fakekey_release( m_fakeKeyHandle );
}



void LinuxKeyboardInput::pressAndReleaseKey( KeySym keysym )
{
	pressKey( keysym );
	releaseKey( keysym );
}



void LinuxKeyboardInput::pressAndReleaseKey( const QByteArray& utf8Data )
{
	fakekey_press( m_fakeKeyHandle, reinterpret_cast<const unsigned char *>( utf8Data.constData() ), utf8Data.size(), 0 );
	fakekey_release( m_fakeKeyHandle );
}



void LinuxKeyboardInput::sendString( const QString& string )
{
	for( int i = 0; i < string.size(); ++i )
	{
		pressAndReleaseKey( string.midRef( i, 1 ).toUtf8() );
	}
}
