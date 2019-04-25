/*
 * LogoffEventFilter.cpp - implementation of LogoffEventFilter class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>

#include "VeyonCore.h"
#include "LogoffEventFilter.h"

LogoffEventFilter::LogoffEventFilter() :
	m_shutdownEventHandle( OpenEvent( EVENT_ALL_ACCESS, false, "Global\\SessionEventUltra" ) )
{
	if( m_shutdownEventHandle == nullptr )
	{
		// no global event available already as we're not running under the
		// control of the veyon service supervisor?
		if( GetLastError() == ERROR_FILE_NOT_FOUND )
		{
			vWarning() << "Creating session event";
			// then create our own event as otherwise the VNC server main loop
			// will eat 100% CPU due to failing WaitForSingleObject() calls
			m_shutdownEventHandle = CreateEvent( nullptr, false, false, "Global\\SessionEventUltra" );
		}
		else
		{
			vWarning() << "Could not open or create session event";
		}
	}

	QCoreApplication::instance()->installNativeEventFilter( this );
}



bool LogoffEventFilter::nativeEventFilter( const QByteArray& eventType, void* message, long* result )
{
	Q_UNUSED(eventType);
	Q_UNUSED(result);

	const auto winMsg = reinterpret_cast<MSG *>( message )->message;

	if( winMsg == WM_QUERYENDSESSION )
	{
		vInfo() << "Got WM_QUERYENDSESSION - initiating server shutdown";

		// tell UltraVNC server to quit
		SetEvent( m_shutdownEventHandle );
	}

	return false;
}
