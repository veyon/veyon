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

#include <QBuffer>

#include "DesktopInputController.h"
#include "VariantStream.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"
#include "WindowsCoreFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsServiceCore.h"
#include "WindowsUserFunctions.h"
#include "WtsSessionManager.h"

#include "authSSP.h"

#define XK_MISCELLANY

#include "keysymdef.h"


WindowsUserFunctions::WindowsUserFunctions()
{
	connect( VeyonCore::instance(), &VeyonCore::applicationLoaded,
			 this, &WindowsUserFunctions::checkPendingLogonTasks );
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



bool WindowsUserFunctions::logon( const QString& username, const Password& password )
{
	if( isAnyUserLoggedOn() )
	{
		vInfo() << "Skipping user logon as a user is already logged on";
		return false;
	}

	if( writePersistentLogonCredentials( username, password ) )
	{
		logoff();
		return true;
	}

	vCritical() << "failed";

	return false;
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



void WindowsUserFunctions::checkPendingLogonTasks()
{
	// running inside a Veyon Server component?
	if( WindowsServiceCore::serviceDataTokenFromEnvironment().isEmpty() == false &&
		isAnyUserLoggedOn() == false )
	{
		vDebug() << "Reading logon credentials";
		QString username;
		Password password;
		if( readPersistentLogonCredentials( &username, &password ) )
		{
			clearPersistentLogonCredentials();

			performFakeInputLogon( username, password );
		}
	}
}



bool WindowsUserFunctions::readPersistentLogonCredentials( QString* username, Password* password )
{
	if( username == nullptr || password == nullptr )
	{
		vCritical() << "Invalid input pointers";
		return false;
	}

	auto logonData = ServiceDataManager::read( WindowsServiceCore::serviceDataTokenFromEnvironment() );
	if( logonData.isEmpty() )
	{
		vCritical() << "Empty data";
		return false;
	}

	QBuffer logonDataBuffer( &logonData );
	if( logonDataBuffer.open( QBuffer::ReadOnly ) == false )
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	VariantStream stream( &logonDataBuffer );
	*username = stream.read().toString();
	*password = VeyonCore::cryptoCore().decryptPassword( stream.read().toString() );

	return username->isEmpty() == false && password->isEmpty() == false;
}



bool WindowsUserFunctions::writePersistentLogonCredentials( const QString& username, const Password& password )
{
	QBuffer logonDataBuffer;
	if( logonDataBuffer.open( QBuffer::WriteOnly ) == false )
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	VariantStream stream( &logonDataBuffer );
	stream.write( username );
	stream.write( VeyonCore::cryptoCore().encryptPassword( password ) );

	if( ServiceDataManager::write( WindowsServiceCore::serviceDataTokenFromEnvironment(),
								   logonDataBuffer.data() ) == false )
	{
		vCritical() << "Failed to write persistent service data";
		return false;
	}

	return true;
}



bool WindowsUserFunctions::clearPersistentLogonCredentials()
{
	return ServiceDataManager::write( WindowsServiceCore::serviceDataTokenFromEnvironment(), {} );
}



bool WindowsUserFunctions::performFakeInputLogon( const QString& username, const Password& password )
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

	ctrlAltDel();

	QThread::msleep( static_cast<unsigned long>( config.logonInputStartDelay() ) );

	input.pressAndReleaseKey( XK_Delete );

	sendString( username );

	input.pressAndReleaseKey( XK_Tab );

	sendString( QString::fromUtf8( password.toByteArray() ) );

	input.pressAndReleaseKey( XK_Return );

	return true;
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
