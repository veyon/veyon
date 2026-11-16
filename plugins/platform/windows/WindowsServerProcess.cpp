// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
			m_processHandle.reset();
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
		GetExitCodeProcess(m_processHandle.get(), &exitCode))
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
		QStringLiteral("%1=%2").arg(QLatin1String(ServiceDataManager::serviceDataTokenEnvironmentVariable()),
									QString::fromUtf8(m_token.toByteArray()))};

	vInfo() << "Starting server for WTS session" << m_wtsSessionId << "with user" << user;
	m_processHandle = WindowsCoreFunctions::runProgramInSession(VeyonCore::filesystem().serverFilePath(), {},
																extraEnv,
																baseProcessId, {}, {});
	if (m_processHandle.isInvalid())
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
		if (WaitForSingleObject(m_processHandle.get(), ServerWaitTime) == WAIT_TIMEOUT)
		{
			vWarning() << "Terminating server";
			TerminateProcess(m_processHandle.get(), 0);
			WaitForSingleObject(m_processHandle.get(), ServerWaitTime);
		}

		m_processHandle.reset();
	}
}
