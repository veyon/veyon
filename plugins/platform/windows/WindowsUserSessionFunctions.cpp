/*
 * WindowsUserSessionFunctions.cpp - implementation of WindowsUserSessionFunctions class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "WindowsUserSessionFunctions.h"

#include <wtsapi32.h>

QStringList WindowsUserSessionFunctions::loggedOnUsers()
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
