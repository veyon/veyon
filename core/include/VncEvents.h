/*
 * VncEvent.h - declaration of VncEvent and subclasses
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

#pragma once

#include <QString>

using rfbClient = struct _rfbClient;

// clazy:excludeall=copyable-polymorphic

class VncEvent
{
public:
	virtual ~VncEvent() = default;
	virtual void fire( rfbClient* client ) = 0;

} ;


class VncKeyEvent : public VncEvent
{
public:
	VncKeyEvent( unsigned int key, bool pressed );

	void fire( rfbClient* client ) override;

private:
	unsigned int m_key;
	bool m_pressed;
} ;


class VncPointerEvent : public VncEvent
{
public:
	VncPointerEvent( int x, int y, int buttonMask );

	void fire( rfbClient* client ) override;

private:
	int m_x;
	int m_y;
	int m_buttonMask;
} ;


class VncClientCutEvent : public VncEvent
{
public:
	explicit VncClientCutEvent( const QString& text );

	void fire( rfbClient* client ) override;

private:
	QByteArray m_text;
} ;
