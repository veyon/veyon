/*
 * WindowsUserFunctions.cpp - implementation of WindowsUserFunctions class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QBuffer>

#include "DesktopInputController.h"
#include "VariantStream.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"
#include "WindowsCoreFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsServiceControl.h"
#include "WindowsServiceCore.h"
#include "WindowsUserFunctions.h"
#include "WtsSessionManager.h"

#include "authSSP.h"

#define XK_MISCELLANY

#include "keysymdef.h"


QString WindowsUserFunctions::fullName( const QString& username )
{
	QString fullName;

	auto realUsername = username;
	LPWSTR domainController = nullptr;

	const auto nameParts = username.split( QLatin1Char('\\') );
	if( nameParts.size() > 1 )
	{
		realUsername = nameParts[1];
		if( NetGetDCName( nullptr, WindowsCoreFunctions::toConstWCharArray( nameParts[0] ),
						  reinterpret_cast<LPBYTE *>( &domainController ) ) != NERR_Success )
		{
			domainController = nullptr;
		}
	}

	LPUSER_INFO_2 buf = nullptr;
	const auto nStatus = NetUserGetInfo( domainController,
										 WindowsCoreFunctions::toConstWCharArray( realUsername ),
										 2, reinterpret_cast<LPBYTE *>( &buf ) );

	if( nStatus == NERR_Success && buf != nullptr )
	{
		fullName = QString::fromWCharArray( buf->usri2_full_name );
	}

	if( buf != nullptr )
	{
		NetApiBufferFree( buf );
	}

	if( domainController != nullptr )
	{
		NetApiBufferFree( domainController );
	}

	return fullName;
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



bool WindowsUserFunctions::isAnyUserLoggedOn()
{
	const auto sessions = WtsSessionManager::activeSessions();
	for( const auto session : sessions )
	{
		if( WtsSessionManager::querySessionInformation( session, WtsSessionManager::SessionInfo::UserName ).isEmpty() == false )
		{
			return true;
		}
	}

	return false;
}



QString WindowsUserFunctions::currentUser()
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
		HANDLE token = nullptr;
		result = LogonUserW( userWide.data(), domainWide.data(), passwordWide.data(),
							 LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &token );
		vDebug() << "LogonUserW()" << result << GetLastError();
		if( token )
		{
			CloseHandle( token );
		}
	}
	else
	{
		result = SSPLogonUser( domainWide.data(), userWide.data(), passwordWide.data() );
		vDebug() << "SSPLogonUser()" << result << GetLastError();
	}

	return result;
}



QString WindowsUserFunctions::domainController()
{
	QString dcName;
	LPBYTE outBuffer = nullptr;

	if( NetGetDCName( nullptr, nullptr, &outBuffer ) == NERR_Success )
	{
		dcName = QString::fromUtf16( reinterpret_cast<const ushort *>( outBuffer ) );

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not query domain controller name!";
	}

	return dcName;
}



QStringList WindowsUserFunctions::domainUserGroups()
{
	const auto dc = domainController();

	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetGroupEnum( WindowsCoreFunctions::toConstWCharArray( dc ), 0, &outBuffer, MAX_PREFERRED_LENGTH,
					  &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		const auto* groupInfos = reinterpret_cast<GROUP_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( groupInfos[i].grpi0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all domain groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch domain groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::domainGroupsOfUser( const QString& username )
{
	const auto dc = domainController();

	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	const auto usernameWithoutDomain = VeyonCore::stripDomain( username );

	if( NetUserGetGroups( WindowsCoreFunctions::toConstWCharArray( dc ),
						  WindowsCoreFunctions::toConstWCharArray( usernameWithoutDomain ),
						  0, &outBuffer, MAX_PREFERRED_LENGTH,
						  &entriesRead, &totalEntries ) == NERR_Success )
	{
		const auto* groupUsersInfo = reinterpret_cast<GROUP_USERS_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( groupUsersInfo[i].grui0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all domain groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch domain groups for user" << username;
	}

	return groupList;
}



QStringList WindowsUserFunctions::localUserGroups()
{
	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetLocalGroupEnum( nullptr, 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		const auto* groupInfos = reinterpret_cast<LOCALGROUP_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( groupInfos[i].lgrpi0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all local groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch local groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::localGroupsOfUser( const QString& username )
{
	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetUserGetLocalGroups( nullptr, WindowsCoreFunctions::toConstWCharArray( username ),
							   0, 0, &outBuffer, MAX_PREFERRED_LENGTH,
							   &entriesRead, &totalEntries ) == NERR_Success )
	{
		const auto* localGroupUsersInfo = reinterpret_cast<LOCALGROUP_USERS_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( localGroupUsersInfo[i].lgrui0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all local groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch local groups for user" << username;
	}

	return groupList;
}
