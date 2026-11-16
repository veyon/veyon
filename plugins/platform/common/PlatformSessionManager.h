// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>
#include <QThread>

#include "PlatformSessionFunctions.h"

class QLocalSocket;

// clazy:excludeall=copyable-polymorphic

class PlatformSessionManager : public QThread
{
	Q_OBJECT
public:
	enum class Mode {
		Local,
		Active,
		Multi
	};
	Q_ENUM(Mode)

	using SessionId = PlatformSessionFunctions::SessionId;
	using PlatformSessionId = QString;

	PlatformSessionManager(QObject* parent = nullptr);
	~PlatformSessionManager() override;

	SessionId openSession(const PlatformSessionId& platformSessionId);
	void closeSession(const PlatformSessionId& platformSessionId);

	static SessionId resolveSessionId(const PlatformSessionId& platformSessionId);

	Mode mode() const
	{
		return m_mode;
	}

	int maximumSessionCount() const
	{
		return m_maximumSessionCount;
	}

	using SessionMap = QMap<PlatformSessionId, SessionId>;

protected:
	void run() override;

private:
	static constexpr auto ServerConnectTimeout = 5000;
	static constexpr auto SocketWaitTimeout = 1000;
	static constexpr auto MessageReadTimeout = 10000;

	static QString serverName()
	{
		return QStringLiteral("VeyonSessionManager");
	}

	static bool waitForMessage(QLocalSocket* socket);

	const Mode m_mode;
	const int m_maximumSessionCount;

	QMutex m_mutex;
	QVariantMap m_sessions;
};
