/*
 * WtsSessionManager.h - header file for WtsSessionManager class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QObject>

class WtsSessionManager
{
	Q_GADGET
public:
	using SessionId = DWORD;
	using SessionList = QList<DWORD>;
	using ProcessId = DWORD;

	enum class SessionInfo {
		UserName,
		DomainName,
	};

	Q_ENUM(SessionInfo)

	static constexpr SessionId InvalidSession = static_cast<SessionId>( -1 );
	static constexpr ProcessId InvalidProcess = static_cast<ProcessId>( -1 );

	static SessionId currentSession();

	static SessionId activeConsoleSession();

	static SessionList activeSessions();

	static QString querySessionInformation( SessionId sessionId, SessionInfo sessionInfo );

	static ProcessId findWinlogonProcessId( SessionId sessionId );
	static ProcessId findUserProcessId( const QString& userName );
	static ProcessId findProcessId( const QString& processName );

} ;
