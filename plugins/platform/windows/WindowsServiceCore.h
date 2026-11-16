// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include "PlatformServiceFunctions.h"
#include "PlatformSessionManager.h"
#include "ServiceDataManager.h"
#include "WindowsSmartObjects.h"

class WindowsServiceCore
{
public:
	WindowsServiceCore(const QString& name, const PlatformServiceFunctions::ServiceEntryPoint& serviceEntryPoint);
	~WindowsServiceCore() = default;

	static WindowsServiceCore* instance();

	bool runAsService();

	void manageServerInstances();

private:
	void manageServersForAllSessions();
	void manageServerForConsoleSession();

	static void WINAPI serviceMainStatic(DWORD argc, LPWSTR* argv);
	static DWORD WINAPI serviceCtrlStatic(DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context);

	void serviceMain();
	DWORD serviceCtrl(DWORD ctrlCode, DWORD eventType, LPVOID eventData, LPVOID context);
	bool reportStatus(DWORD state, DWORD exitCode, DWORD waitHint);

	SmartWCharPtr m_name;
	const PlatformServiceFunctions::ServiceEntryPoint& m_serviceEntryPoint;

	static WindowsServiceCore* s_instance;
	SERVICE_STATUS m_status{};
	SERVICE_STATUS_HANDLE m_statusHandle{nullptr};
	HANDLE m_sessionChangeEvent{nullptr};
	HANDLE m_stopServiceEvent{nullptr};
	HANDLE m_serverShutdownEvent{nullptr};
	QAtomicInt m_serviceStopRequested{0};
	QAtomicInt m_sessionChanged{0};

	ServiceDataManager m_dataManager{};
	PlatformSessionManager m_sessionManager{};

	static constexpr auto SessionPollingInterval = 5000;
	static constexpr auto MinimumServerUptimeTime = 10000;
	static constexpr auto ServiceStartTimeout = 15000;
};
