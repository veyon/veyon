// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QSharedPointer>

#include "PlatformCoreFunctions.h"

#ifdef HAVE_LIBPROC2
#include <libproc2/pids.h>
#ifdef LIBPROC2_PIDS_VAL_OLD_API
#undef PIDS_VAL
#define PIDS_VAL(relative_enum, type, stack) stack->head[relative_enum].result.type
#endif
#else
struct proc_t;
#endif

// clazy:excludeall=copyable-polymorphic

class LinuxCoreFunctions : public PlatformCoreFunctions
{
public:
	LinuxCoreFunctions();

	bool applyConfiguration() override;

	bool prepareSessionBusAccess() override;

	void initNativeLoggingSystem(const QString& appName) override;
	void writeToNativeLoggingSystem(const QString& message, Logger::LogLevel loglevel) override;

	StandardInputReader* createStandardInputReader(QObject* parent) override;

	void reboot() override;
	void powerDown(bool installUpdates) override;

	void raiseWindow(QWidget* widget, bool stayOnTop) override;

	void disableScreenSaver() override;
	void restoreScreenSaverSettings() override;

	void setSystemUiState(bool enabled) override;

	QString activeDesktopName() override;

	bool isRunningAsAdmin() const override;
	bool runProgramAsAdmin(const QString& program, const QStringList& parameters) override;

	bool runProgramAsUser(const QString& program, const QStringList& parameters,
						  const QString& username,
						  const QString& desktop,
						  const QByteArray& stdInData) override;

	QString genericUrlHandler() const override;

	QString queryDisplayDeviceName(const QScreen& screen) const override;

	QString getApplicationName(ProcessId processId) const override;

	using DBusInterfacePointer = QSharedPointer<QDBusInterface>;

	static DBusInterfacePointer kdeSessionManager();
	static DBusInterfacePointer gnomeSessionManager();
	static DBusInterfacePointer mateSessionManager();
	static DBusInterfacePointer xfcePowerManager();
	static DBusInterfacePointer systemdLoginManager();
	static DBusInterfacePointer consoleKitManager();

	static bool isSystemdManaged();
	static int systemctl(const QStringList& arguments);

	static void restartDisplayManagers();

#ifdef HAVE_LIBPROC2
	static void forEachChildProcess(const std::function<bool(const pids_stack*)>& visitor,
									int parentPid, const std::vector<pids_item>& items, bool visitParent);
#else
	static void forEachChildProcess(const std::function<bool(proc_t*)>& visitor,
									int parentPid, int flags, bool visitParent);
#endif

	static bool waitForProcess(qint64 pid, int timeout, int sleepInterval);

private:
	int m_screenSaverTimeout{0};
	int m_screenSaverPreferBlanking{0};
	bool m_dpmsEnabled{false};
	unsigned short m_dpmsStandbyTimeout{0};
	unsigned short m_dpmsSuspendTimeout{0};
	unsigned short m_dpmsOffTimeout{0};

	// Wayland screen saver inhibition via DBus
	const bool m_isWaylandSession;
	static constexpr auto WaylandScreenSaverInhibitCookie = 0;
	unsigned m_waylandInhibitCookie{0};

	void disableScreenSaverWayland();
	void restoreScreenSaverSettingsWayland();
};
