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

#include <ntsecapi.h>
#include <ntstatus.h>

QStringList WindowsUserSessionFunctions::loggedOnUsers()
{
	QStringList users;

	ULONG sessionCount = 0;
	PLUID sessionList = nullptr;

	if( LsaEnumerateLogonSessions( &sessionCount, &sessionList ) != STATUS_SUCCESS )
	{
		return users;
	}

	for( ULONG i = 0; i < sessionCount; ++i )
	{
		PSECURITY_LOGON_SESSION_DATA sessionData = nullptr;

		if( LsaGetLogonSessionData( sessionList+i, &sessionData ) == STATUS_SUCCESS )
		{
			const auto user = QString::fromWCharArray( sessionData->UserName.Buffer, sessionData->UserName.Length );
			if( users.contains( user ) == false )
			{
				users += user;
			}
			LsaFreeReturnBuffer( sessionData );
		}
	}

	LsaFreeReturnBuffer( sessionList );

	return users;
}
