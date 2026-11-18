// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <wtsapi32.h>

#include "Filesystem.h"
#include "WindowsCoreFunctions.h"
#include "WindowsServerProcess.h"


// fails closed: a failed query counts as "user logged on" (ServiceDataManager contract)
static bool isUserLoggedOnInWtsSession(DWORD wtsSessionId)
{
	LPWSTR userName = nullptr;
	DWORD userNameSize = 0;
	if (WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, wtsSessionId, WTSUserName,
									&userName, &userNameSize) == FALSE ||
		userName == nullptr)
	{
		return true;
	}

	const auto loggedOn = userName[0] != L'\0';
	WTSFreeMemory(userName);

	return loggedOn;
}



VeyonServerProcess::VeyonServerProcess(DWORD wtsSessionId, int sessionId, ServiceDataManager& dataManager) :
	m_wtsSessionId(wtsSessionId),
	m_sessionId(sessionId),
	m_dataManager(dataManager)
{
	start();
}



VeyonServerProcess::~VeyonServerProcess()
{
	stop();
}



bool VeyonServerProcess::restartIfNotRunning()
{
	// a server whose IPC endpoint failed (02 §1: too many failed handshakes) is considered failed
	const auto endpointFailed = m_dataManager.hasServerEndpointFailed(m_ipcEndpointName);
	if (endpointFailed)
	{
		vCritical() << "IPC endpoint of server for WTS session" << m_wtsSessionId << "failed - restarting server";
		stop();
	}

	if (endpointFailed || isRunning() == false)
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



bool VeyonServerProcess::waitForFinished(DWORD timeout) const
{
	if (m_processHandle)
	{
		return WaitForSingleObject(m_processHandle.get(), timeout) != WAIT_TIMEOUT;
	}

	return true;
}



void VeyonServerProcess::start()
{
	m_dataManager.removeServerEndpoint(m_ipcEndpointName);
	m_ipcEndpointName.clear();

	const auto baseProcessId = WtsSessionManager::findProcessId(QStringLiteral("winlogon.exe"), m_wtsSessionId);
	const auto user = WtsSessionManager::querySessionInformation(m_wtsSessionId, WtsSessionManager::SessionInfo::UserName);

	// one endpoint and token per start, accessible for SYSTEM and administrators only; the
	// server (SYSTEM in the user session) must be the started process in the expected session
	IpcServer::PeerPolicy peerPolicy;
	peerPolicy.windowsAccess = IpcServer::WindowsAccess::SystemAndAdministrators;
	peerPolicy.windowsSessionId = qint64(m_wtsSessionId);
	peerPolicy.checkProcessId = true;

	const auto wtsSessionId = m_wtsSessionId;
	// the server runs as SYSTEM, so the session user cannot read its memory
	auto endpoint = m_dataManager.createServerEndpoint(m_sessionId, QString::number(m_wtsSessionId), peerPolicy,
													   [wtsSessionId]() { return isUserLoggedOnInWtsSession(wtsSessionId); },
													   {});
	if (endpoint.has_value() == false)
	{
		vCritical() << "not starting server for WTS session" << m_wtsSessionId << "without IPC endpoint";
		m_uptime.restart();
		return;
	}

	m_ipcEndpointName = endpoint->name;

	// the token is passed via stdin only; runProgramInSession() makes handles inheritable only
	// when stdin data is passed
	auto stdInData = endpoint->credentialsLine.toUnprotectedByteArray();
	endpoint->credentialsLine.clear();

	vInfo() << "Starting server for WTS session" << m_wtsSessionId << "with user" << user;
	m_processHandle = WindowsCoreFunctions::runProgramInSession(VeyonCore::filesystem().serverFilePath(), {},
																{},
																baseProcessId, {}, stdInData);

	SecureZeroMemory(stdInData.data(), size_t(stdInData.size()));

	if (m_processHandle.isInvalid())
	{
		vCritical() << "Failed to start server!";
		m_dataManager.removeServerEndpoint(m_ipcEndpointName);
		m_ipcEndpointName.clear();
	}
	else
	{
		m_dataManager.setServerProcessId(m_ipcEndpointName, IpcServer::ProcessId(GetProcessId(m_processHandle.get())));
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

	m_dataManager.removeServerEndpoint(m_ipcEndpointName);
	m_ipcEndpointName.clear();
}
