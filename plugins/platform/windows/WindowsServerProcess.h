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
	VeyonServerProcess(WtsSessionManager::SessionId wtsSessionId, const ServiceDataManager::Token& token);
	~VeyonServerProcess();

	auto uptime() const
	{
		return m_uptime.elapsed();
	}

	bool restartIfNotRunning();
	bool isRunning() const;
	bool waitForFinished(DWORD timeout) const;

private:
	void start();
	void stop();

	static constexpr auto ServerQueryTime = 10;
	static constexpr auto ServerWaitTime = 5000;

	const WtsSessionManager::SessionId m_wtsSessionId;
	const ServiceDataManager::Token m_token;

	SmartHandle m_processHandle{nullptr};
	QElapsedTimer m_uptime;
};
