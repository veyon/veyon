/*
 * WindowsUserFunctions.cpp - implementation of WindowsUserFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "WindowsUserFunctions.h"

#include <wtsapi32.h>
#include <lm.h>

static QString querySessionInformation( DWORD sessionId, WTS_INFO_CLASS infoClass )
{
	QString result;
	LPTSTR pBuffer = NULL;
	DWORD dwBufferLen;
	if( WTSQuerySessionInformation(
					WTS_CURRENT_SERVER_HANDLE,
					sessionId,
					infoClass,
					&pBuffer,
					&dwBufferLen ) )
	{
		result = QString::fromWCharArray( pBuffer );
	}
	WTSFreeMemory( pBuffer );

	return result;
}



QString WindowsUserFunctions::fullName( const QString& username )
{
	QString fullName;

	QString realUsername = username;
	PBYTE domainController = nullptr;

	const auto nameParts = username.split( '\\' );
	if( nameParts.size() > 1 )
	{
		realUsername = nameParts[1];
		if( NetGetDCName( nullptr, (LPWSTR) nameParts[0].utf16(), &domainController ) != NERR_Success )
		{
			domainController = nullptr;
		}
	}

	LPUSER_INFO_2 buf = nullptr;
	NET_API_STATUS nStatus = NetUserGetInfo( (LPWSTR) domainController, (LPWSTR) realUsername.utf16(), 2, (LPBYTE *) &buf );
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
	groupList.removeAll( QStringLiteral("") );

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
	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QString WindowsUserFunctions::currentUser()
{
	auto sessionId = WTSGetActiveConsoleSessionId();

	auto username = querySessionInformation( sessionId, WTSUserName );
	auto domainName = querySessionInformation( sessionId, WTSDomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		wchar_t computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( domainName == QString::fromWCharArray( computerName ) )
		{
			// reset domain name as we do not need to store local computer name
			domainName = QString();
		}
	}

	if( domainName.isEmpty() )
	{
		return username;
	}

	return domainName + '\\' + username;
}



QStringList WindowsUserFunctions::loggedOnUsers()
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



void WindowsUserFunctions::logon( const QString& username, const QString& password )
{
	Q_UNUSED(username);
	Q_UNUSED(password);

	// TODO
}



void WindowsUserFunctions::logout()
{
	ExitWindowsEx( EWX_LOGOFF | (EWX_FORCE | EWX_FORCEIFHUNG), SHTDN_REASON_MAJOR_OTHER );
}



QString WindowsUserFunctions::domainController()
{
	QString dcName;
	LPBYTE outBuffer = nullptr;

	if( NetGetDCName( nullptr, nullptr, &outBuffer ) == NERR_Success )
	{
		dcName = QString::fromUtf16( (const ushort *) outBuffer );

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not query domain controller name!";
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

	if( NetGroupEnum( (LPCWSTR) dc.utf16(), 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		const GROUP_INFO_0* groupInfos = (GROUP_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			    groupList += QString::fromUtf16( (const ushort *) groupInfos[i].grpi0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all domain groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch domain groups";
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

	if( NetUserGetGroups( (LPCWSTR) dc.utf16(), (LPCWSTR) username.utf16(), 0, &outBuffer, MAX_PREFERRED_LENGTH,
	                      &entriesRead, &totalEntries ) == NERR_Success )
	{
		const GROUP_USERS_INFO_0* groupUsersInfo = (GROUP_USERS_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			    groupList += QString::fromUtf16( (const ushort *) groupUsersInfo[i].grui0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all domain groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch domain groups for user" << username;
	}

	return groupList;
}



QStringList WindowsUserFunctions::localUserGroups()
{
	QStringList groupList;

	LPBYTE outBuffer = NULL;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetLocalGroupEnum( nullptr, 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		LOCALGROUP_INFO_0* groupInfos = (LOCALGROUP_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			    groupList += QString::fromUtf16( (const ushort *) groupInfos[i].lgrpi0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all local groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch local groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::localGroupsOfUser( const QString& username )
{
	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetUserGetLocalGroups( nullptr, (LPCWSTR) username.utf16(), 0, 0, &outBuffer, MAX_PREFERRED_LENGTH,
	                           &entriesRead, &totalEntries ) == NERR_Success )
	{
		const LOCALGROUP_USERS_INFO_0* localGroupUsersInfo = (LOCALGROUP_USERS_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			    groupList += QString::fromUtf16( (const ushort *) localGroupUsersInfo[i].lgrui0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all local groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch local groups for user" << username;
	}

	return groupList;
}
