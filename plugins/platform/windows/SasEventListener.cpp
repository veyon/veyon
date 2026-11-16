// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include "VeyonCore.h"
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

	m_sasEvent.reset(CreateEvent(nullptr, false, false, L"Global\\VeyonServiceSasEvent"));
	if (m_sasEvent.isInvalid())
	{
		vWarning() << "failed to create SAS event";
	}

	m_stopEvent.reset(CreateEvent(nullptr, false, false, L"StopEvent"));
	if (m_stopEvent.isInvalid())
	{
		vWarning() << "failed to create stop event";
	}
}



SasEventListener::~SasEventListener()
{
	if (m_sasLibrary)
	{
		FreeLibrary(m_sasLibrary);
	}
}



void SasEventListener::stop()
{
	if (m_stopEvent)
	{
		SetEvent(m_stopEvent.get());
	}
}



void SasEventListener::run()
{
	if (m_sasEvent.isInvalid() || m_stopEvent.isInvalid())
	{
		return;
	}

	std::array<HANDLE, 2> eventObjects{m_sasEvent.get(), m_stopEvent.get()};

	while (isInterruptionRequested() == false)
	{
		const auto event = WaitForMultipleObjects(eventObjects.size(), eventObjects.data(), FALSE, WaitPeriod);

		switch (event)
		{
		case WAIT_OBJECT_0 + 0:
			ResetEvent(m_sasEvent.get());
			if (m_sendSas)
			{
				m_sendSas(false);
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
