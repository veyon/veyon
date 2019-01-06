/*
 * SasEventListener.cpp - implementation of SasEventListener class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>

#include "SasEventListener.h"

SasEventListener::SasEventListener()
{
	const wchar_t* sasDll = L"\\sas.dll";
	wchar_t sasPath[MAX_PATH + wcslen(sasDll) + 1 ] = { 0 };
	if( GetSystemDirectory( sasPath, MAX_PATH ) == 0 )
	{
		qCritical() << Q_FUNC_INFO << "could not determine system directory";
	}
	wcscat( sasPath, sasDll );

	m_sasLibrary = LoadLibrary( sasPath ); // Flawfinder: ignore
	m_sendSas = (SendSas)GetProcAddress( m_sasLibrary, "SendSAS" );

	if( m_sendSas == nullptr )
	{
		qWarning( "SendSAS is not supported by operating system!" );
	}

	m_sasEvent = CreateEvent( nullptr, false, false, L"Global\\VeyonServiceSasEvent" );
	m_stopEvent = CreateEvent( nullptr, false, false, L"StopEvent" );
}



SasEventListener::~SasEventListener()
{
	CloseHandle( m_stopEvent );
	CloseHandle( m_sasEvent );

	FreeLibrary( m_sasLibrary );
}



void SasEventListener::stop()
{
	SetEvent( m_stopEvent );
}



void SasEventListener::run()
{
	HANDLE eventObjects[2] = { m_sasEvent, m_stopEvent };

	while( isInterruptionRequested() == false )
	{
		int event = WaitForMultipleObjects( 2, eventObjects, false, WaitPeriod );

		switch( event )
		{
		case WAIT_OBJECT_0 + 0:
			ResetEvent( m_sasEvent );
			if( m_sendSas )
			{
				m_sendSas( false );
			}
			break;

		case WAIT_OBJECT_0 + 1:
			requestInterruption();
			break;

		default:
			break;
		}
	}
}
