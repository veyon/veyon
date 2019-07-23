/*
 * WindowsUserFunctions.cpp - implementation of WindowsUserFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <winsock2.h>
#include <windows.h>
#include <lm.h>
#include <ntsecapi.h>

#include "VeyonConfiguration.h"
#include "WindowsCoreFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsUserFunctions.h"
#include "WtsSessionManager.h"

#include "authSSP.h"


static bool storePassword( const QString& password )
{
	LSA_OBJECT_ATTRIBUTES loa;
	loa.Length = sizeof(loa);
	loa.RootDirectory = nullptr;
	loa.ObjectName = nullptr;
	loa.Attributes = 0;
	loa.SecurityDescriptor = nullptr;
	loa.SecurityQualityOfService = nullptr;

	LSA_HANDLE lh;

	auto status = LsaOpenPolicy( nullptr, &loa, POLICY_CREATE_SECRET, &lh );

	if( status )
	{
		vCritical() << "Error opening LSA policy:" << LsaNtStatusToWinError( status );
		return false;
	}

	const auto nameWide = WindowsCoreFunctions::toWCharArray( QStringLiteral("DefaultPassword") );

	LSA_UNICODE_STRING name;
	name.Buffer = nameWide.data();
	name.MaximumLength = name.Length = static_cast<USHORT>( wcslen(name.Buffer) - 2 );

	if( password.isEmpty() )
	{
		status = LsaStorePrivateData( lh, &name, nullptr );
		if( status )
		{
			vCritical() << "Error clearing stored password:" << LsaNtStatusToWinError( status );
			return false;
		}
		return true;
	}

	const auto passwordWide = WindowsCoreFunctions::toWCharArray( password );

	LSA_UNICODE_STRING data;
	data.Buffer = passwordWide.data();
	data.MaximumLength = data.Length = static_cast<USHORT>( wcslen(data.Buffer) * sizeof(*data.Buffer) );

	status = LsaStorePrivateData( lh, &name, &data );

	LsaClose(lh);

	if( status )
	{
		vCritical() << "Error storing password:" << LsaNtStatusToWinError( status );
		return false;
	}

	return true;
}



static bool setAutologon( const QString& username, const QString& password, const QString& domainName )
{
	HKEY h;

	auto err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
							 0, KEY_WOW64_64KEY | KEY_SET_VALUE, &h );
	if( err != ERROR_SUCCESS )
	{
		vCritical() << "Unable to open Winlogon subkey:" << err;
		return false;
	}

	if( storePassword( password ) == false )
	{
		return false;
	}

	const auto usernameWide = WindowsCoreFunctions::toWCharArray( username );
	const auto domainNameWide = WindowsCoreFunctions::toWCharArray( domainName );

	err = RegSetValueEx( h, L"DefaultUserName", 0, REG_SZ, reinterpret_cast<CONST BYTE *>( usernameWide.data() ),
						 static_cast<DWORD>( ( username.length() + 1 ) * 2 ) );
	if( err != ERROR_SUCCESS )
	{
		vCritical() << "Unable to set default logon user name:" << err;
		return false;
	}

	err = RegSetValueEx( h, L"DefaultDomainName", 0, REG_SZ, reinterpret_cast<CONST BYTE *>( domainNameWide.data() ),
						 static_cast<DWORD>( ( domainName.length() + 1 ) * 2 ) );
	if( err != ERROR_SUCCESS )
	{
		vCritical() << "Unable to set default domain name:" << err;
		return false;
	}

	err = RegSetValueEx( h, L"AutoAdminLogon", 0, REG_SZ, reinterpret_cast<CONST BYTE *>( L"1" ), 2 );
	if( err != ERROR_SUCCESS )
	{
		vCritical() << "Unable to set automatic logon flag" << err;
		return false;
	}

	err = RegSetValueEx( h, L"ForceAutoLogon", 0, REG_SZ, reinterpret_cast<CONST BYTE *>( L"1" ), 2 );
	if( err != ERROR_SUCCESS )
	{
		vCritical() << "Unable to set forced automatic logon flag:" << err;
		return false;
	}

	return true;
}



static bool clearAutologon()
{
	HKEY h;

	auto i = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
						   0, KEY_WOW64_64KEY | KEY_SET_VALUE, &h );
	if( i != ERROR_SUCCESS )
	{
		vCritical() << "Unable to open Winlogon subkey" << i;
		return false;
	}

	i = RegSetValueEx(h, L"DefaultUserName", 0, REG_SZ, reinterpret_cast<const BYTE *>( L"" ), 1 );
	if( i != ERROR_SUCCESS )
	{
		vCritical() << "Unable to set default logon user name" << i;
		return false;
	}

	if( storePassword( {} ) == false )
	{
		return false;
	}

	i = RegDeleteValue( h, L"DefaultPassword" );
	if( i != ERROR_SUCCESS && i != ERROR_FILE_NOT_FOUND )
	{
		vCritical() << "Unable to remove default logon password" << i;
		return false;
	}

	i = RegDeleteValue( h, L"ForceAutoLogon" );
	if( i != ERROR_SUCCESS && i != ERROR_FILE_NOT_FOUND )
	{
		vCritical() << "Unable to remove force logon flag" << i;
		return false;
	}

	i = RegSetValueEx( h, L"AutoAdminLogon", 0, REG_SZ, reinterpret_cast<CONST BYTE *>( L"0" ), 2 );
	if( i != ERROR_SUCCESS )
	{
		vCritical() << "Unable to clear automatic logon flag" << i;
		return false;
	}

	return true;
}



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
	return WtsSessionManager::loggedOnUsers().isEmpty() == false;
}



QString WindowsUserFunctions::currentUser()
{
	const auto sessionId = WtsSessionManager::currentSession();

	auto username = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfo::UserName );
	auto domainName = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfo::DomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		wchar_t computerName[MAX_PATH]; // Flawfinder: ignore
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( domainName == QString::fromWCharArray( computerName ) )
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



bool WindowsUserFunctions::logon( const QString& username, const QString& password )
{
	if( isAnyUserLoggedOn() )
	{
		vInfo() << "Skipping user logon as a user is already logged on";
		return false;
	}

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

	if( setAutologon( user, domain, password ) == false )
	{
		vCritical() << "Failed to logon user";
		return false;
	}

	VeyonCore::platform().coreFunctions().reboot();

	return true;
}



void WindowsUserFunctions::logoff()
{
	ExitWindowsEx( EWX_LOGOFF | (EWX_FORCE | EWX_FORCEIFHUNG), SHTDN_REASON_MAJOR_OTHER );
}



bool WindowsUserFunctions::authenticate( const QString& username, const QString& password )
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
	auto passwordWide = WindowsCoreFunctions::toWCharArray( password );

	bool result = false;

	WindowsPlatformConfiguration config( &VeyonCore::config() );

	if( config.disableSSPIBasedUserAuthentication() )
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

	if( NetGroupEnum( WindowsCoreFunctions::toConstWCharArray( dc ), 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
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
