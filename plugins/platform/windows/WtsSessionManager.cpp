/*
 * WtsSessionManager.cpp - implementation of WtsSessionManager class
 *
 * Copyright (c) 2018-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <sddl.h>
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
	PWTS_SESSION_INFO sessions;
	DWORD sessionCount = 0;

	const auto result = WTSEnumerateSessions( WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &sessionCount );
	if( result == false )
	{
		return {};
	}

	SessionList sessionList;
	sessionList.reserve( sessionCount );

	for( DWORD sessionIndex = 0; sessionIndex < sessionCount; ++sessionIndex )
	{
		const auto session = &sessions[sessionIndex];
		if( session->State == WTSActive ||
			QString::fromWCharArray(session->pWinStationName)
			.compare( QLatin1String("multiseat"), Qt::CaseInsensitive ) == 0 )
		{
			sessionList.append( session->SessionId );
		}
	}

	WTSFreeMemory( sessions );

	return sessionList;
}



QString WtsSessionManager::querySessionInformation(SessionId sessionId, SessionInfo sessionInfo)
{
	if (sessionId == InvalidSession)
	{
		vCritical() << "called with invalid session ID";
		return {};
	}

	WTS_INFO_CLASS infoClass{};

	switch (sessionInfo)
	{
	case SessionInfo::UserName: infoClass = WTSUserName; break;
	case SessionInfo::DomainName: infoClass = WTSDomainName; break;
	case SessionInfo::SessionUptime: infoClass = WTSSessionInfo; break;
	case SessionInfo::ClientAddress: infoClass = WTSClientAddress; break;
	case SessionInfo::ClientName: infoClass = WTSClientName; break;
	default:
		vCritical() << "invalid session info" << sessionInfo << "requested";
		return {};
	}

	QString result;
	LPWSTR queryBuffer = nullptr;
	DWORD bufferLen;

	if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionId, infoClass, &queryBuffer, &bufferLen))
	{
		switch (infoClass)
		{
		case WTSClientAddress:
		{
			const auto clientAddress = PWTS_CLIENT_ADDRESS(queryBuffer);
			switch (clientAddress->AddressFamily)
			{
			case AF_UNSPEC:
				result = QString::fromLatin1(reinterpret_cast<const char *>(clientAddress->Address));
				break;
			case AF_INET:
				result = QStringLiteral("%1.%2.%3.%4")
						 .arg(int(clientAddress->Address[2]))
						.arg(int(clientAddress->Address[3]))
						.arg(int(clientAddress->Address[4]))
						.arg(int(clientAddress->Address[5]));
				break;
			case AF_INET6:
				for (int i = 2; i < 18; i++)
				{
					if (i != 2 && i % 2 == 0)
					{
						result.append(QLatin1Char(':'));
					}
					result.append(QStringLiteral("%1").arg( int(clientAddress->Address[i]), 16, 2, QLatin1Char('0')));
				}
				break;
			}
			break;
		}
		case WTSSessionInfo:
		{
			const auto sessionInfoData = PWTSINFO(queryBuffer);
			switch (sessionInfo)
			{
			case SessionInfo::SessionUptime:
				result = QString::number((sessionInfoData->CurrentTime.QuadPart -
										  qMax(sessionInfoData->ConnectTime.QuadPart, sessionInfoData->LogonTime.QuadPart))
										 / (1000*1000*10));
				break;
			default:
				vCritical() << "unhandled session info" << sessionInfo;
				break;
			}
			break;
		}
		default:
			result = QString::fromWCharArray(queryBuffer);
			break;
		}
	}
	else
	{
		const auto lastError = GetLastError();
		vCritical() << lastError;
	}

	WTSFreeMemory(queryBuffer);

	return result;
}



QString WtsSessionManager::queryUserSid(SessionId sessionId)
{
	HANDLE userToken = nullptr;
	if (WTSQueryUserToken(sessionId, &userToken) == false)
	{
		vCritical() << "could not query user token for session" << sessionId;
		return {};
	}

	DWORD tokenSize = 0;
	if (GetTokenInformation(userToken, TokenUser, nullptr, 0, &tokenSize) ||
		tokenSize == 0 ||
		GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		CloseHandle(userToken);
		return {};
	}

	const auto userInfo = reinterpret_cast<PTOKEN_USER>(HeapAlloc(GetProcessHeap(), 0, tokenSize));
	if (!userInfo)
	{
		CloseHandle(userToken);
		return {};
	}

	if (!GetTokenInformation(userToken, TokenUser, userInfo, tokenSize, &tokenSize))
	{
		HeapFree(GetProcessHeap(), 0, userInfo);
		CloseHandle(userToken);
		return {};
	}

	QString sid;
	wchar_t* stringSid = nullptr;
	if (ConvertSidToStringSid(userInfo->User.Sid, &stringSid) && stringSid)
	{
		sid = QString::fromWCharArray(stringSid);
		LocalFree(stringSid);
	}

	HeapFree(GetProcessHeap(), 0, userInfo);
	CloseHandle(userToken);

	return sid;
}



bool WtsSessionManager::isRemote( SessionId sessionId )
{
	const auto WTSIsRemoteSession = static_cast<WTS_INFO_CLASS>(29);

	BOOL *isRDP = nullptr;
	DWORD dataLen = 0;

	if( WTSQuerySessionInformation( WTS_CURRENT_SERVER_HANDLE, sessionId, WTSIsRemoteSession,
									reinterpret_cast<LPTSTR *>(&isRDP), &dataLen ) &&
		isRDP )
	{
		const auto result = *isRDP;
		WTSFreeMemory( isRDP );
		return result;
	}

	return false;
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



WtsSessionManager::ProcessId WtsSessionManager::findProcessId(const QString& processName, SessionId sessionId)
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

		if (sessionId != InvalidSession && processInfo[proc].SessionId != sessionId)
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
