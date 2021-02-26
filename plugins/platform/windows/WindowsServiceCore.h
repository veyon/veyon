/*
 * WindowsServiceCore.h - header file for WindowsServiceCore class
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>

#include "PlatformServiceFunctions.h"
#include "PlatformSessionManager.h"
#include "ServiceDataManager.h"

class WindowsServiceCore
{
public:
	WindowsServiceCore( const QString& name, const PlatformServiceFunctions::ServiceEntryPoint& serviceEntryPoint );
	~WindowsServiceCore() = default;

	static WindowsServiceCore* instance();

	bool runAsService();

	void manageServerInstances();

private:
	void manageServersForAllSessions();
	void manageServerForActiveConsoleSession();

	static void WINAPI serviceMainStatic( DWORD argc, LPWSTR* argv );
	static DWORD WINAPI serviceCtrlStatic( DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context );

	void serviceMain();
	DWORD serviceCtrl( DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context );
	bool reportStatus( DWORD state, DWORD exitCode, DWORD waitHint );

	QSharedPointer<wchar_t> m_name;
	const PlatformServiceFunctions::ServiceEntryPoint& m_serviceEntryPoint;

	static WindowsServiceCore* s_instance;
	SERVICE_STATUS m_status{};
	SERVICE_STATUS_HANDLE m_statusHandle{nullptr};
	HANDLE m_stopServiceEvent{nullptr};
	HANDLE m_serverShutdownEvent{nullptr};
	QAtomicInt m_sessionChangeEvent{0};

	ServiceDataManager m_dataManager{};
	PlatformSessionManager m_sessionManager{};

	static constexpr auto SessionPollingInterval = 100;
	static constexpr auto MinimumServerUptimeTime = 10000;
	static constexpr auto ServiceStartTimeout = 15000;

} ;
