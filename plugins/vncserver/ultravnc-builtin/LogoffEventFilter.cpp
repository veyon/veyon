// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>

#include "VeyonCore.h"
#include "LogoffEventFilter.h"

LogoffEventFilter::LogoffEventFilter() :
	m_shutdownEventHandle(OpenEvent(EVENT_ALL_ACCESS, false, "Global\\SessionEventVeyon"))
{
	if (m_shutdownEventHandle == nullptr)
	{
		// no global event available already as we're not running under the
		// control of the veyon service supervisor?
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			vWarning() << "Creating session event";
			// then create our own event as otherwise the VNC server main loop
			// will eat 100% CPU due to failing WaitForSingleObject() calls
			m_shutdownEventHandle = CreateEvent(nullptr, false, false, "Global\\SessionEventVeyon");
		}
		else
		{
			vWarning() << "Could not open or create session event";
		}
	}

	QCoreApplication::instance()->installNativeEventFilter(this);
}



bool LogoffEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
	Q_UNUSED(eventType);
	Q_UNUSED(result);

	if (!message)
	{
		return false;
	}

	const auto winMsg = reinterpret_cast<MSG*>(message)->message;

	if (winMsg == WM_ENDSESSION)
	{
		vInfo() << "Got WM_ENDSESSION - initiating server shutdown";

		// tell UltraVNC server to quit
		SetEvent(m_shutdownEventHandle);
	}

	return false;
}
