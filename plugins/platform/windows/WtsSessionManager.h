// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
		SessionUptime,
		ClientAddress,
		ClientName
	};

	Q_ENUM(SessionInfo)

	static constexpr SessionId InvalidSession = static_cast<SessionId>(-1);
	static constexpr ProcessId InvalidProcess = static_cast<ProcessId>(-1);

	static SessionId currentSession();

	static SessionId activeConsoleSession();

	static SessionList activeSessions();

	static QString querySessionInformation(SessionId sessionId, SessionInfo sessionInfo);
	static QString queryUserSid(SessionId sessionId);

	static ProcessId findUserProcessId(const QString& userName);
	static ProcessId findProcessId(const QString& processName, SessionId sessionId = InvalidSession);
};
