// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>

#include "ServiceDataManager.h"
#include "WtsSessionManager.h"
#include "WindowsSmartObjects.h"

// clazy:excludeall=rule-of-three
class VeyonServerProcess
{
public:
	VeyonServerProcess(WtsSessionManager::SessionId wtsSessionId, int sessionId, ServiceDataManager& dataManager);
	~VeyonServerProcess();

	auto uptime() const
	{
		return m_uptime.elapsed();
	}

	bool restartIfNotRunning();
	bool isRunning() const;
	bool hasServerEndpointFailed() const;
	bool waitForFinished(DWORD timeout) const;

private:
	void start();
	void stop();

	static constexpr auto ServerQueryTime = 10;
	static constexpr auto ServerWaitTime = 5000;

	const WtsSessionManager::SessionId m_wtsSessionId;
	const int m_sessionId;
	ServiceDataManager& m_dataManager;
	QString m_ipcEndpointName;

	SmartHandle m_processHandle{nullptr};
	QElapsedTimer m_uptime;
};
