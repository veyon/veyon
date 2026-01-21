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

#include "Filesystem.h"
#include "WindowsCoreFunctions.h"
#include "WindowsServerProcess.h"


VeyonServerProcess::VeyonServerProcess(DWORD wtsSessionId, const ServiceDataManager::Token& token) :
	m_wtsSessionId(wtsSessionId),
	m_token(token)
{
	start();
}



VeyonServerProcess::~VeyonServerProcess()
{
	stop();
}



bool VeyonServerProcess::restartIfNotRunning()
{
	if (isRunning() == false)
	{
		if (m_processHandle)
		{
			CloseHandle(m_processHandle);
			m_processHandle = nullptr;
		}

		start();

		return true;
	}

	return false;
}



bool VeyonServerProcess::isRunning() const
{
	DWORD exitCode = 0;
	if (m_processHandle &&
		GetExitCodeProcess(m_processHandle, &exitCode))
	{
		return exitCode == STILL_ACTIVE;
	}

	return false;
}



void VeyonServerProcess::start()
{
	const auto baseProcessId = WtsSessionManager::findProcessId(QStringLiteral("winlogon.exe"), m_wtsSessionId);
	const auto user = WtsSessionManager::querySessionInformation(m_wtsSessionId, WtsSessionManager::SessionInfo::UserName);

	const QStringList extraEnv{
		QStringLiteral("%1=%2").arg(QLatin1String( ServiceDataManager::serviceDataTokenEnvironmentVariable()),
									QString::fromUtf8(m_token.toByteArray()))
	};

	vInfo() << "Starting server for WTS session" << m_wtsSessionId << "with user" << user;
	m_processHandle = WindowsCoreFunctions::runProgramInSession(VeyonCore::filesystem().serverFilePath(), {},
																extraEnv,
																baseProcessId, {});
	if (m_processHandle == nullptr)
	{
		vCritical() << "Failed to start server!";
	}

	m_uptime.restart();
}



void VeyonServerProcess::stop()
{
	if (m_processHandle)
	{
		vInfo() << "Waiting for server to shutdown";
		if (WaitForSingleObject(m_processHandle, ServerWaitTime) == WAIT_TIMEOUT)
		{
			vWarning() << "Terminating server";
			TerminateProcess(m_processHandle, 0);
			WaitForSingleObject(m_processHandle, ServerWaitTime);
		}

		CloseHandle(m_processHandle);
		m_processHandle = nullptr;
	}
}
