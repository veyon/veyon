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

#include <QProcess>


QMutex InputDeviceBlocker::s_refCntMutex;
int InputDeviceBlocker::s_refCnt = 0;


InputDeviceBlocker::InputDeviceBlocker( bool enabled ) :
	m_enabled( false )
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
			saveKeyMapTable();
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
								interception_is_keyboard,
								INTERCEPTION_FILTER_KEY_ALL );
#endif
}



void InputDeviceBlocker::disableInterception()
{
#ifdef VEYON_BUILD_WIN32
	interception_destroy_context( m_interceptionContext );
#endif
}



void InputDeviceBlocker::saveKeyMapTable()
{
#ifdef VEYON_BUILD_LINUX
	// read original keymap
	QProcess p;
	p.start( QStringLiteral( "xmodmap" ), { QStringLiteral( "-pke" ) } );	// print keymap
	if( p.waitForStarted( XmodmapMaxStartTime ) )
	{
		p.waitForFinished();
		m_origKeyTable = p.readAll();
	}
	else
	{
		xmodmapError();
	}
#endif
}



void InputDeviceBlocker::setEmptyKeyMapTable()
{
#ifdef VEYON_BUILD_LINUX
	// create empty key map table
	QStringList emptyKeyMapTable;
	const int keyCodeStart = 8;
	const int keyCodeEnd = 256;
	emptyKeyMapTable.reserve( keyCodeStart - keyCodeEnd );

	for( auto i = keyCodeStart; i < keyCodeEnd; ++i )
	{
		emptyKeyMapTable += QString( QStringLiteral( "keycode %1 =" ) ).arg( i );
	}

	// start new xmodmap process and dump our empty keytable from stdin
	QProcess p;
	p.start( QStringLiteral( "xmodmap" ), { QStringLiteral( "-" ) } );
	if( p.waitForStarted( XmodmapMaxStartTime ) )
	{
		p.write( emptyKeyMapTable.join( '\n' ).toUtf8() );
		p.closeWriteChannel();
		p.waitForFinished();
	}
	else
	{
		xmodmapError();
	}
#endif
}



void InputDeviceBlocker::restoreKeyMapTable()
{
#ifdef VEYON_BUILD_LINUX
	// start xmodmap process and dump our original keytable from stdin
	QProcess p;
	p.start( QStringLiteral( "xmodmap" ), { QStringLiteral( "-" ) } );
	if( p.waitForStarted( XmodmapMaxStartTime ) )
	{
		p.write( m_origKeyTable );
		p.closeWriteChannel();
		p.waitForFinished();
	}
	else
	{
		xmodmapError();
	}
#endif
}



void InputDeviceBlocker::xmodmapError()
{
	qCritical( "InputDeviceBlocker: could not start xmodmap - do you have x11-xserver-utils, xmodmap or xorg-xmodmap installed?" );
}
