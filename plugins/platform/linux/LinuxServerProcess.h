// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QProcess>

#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxServerProcess : public QProcess
{
	Q_OBJECT
public:
	explicit LinuxServerProcess(const QProcessEnvironment& processEnvironment,
								const QString& sessionPath, int sessionId,
								LinuxUserFunctions::UserId sessionUserId,
								LinuxSessionFunctions::Type sessionType,
								QObject* parent = nullptr);
	~LinuxServerProcess() override;

	void start();
	void stop();

	LinuxUserFunctions::UserId sessionUserId() const
	{
		return m_sessionUserId;
	}

private:
	void setProcessUserId();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	void setupChildProcess() override
	{
		if (m_sessionType == LinuxSessionFunctions::Type::Wayland)
		{
			setProcessUserId();
		}
	}
#endif

	static constexpr auto ServerShutdownTimeout = 1000;
	static constexpr auto ServerTerminateTimeout = 3000;
	static constexpr auto ServerKillTimeout = 3000;
	static constexpr auto ServerWaitSleepInterval = 100;

	const QString m_sessionPath;
	int m_sessionId;
	const LinuxSessionFunctions::Type m_sessionType;
	LinuxUserFunctions::UserId m_sessionUserId{LinuxUserFunctions::InvalidUserId};
};
