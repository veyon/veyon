/*
 * VeyonServerProcess.cpp - implementation of VeyonServerProcess class
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QElapsedTimer>

#include "ServiceDataManager.h"
#include "WtsSessionManager.h"

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

private:
	void start();
	void stop();

	static constexpr auto ServerQueryTime = 10;
	static constexpr auto ServerWaitTime = 3000;

	const WtsSessionManager::SessionId m_wtsSessionId;
	const ServiceDataManager::Token m_token;

	HANDLE m_processHandle{nullptr};
	QElapsedTimer m_uptime;

};
