// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSet>

#include "LinuxCoreFunctions.h"
#include "PlatformSessionManager.h"
#include "ServiceDataManager.h"

#include <sys/types.h>

class LinuxServerProcess;
class LinuxServiceBusInterface;

// clazy:excludeall=copyable-polymorphic

class LinuxServiceCore : public QObject
{
	Q_OBJECT
public:
	explicit LinuxServiceCore(QObject* parent = nullptr);
	~LinuxServiceCore() override;

	void run();

private Q_SLOTS:
	void startServer(const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath);
	void stopServer(const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath);

private:
	static constexpr auto LoginManagerReconnectInterval = 3000;
	static constexpr auto SessionEnvironmentProbingInterval = 1000;
	static constexpr auto SessionStateProbingInterval = 1000;
	static constexpr auto ServerRestartInterval = 5000;
	static constexpr auto ServerWatchdogInterval = 10000;

	void connectToLoginManager();
	void startServers();
	void startServer(const QString& sessionPath);
	void deferServerStart(const QString& sessionPath, int delay);
	void stopServer(const QString& sessionPath);
	void stopAllServers();

	void checkSessionState(const QString& sessionPath);

	bool isActiveSessionUser(uid_t uid) const;

	LinuxCoreFunctions::DBusInterfacePointer m_loginManager{LinuxCoreFunctions::systemdLoginManager()};
	QMap<QString, LinuxServerProcess*> m_serverProcesses;
	QStringList m_deferredServerSessions;
	QSet<QString> m_sessionsWithUnknownState;
	QSet<QString> m_sessionsWithPendingStateRecheck;

	LinuxServiceBusInterface* m_busInterface = nullptr;

	ServiceDataManager m_dataManager{};
	PlatformSessionManager m_sessionManager{};
};
