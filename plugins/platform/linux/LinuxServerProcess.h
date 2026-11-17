// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QProcess>

#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"
#include "SecureBuffer.h"

class ServiceDataManager;

// clazy:excludeall=copyable-polymorphic

class LinuxServerProcess : public QProcess
{
	Q_OBJECT
public:
	explicit LinuxServerProcess(const QProcessEnvironment& processEnvironment,
								const QString& sessionPath, int sessionId,
								LinuxUserFunctions::UserId sessionUserId,
								LinuxSessionFunctions::Type sessionType,
								ServiceDataManager& dataManager,
								QObject* parent = nullptr);
	~LinuxServerProcess() override;

	void start();
	void stop();

	LinuxUserFunctions::UserId sessionUserId() const
	{
		return m_sessionUserId;
	}

	const QString& ipcEndpointName() const
	{
		return m_ipcEndpointName;
	}

private:
	void setProcessUserId();

	bool createIpcEndpoint(LinuxUserFunctions::UserId serverUserId);
	void removeIpcEndpoint();
	void sendIpcCredentials();

	bool hasDesktopPortalRegistry();
	static void warnAboutPtraceScope();
	void startInSystemdScope();

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
	ServiceDataManager& m_dataManager;

	QString m_ipcEndpointName;
	Veyon::Crypto::SecureBuffer m_ipcCredentialsLine;
};
