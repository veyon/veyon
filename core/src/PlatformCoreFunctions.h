// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFile>
#include <QWindow>

#include "Logger.h"
#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class QScreen;

class StandardInputReader;

class PlatformCoreFunctions
{
public:
	using ProcessId = qint64;
	static constexpr ProcessId InvalidProcessId = -1;

	virtual ~PlatformCoreFunctions() = default;

	virtual bool applyConfiguration() = 0;

	virtual bool prepareSessionBusAccess() = 0;

	virtual void initNativeLoggingSystem(const QString& appName) = 0;
	virtual void writeToNativeLoggingSystem(const QString& message, Logger::LogLevel loglevel) = 0;

	virtual StandardInputReader* createStandardInputReader(QObject* parent) = 0;

	virtual void reboot() = 0;
	virtual void powerDown(bool installUpdates) = 0;

	virtual void raiseWindow(QWidget* widget, bool stayOnTop) = 0;

	virtual void raiseWindow(QWindow* window, bool stayOnTop)
	{
		if (!window) return;
		window->setFlag(Qt::WindowStaysOnTopHint, stayOnTop);
		window->show();
		window->raise();
		window->requestActivate();
	}

	virtual void disableScreenSaver() = 0;
	virtual void restoreScreenSaverSettings() = 0;

	virtual void setSystemUiState(bool enabled) = 0;

	virtual QString activeDesktopName() = 0;

	virtual bool isRunningAsAdmin() const = 0;
	virtual bool runProgramAsAdmin(const QString& program, const QStringList& parameters) = 0;

	virtual bool runProgramAsUser(const QString& program,
								  const QStringList& parameters,
								  const QString& username,
								  const QString& desktop,
								  const QByteArray& stdInData = {}) = 0;

	virtual QString genericUrlHandler() const = 0;

	virtual QString queryDisplayDeviceName(const QScreen& screen) const = 0;

	virtual QString getApplicationName(ProcessId processId) const = 0;
};
