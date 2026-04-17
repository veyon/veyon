/*
 * SasEventListener.cpp - implementation of SasEventListener class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "SasEventListener.h"

SasEventListener::SasEventListener()
{
	m_sasLibrary = LoadLibraryEx(L"sas.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (m_sasLibrary)
	{
		m_sendSas = reinterpret_cast<SendSas>(GetProcAddress(m_sasLibrary, "SendSAS"));
	}

	if (m_sasLibrary == nullptr || m_sendSas == nullptr)
	{
		vWarning() << "SendSAS is not supported by operating system!";
	}

	m_sasEvent = CreateEvent( nullptr, false, false, L"Global\\VeyonServiceSasEvent" );
	if (m_sasEvent == nullptr)
	{
		vWarning() << "failed to create SAS event";
	}

	m_stopEvent = CreateEvent( nullptr, false, false, L"StopEvent" );
	if (m_stopEvent == nullptr)
	{
		vWarning() << "failed to create stop event";
	}
}



SasEventListener::~SasEventListener()
{
	if (m_stopEvent)
	{
		CloseHandle(m_stopEvent);
	}
	if (m_sasEvent)
	{
		CloseHandle(m_sasEvent);
	}

	if (m_sasLibrary)
	{
		FreeLibrary(m_sasLibrary);
	}
}



void SasEventListener::stop()
{
	if (m_stopEvent)
	{
		SetEvent(m_stopEvent);
	}
}



void SasEventListener::run()
{
	if (m_sasEvent == nullptr || m_stopEvent == nullptr)
	{
		return;
	}

	std::array<HANDLE, 2> eventObjects{ m_sasEvent, m_stopEvent };

	while( isInterruptionRequested() == false )
	{
		const auto event = WaitForMultipleObjects( eventObjects.size(), eventObjects.data(), FALSE, WaitPeriod );

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
