/*
 * VncEvents.cpp - implementation of VNC event classes
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <rfb/rfbclient.h>

#include "VncEvents.h"


VncKeyEvent::VncKeyEvent( unsigned int key, bool pressed ) :
	m_key( key ),
	m_pressed( pressed )
{
}


void VncKeyEvent::fire( rfbClient* client )
{
	SendKeyEvent( client, m_key, m_pressed );
}



VncPointerEvent::VncPointerEvent(int x, int y, int buttonMask) :
	m_x( x ),
	m_y( y ),
	m_buttonMask( buttonMask )
{
}



void VncPointerEvent::fire(rfbClient *client)
{
	SendPointerEvent( client, m_x, m_y, m_buttonMask );
}



VncClientCutEvent::VncClientCutEvent( const QString& text ) :
	m_text( text.toUtf8() )
{
}



void VncClientCutEvent::fire( rfbClient* client )
{
	SendClientCutText( client, m_text.data(), m_text.size() ); // clazy:exclude=detaching-member
}
