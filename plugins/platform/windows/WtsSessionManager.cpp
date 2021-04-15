/*
 * WtsSessionManager.cpp - implementation of WtsSessionManager class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>
#include <wtsapi32.h>

#include "WindowsCoreFunctions.h"
#include "WtsSessionManager.h"


WtsSessionManager::SessionId WtsSessionManager::currentSession()
{
	auto sessionId = InvalidSession;
	if( ProcessIdToSessionId( GetCurrentProcessId(), &sessionId ) == 0 )
	{
		vWarning() << GetLastError();
		return InvalidSession;
	}

	return sessionId;
}



WtsSessionManager::SessionId WtsSessionManager::activeConsoleSession()
{
	return WTSGetActiveConsoleSessionId();
}



WtsSessionManager::SessionList WtsSessionManager::activeSessions()
{
	SessionList sessionList;

	PWTS_SESSION_INFO sessions;
	DWORD sessionCount = 0;

	auto result = WTSEnumerateSessions( WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &sessionCount );
	if( result == false )
	{
		return sessionList;
	}

	sessionList.reserve( int(sessionCount) );

	for( DWORD sessionIndex = 0; sessionIndex < sessionCount; ++sessionIndex )
	{
		auto session = &sessions[sessionIndex];
		if( session->State == WTSActive )
		{
			sessionList.append( session->SessionId );
		}
	}

	WTSFreeMemory( sessions );

	return sessionList;
}



QString WtsSessionManager::querySessionInformation( SessionId sessionId, SessionInfo sessionInfo )
{
	if( sessionId == InvalidSession )
	{
		vCritical() << "called with invalid session ID";
		return {};
	}

	WTS_INFO_CLASS infoClass = WTSInitialProgram;

	switch( sessionInfo )
	{
	case SessionInfo::UserName: infoClass = WTSUserName; break;
	case SessionInfo::DomainName: infoClass = WTSDomainName; break;
	default:
		vCritical() << "invalid session info" << sessionInfo << "requested";
		return {};
	}

	QString result;
	LPTSTR pBuffer = nullptr;
	DWORD dwBufferLen;

	if( WTSQuerySessionInformation( WTS_CURRENT_SERVER_HANDLE, sessionId, infoClass,
									&pBuffer, &dwBufferLen ) )
	{
		result = QString::fromWCharArray( pBuffer );
	}
	else
	{
		const auto lastError = GetLastError();
		vCritical() << lastError;
	}

	WTSFreeMemory( pBuffer );

	vDebug() << sessionId << sessionInfo << result;

	return result;
}



WtsSessionManager::ProcessId WtsSessionManager::findWinlogonProcessId( SessionId sessionId )
{
	if( sessionId == InvalidSession )
	{
		vCritical() << "called with invalid session ID";
		return InvalidProcess;
	}

	PWTS_PROCESS_INFO processInfo = nullptr;
	DWORD processCount = 0;

	if( WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &processInfo, &processCount ) == false )
	{
		return InvalidProcess;
	}

	auto pid = InvalidProcess;
	const auto processName = QStringLiteral("winlogon.exe");

	for( DWORD proc = 0; proc < processCount; ++proc )
	{
		if( processInfo[proc].ProcessId == 0 )
		{
			continue;
		}

		if( processName.compare( QString::fromWCharArray( processInfo[proc].pProcessName ), Qt::CaseInsensitive ) == 0 &&
			sessionId == processInfo[proc].SessionId )
		{
			pid = processInfo[proc].ProcessId;
			break;
		}
	}

	WTSFreeMemory( processInfo );

	return pid;
}



WtsSessionManager::ProcessId WtsSessionManager::findUserProcessId( const QString& userName )
{
	DWORD sidLen = SECURITY_MAX_SID_SIZE; // Flawfinder: ignore
	std::array<char, SECURITY_MAX_SID_SIZE> userSID{};
	std::array<wchar_t, DOMAIN_LENGTH> domainName{};
	DWORD domainLen = domainName.size();
	SID_NAME_USE sidNameUse;

	if( LookupAccountName( nullptr,		// system name
						   WindowsCoreFunctions::toConstWCharArray( userName ),
						   userSID.data(),
						   &sidLen,
						   domainName.data(),
						   &domainLen,
						   &sidNameUse ) == false )
	{
		vCritical() << "could not look up SID structure";
		return InvalidProcess;
	}

	PWTS_PROCESS_INFO processInfo;
	DWORD processCount = 0;

	if( WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &processInfo, &processCount ) == false )
	{
		vWarning() << "WTSEnumerateProcesses() failed:" << GetLastError();
		return InvalidProcess;
	}

	auto pid = InvalidProcess;

	for( DWORD proc = 0; proc < processCount; ++proc )
	{
		if( processInfo[proc].ProcessId > 0 &&
			processInfo[proc].pUserSid != nullptr &&
			EqualSid( processInfo[proc].pUserSid, userSID.data() ) )
		{
			pid = processInfo[proc].ProcessId;
			break;
		}
	}

	WTSFreeMemory( processInfo );

	return pid;
}



WtsSessionManager::ProcessId WtsSessionManager::findProcessId( const QString& processName )
{
	PWTS_PROCESS_INFO processInfo = nullptr;
	DWORD processCount = 0;

	if( WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &processInfo, &processCount ) == false )
	{
		return InvalidProcess;
	}

	auto pid = InvalidProcess;

	for( DWORD proc = 0; proc < processCount; ++proc )
	{
		if( processInfo[proc].ProcessId == 0 )
		{
			continue;
		}

		if( processName.compare( QString::fromWCharArray( processInfo[proc].pProcessName ), Qt::CaseInsensitive ) == 0 )
		{
			pid = processInfo[proc].ProcessId;
			break;
		}
	}

	WTSFreeMemory( processInfo );

	return pid;
}
