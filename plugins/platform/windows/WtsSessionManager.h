/*
 * WtsSessionManager.h - header file for WtsSessionManager class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <windows.h>

class WtsSessionManager
{
public:
	typedef DWORD SessionId;
	typedef QList<DWORD> SessionList;

	enum SessionInfo {
		SessionInfoNone,
		SessionInfoUserName,
		SessionInfoWinStationName,
		SessionInfoDomainName,
		SessionInfoCount
	};

	enum {
		SessionUpdateInterval = 1000
	};

	static const SessionId InvalidSession = 0xFFFFFFFF;

	static SessionId activeConsoleSession();

	static SessionList activeSessions();

	static QString querySessionInformation( SessionId sessionId, SessionInfo sessionInfo );

	static DWORD findWinlogonProcessId( SessionId sessionId );
	static DWORD findProcessId( const QString& userName );

	static QStringList loggedOnUsers();

} ;
