/*
 * WtsSessionManager.cpp - implementation of WtsSessionManager class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

	sessionList.reserve( sessionCount );

	for( DWORD sessionIndex = 0; sessionIndex < sessionCount; ++sessionIndex )
	{
		auto session = &sessions[sessionIndex];
		if( session->State == WTSActive )
		{
			sessionList.append( session->SessionId );
		}
	}

	return sessionList;
}



QString WtsSessionManager::querySessionInformation( SessionId sessionId, SessionInfo sessionInfo )
{
	WTS_INFO_CLASS infoClass = WTSInitialProgram;

	switch( sessionInfo )
	{
	case SessionInfoUserName: infoClass = WTSUserName; break;
	case SessionInfoWinStationName: infoClass = WTSWinStationName; break;
	case SessionInfoDomainName: infoClass = WTSDomainName; break;
	default:
		qCritical() << Q_FUNC_INFO << "invalid session info" << sessionInfo << "requested";
		return QString();
	}

	QString result;
	LPTSTR pBuffer = nullptr;
	DWORD dwBufferLen;

	if( WTSQuerySessionInformation( WTS_CURRENT_SERVER_HANDLE, sessionId, infoClass,
									&pBuffer, &dwBufferLen ) )
	{
		result = QString::fromWCharArray( pBuffer );
	}

	WTSFreeMemory( pBuffer );

	return result;
}



DWORD WtsSessionManager::findWinlogonProcessId( SessionId sessionId )
{
	PWTS_PROCESS_INFO processInfo = nullptr;
	DWORD processCount = 0;
	DWORD pid = static_cast<DWORD>( -1 );

	if( WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &processInfo, &processCount ) == false )
	{
		return pid;
	}

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


DWORD WtsSessionManager::findProcessId( const QString& userName )
{
	DWORD sidLen = SECURITY_MAX_SID_SIZE; // Flawfinder: ignore
	char userSID[SECURITY_MAX_SID_SIZE]; // Flawfinder: ignore
	wchar_t domainName[MAX_PATH]; // Flawfinder: ignore
	domainName[0] = 0;
	DWORD domainLen = MAX_PATH;
	SID_NAME_USE sidNameUse;

	if( LookupAccountName( nullptr,		// system name
						   WindowsCoreFunctions::toConstWCharArray( userName ),
						   userSID,
						   &sidLen,
						   domainName,
						   &domainLen,
						   &sidNameUse ) == false )
	{
		qCritical( "Could not look up SID structure" );
		return -1;
	}

	PWTS_PROCESS_INFO processInfo;
	DWORD processCount = 0;

	if( WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &processInfo, &processCount ) == false )
	{
		return -1;
	}

	DWORD pid = -1;

	for( DWORD proc = 0; proc < processCount; ++proc )
	{
		if( processInfo[proc].ProcessId > 0 &&
				EqualSid( processInfo[proc].pUserSid, userSID ) )
		{
			pid = processInfo[proc].ProcessId;
			break;
		}
	}

	WTSFreeMemory( processInfo );

	return pid;
}



QStringList WtsSessionManager::loggedOnUsers()
{
	QStringList users;

	PWTS_SESSION_INFO sessionInfo = nullptr;
	DWORD sessionCount = 0;

	if( WTSEnumerateSessions( WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessionInfo, &sessionCount ) == false )
	{
		return users;
	}

	for( DWORD session = 0; session < sessionCount; ++session )
	{
		if( sessionInfo[session].State != WTSActive )
		{
			continue;
		}

		LPTSTR userBuffer = nullptr;
		DWORD bytesReturned = 0;
		if( WTSQuerySessionInformation( WTS_CURRENT_SERVER_HANDLE, sessionInfo[session].SessionId, WTSUserName,
										&userBuffer, &bytesReturned ) == false ||
				userBuffer == nullptr )
		{
			continue;
		}

		const auto user = QString::fromWCharArray( userBuffer );
		if( user.isEmpty() == false && users.contains( user ) == false )
		{
			users.append( user );
		}

		WTSFreeMemory( userBuffer );
	}

	WTSFreeMemory( sessionInfo );

	return users;
}
