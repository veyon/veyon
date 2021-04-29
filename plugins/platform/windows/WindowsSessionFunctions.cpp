/*
 * WindowsSessionFunctions.cpp - implementation of WindowsSessionFunctions class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "PlatformSessionManager.h"
#include "WindowsSessionFunctions.h"
#include "WtsSessionManager.h"


WindowsSessionFunctions::SessionId WindowsSessionFunctions::currentSessionId()
{
	const auto currentSession = WtsSessionManager::currentSession();

	if( currentSession == WtsSessionManager::activeConsoleSession() )
	{
		return DefaultSessionId;
	}

	return PlatformSessionManager::resolveSessionId( QString::number(currentSession) );
}



QString WindowsSessionFunctions::currentSessionType() const
{
	if(WtsSessionManager::currentSession() == WtsSessionManager::activeConsoleSession() )
	{
		return QStringLiteral("console");
	}

	return QStringLiteral("rdp");
}



bool WindowsSessionFunctions::currentSessionHasUser() const
{
	return WtsSessionManager::querySessionInformation( WtsSessionManager::currentSession(),
													   WtsSessionManager::SessionInfo::UserName ).isEmpty() == false;
}
