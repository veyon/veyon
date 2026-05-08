/*
 * WindowsUserFunctions.cpp - implementation of WindowsUserFunctions class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <dsgetdc.h>
#include <wtsapi32.h>

#include <QBuffer>

#include "DesktopInputController.h"
#include "VeyonConfiguration.h"
#include "WindowsCoreFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsUserFunctions.h"
#include "WtsSessionManager.h"

#include "authSSP.h"

#define XK_MISCELLANY

#include "keysymdef.h"


static std::optional<QByteArray> tokenUserSid(HANDLE token)
{
	DWORD needed = 0;
	GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || needed == 0)
	{
		return std::nullopt;
	}

	QByteArray buffer(static_cast<int>(needed), Qt::Uninitialized);
	if (!GetTokenInformation(token, TokenUser, buffer.data(), needed, &needed))
	{
		return std::nullopt;
	}

	return buffer;
}



static HANDLE openCurrentEffectiveToken()
{
	HANDLE token = nullptr;

	if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token))
	{
		return token;
	}

	if (GetLastError() == ERROR_NO_TOKEN &&
		OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
	{
		return token;
	}

	return nullptr;
}



static bool isCurrentEffectiveUser(HANDLE sessionToken)
{
	HANDLE currentToken = openCurrentEffectiveToken();
	if (!currentToken)
	{
		return false;
	}

	auto closeHandle = qScopeGuard([&]() {
		CloseHandle(currentToken);
	});

	const auto currentUserBuffer = tokenUserSid(currentToken);
	const auto sessionUserBuffer = tokenUserSid(sessionToken);
	if (!currentUserBuffer || !sessionUserBuffer)
	{
		return false;
	}

	const auto currentUser = reinterpret_cast<const TOKEN_USER*>(currentUserBuffer->constData());
	const auto sessionUser = reinterpret_cast<const TOKEN_USER*>(sessionUserBuffer->constData());

	return EqualSid(currentUser->User.Sid, sessionUser->User.Sid);
}



QString WindowsUserFunctions::queryCurrentUserProperty(UserProperty property)
{
	switch (property)
	{
	case UserProperty::LoginName: return currentUserLoginName();
	case UserProperty::FullName: return currentUserFullName();
	case UserProperty::None: break;
	}
	return {};
}



QStringList WindowsUserFunctions::userGroups( bool queryDomainGroups )
{
	auto groupList = localUserGroups();

	if( queryDomainGroups )
	{
		groupList.append( domainUserGroups() );
	}

	groupList.removeDuplicates();
	groupList.removeAll( {} );

	return groupList;
}



QStringList WindowsUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	auto groupList = localGroupsOfUser( username );

	if( queryDomainGroups )
	{
		groupList.append( domainGroupsOfUser( username ) );
	}

	groupList.removeDuplicates();
	groupList.removeAll( {} );

	return groupList;
}



QString WindowsUserFunctions::userGroupSecurityIdentifier(const QString& groupName)
{
	WindowsCoreFunctions::SecurityIdentifierBuffer groupSid{};
	DWORD sidLen = groupSid.size();

	SID_NAME_USE sidNameUse;

	if (LookupAccountName(nullptr, WindowsCoreFunctions::toConstWCharArray(groupName),
						  groupSid.data(), &sidLen,
						  NULL, 0, &sidNameUse) == false)
	{
		vCritical() << "Could not look up SID structure:" << GetLastError();
		return {};
	}

	return WindowsCoreFunctions::securityIdentifierToString(groupSid);
}



bool WindowsUserFunctions::isAnyUserLoggedInLocally()
{
	const auto sessions = WtsSessionManager::activeSessions();
	for( const auto session : sessions )
	{
		if( WtsSessionManager::querySessionInformation( session, WtsSessionManager::SessionInfo::UserName ).isEmpty() == false &&
			WtsSessionManager::isRemote( session ) == false )
		{
			return true;
		}
	}

	return false;
}



bool WindowsUserFunctions::isAnyUserLoggedInRemotely()
{
	const auto sessions = WtsSessionManager::activeSessions();
	for( const auto session : sessions )
	{
		if( WtsSessionManager::querySessionInformation( session, WtsSessionManager::SessionInfo::UserName ).isEmpty() == false &&
			WtsSessionManager::isRemote( session ) )
		{
			return true;
		}
	}

	return false;
}



bool WindowsUserFunctions::prepareLogon( const QString& username, const Password& password )
{
	if( m_logonHelper.prepare( username, password ) )
	{
		logoff();
		return true;
	}

	return false;
}



bool WindowsUserFunctions::performLogon( const QString& username, const Password& password )
{
	WindowsPlatformConfiguration config( &VeyonCore::config() );

	DesktopInputController input( config.logonKeyPressInterval() );

	const auto ctrlAltDel = []() {
		auto sasEvent = OpenEvent( EVENT_MODIFY_STATE, false, L"Global\\VeyonServiceSasEvent" );
		SetEvent( sasEvent );
		CloseHandle( sasEvent );
	};

	const auto sendString = [&input]( const QString& string ) {
		for( const auto& character : string )
		{
			input.pressAndReleaseKey( character );
		}
	};

	WindowsInputDeviceFunctions::stopOnScreenKeyboard();

	ctrlAltDel();

	QThread::msleep( static_cast<unsigned long>( config.logonInputStartDelay() ) );

	if( config.logonConfirmLegalNotice() )
	{
		input.pressAndReleaseKey( XK_Return );
		QThread::msleep( static_cast<unsigned long>( config.logonInputStartDelay() ) );
	}

	input.pressAndReleaseKey( XK_Delete );

	sendString( username );

	input.pressAndReleaseKey( XK_Tab );

	sendString( QString::fromUtf8( password.toByteArray() ) );

	input.pressAndReleaseKey( XK_Return );

	return true;
}



void WindowsUserFunctions::logoff()
{
	ExitWindowsEx( EWX_LOGOFF | (EWX_FORCE | EWX_FORCEIFHUNG), SHTDN_REASON_MAJOR_OTHER );
}



bool WindowsUserFunctions::authenticate( const QString& username, const Password& password )
{
	QString domain;
	QString user;

	const auto userNameParts = username.split( QLatin1Char('\\') );
	if( userNameParts.count() == 2 )
	{
		domain = userNameParts[0];
		user = userNameParts[1];
	}
	else
	{
		user = username;
	}

	auto domainWide = WindowsCoreFunctions::toWCharArray( domain );
	auto userWide = WindowsCoreFunctions::toWCharArray( user );
	auto passwordWide = WindowsCoreFunctions::toWCharArray( QString::fromUtf8( password.toByteArray() ) );

	bool result = false;

	if( WindowsPlatformConfiguration( &VeyonCore::config() ).disableSSPIBasedUserAuthentication() )
	{
		for (auto logonProvider: {LOGON32_PROVIDER_DEFAULT, LOGON32_PROVIDER_WINNT50, LOGON32_PROVIDER_WINNT40})
		{
			HANDLE token = nullptr;
			result = LogonUserW(userWide.data(), domain.isEmpty() ? nullptr : domainWide.data(), passwordWide.data(),
								LOGON32_LOGON_NETWORK, logonProvider, &token);
			const auto error = GetLastError();
			vDebug() << "LogonUserW()" << logonProvider << result << error;
			if (token)
			{
				CloseHandle(token);
				break;
			}
		}
	}
	else
	{
		result = SSPLogonUser( domainWide.data(), userWide.data(), passwordWide.data() );
		const auto error = GetLastError();
		vDebug() << "SSPLogonUser()" << result << error;
	}

	return result;
}



QString WindowsUserFunctions::currentUserLoginName()
{
	const auto sessionId = WtsSessionManager::currentSession();

	auto username = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfo::UserName );
	auto domainName = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfo::DomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		std::array<wchar_t, MAX_COMPUTERNAME_LENGTH+1> computerName{}; // Flawfinder: ignore
		DWORD size = MAX_COMPUTERNAME_LENGTH;
		GetComputerName( computerName.data(), &size );

		if( domainName == QString::fromWCharArray( computerName.data() ) )
		{
			// reset domain name as we do not need to store local computer name
			domainName.clear();
		}
	}

	if( domainName.isEmpty() )
	{
		return username;
	}

	return domainName + QLatin1Char('\\') + username;
}



QString WindowsUserFunctions::currentUserFullName()
{
	HANDLE sessionToken = nullptr;
	const auto sessionId = WtsSessionManager::currentSession();

	if (!WTSQueryUserToken(sessionId, &sessionToken))
	{
		vCritical() << "could not query user token for session" << sessionId;
		return {};
	}

	auto closeHandle = qScopeGuard([&]() {
		CloseHandle(sessionToken);
	});

	const bool needImpersonation = !isCurrentEffectiveUser(sessionToken);

	bool impersonating = false;
	if (needImpersonation)
	{
		if (!ImpersonateLoggedOnUser(sessionToken))
		{
			vCritical() << "could not impersonate session user";
			return {};
		}
		impersonating = true;
	}

	std::array<wchar_t, 256> nameBuffer{};
	auto nameLength = DWORD(nameBuffer.size());

	QString fullName;
	if (GetUserNameExW(NameDisplay, nameBuffer.data(), &nameLength))
	{
		if (nameLength > 0)
		{
			fullName = QString::fromWCharArray(nameBuffer.data(), nameLength);
		}
	}
	else
	{
		const auto error = GetLastError();
		vWarning() << "GetUserNameExW failed:" << error;
	}

	if (impersonating)
	{
		RevertToSelf();
	}

	return fullName;
}



QString WindowsUserFunctions::domainFromUsername( const QString& username )
{
	const auto nameParts = username.split( QLatin1Char('\\') );
	if( nameParts.size() > 1 )
	{
		return nameParts[0];
	}

	return {};
}



QString WindowsUserFunctions::domainController( const QString& domainName )
{
	const auto domainNamePointer = domainName.isEmpty() ? nullptr
														: WindowsCoreFunctions::toConstWCharArray(domainName);

	PDOMAIN_CONTROLLER_INFO dcInfo;

	const auto dsResult = DsGetDcName( nullptr, domainNamePointer, nullptr, nullptr, DS_DIRECTORY_SERVICE_REQUIRED, &dcInfo );
	if( dsResult == ERROR_SUCCESS )
	{
		const auto dcName = QString::fromUtf16(reinterpret_cast<const char16_t *>(dcInfo->DomainControllerName)).
							replace( QLatin1Char('\\'), QString() );

		NetApiBufferFree( dcInfo );

		return dcName;
	}

	vWarning() << "DsGetDcName() failed with" << dsResult << "falling back to NetGetDCName()";

	LPBYTE outBuffer = nullptr;
	const auto netResult = NetGetDCName( nullptr, domainNamePointer, &outBuffer );
	if( netResult == NERR_Success )
	{
		const auto dcName = QString::fromUtf16(reinterpret_cast<const char16_t *>(outBuffer));

		NetApiBufferFree( outBuffer );

		return dcName;
	}

	vWarning() << "querying domain controller for domain" << domainName << "failed with:" << netResult;

	return {};
}



QStringList WindowsUserFunctions::domainUserGroups()
{
	const auto dc = domainController();

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	const auto status = NetGroupEnum( WindowsCoreFunctions::toConstWCharArray(dc), 0, &outBuffer, MAX_PREFERRED_LENGTH,
									  &entriesRead, &totalEntries, nullptr );
	if( status == NERR_Success )
	{
		const auto* groupInfos = reinterpret_cast<GROUP_INFO_0 *>( outBuffer );

		QStringList groupList;
		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16(reinterpret_cast<const char16_t *>(groupInfos[i].grpi0_name));
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all domain groups fetched from DC" << dc << entriesRead << totalEntries;
		}

		NetApiBufferFree( outBuffer );

		return groupList;
	}
	else
	{
		vWarning() << "failed to fetch domain groups from DC" << dc << status;
	}

	return {};
}



QStringList WindowsUserFunctions::domainGroupsOfUser( const QString& username )
{
	const auto dc = domainController( domainFromUsername(username) );
	const auto usernameWithoutDomain = VeyonCore::stripDomain(username);

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	const auto status = NetUserGetGroups( WindowsCoreFunctions::toConstWCharArray(dc),
										  WindowsCoreFunctions::toConstWCharArray(usernameWithoutDomain),
										  0, &outBuffer, MAX_PREFERRED_LENGTH,
										  &entriesRead, &totalEntries );
	if( status == NERR_Success )
	{
		const auto* groupUsersInfo = reinterpret_cast<GROUP_USERS_INFO_0 *>( outBuffer );

		QStringList groupList;
		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16(reinterpret_cast<const char16_t *>(groupUsersInfo[i].grui0_name));
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all domain groups fetched for user" << username << "from DC" << dc
					   << entriesRead << totalEntries;
		}

		NetApiBufferFree( outBuffer );

		return groupList;
	}
	else
	{
		vWarning() << "failed to fetch domain groups for user" << username << "from DC" << dc << status;
	}

	return {};
}



QStringList WindowsUserFunctions::localUserGroups()
{
	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	const auto result = NetLocalGroupEnum( nullptr, 0, &outBuffer, MAX_PREFERRED_LENGTH,
										   &entriesRead, &totalEntries, nullptr );
	if( result == NERR_Success )
	{
		const auto* groupInfos = reinterpret_cast<LOCALGROUP_INFO_0 *>( outBuffer );

		QStringList groupList;
		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16(reinterpret_cast<const char16_t *>(groupInfos[i].lgrpi0_name));
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all local groups fetched" << entriesRead << totalEntries;
		}

		NetApiBufferFree( outBuffer );

		return groupList;
	}
	else
	{
		vWarning() << "failed to fetch local groups:" << result;
	}

	return {};
}



QStringList WindowsUserFunctions::localGroupsOfUser( const QString& username )
{
	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	const auto result = NetUserGetLocalGroups( nullptr, WindowsCoreFunctions::toConstWCharArray(username),
											   0, 0, &outBuffer, MAX_PREFERRED_LENGTH,
											   &entriesRead, &totalEntries );
	if( result == NERR_Success )
	{
		const auto* localGroupUsersInfo = reinterpret_cast<LOCALGROUP_USERS_INFO_0 *>( outBuffer );

		QStringList groupList;
		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16(reinterpret_cast<const char16_t *>(localGroupUsersInfo[i].lgrui0_name));
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all local groups fetched for user" << username << entriesRead << totalEntries;
		}

		NetApiBufferFree( outBuffer );

		return groupList;
	}
	else
	{
		vWarning() << "failed to fetch local groups for user" << username << result;
	}

	return {};
}
