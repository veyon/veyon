/*
 * LinuxCoreFunctions.h - declaration of LinuxCoreFunctions class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QDBusInterface>
#include <QSharedPointer>

#include "PlatformCoreFunctions.h"

struct proc_t;

// clazy:excludeall=copyable-polymorphic

class LinuxCoreFunctions : public PlatformCoreFunctions
{
public:
	LinuxCoreFunctions() = default;

	bool applyConfiguration() override;

	void initNativeLoggingSystem( const QString& appName ) override;
	void writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel ) override;

	void reboot() override;
	void powerDown( bool installUpdates ) override;

	void raiseWindow( QWidget* widget, bool stayOnTop ) override;

	void disableScreenSaver() override;
	void restoreScreenSaverSettings() override;

	void setSystemUiState( bool enabled ) override;

	QString activeDesktopName() override;

	bool isRunningAsAdmin() const override;
	bool runProgramAsAdmin( const QString& program, const QStringList& parameters ) override;

	bool runProgramAsUser( const QString& program, const QStringList& parameters,
						   const QString& username,
						   const QString& desktop = {} ) override;

	QString genericUrlHandler() const override;

	using DBusInterfacePointer = QSharedPointer<QDBusInterface>;

	static DBusInterfacePointer kdeSessionManager();
	static DBusInterfacePointer gnomeSessionManager();
	static DBusInterfacePointer mateSessionManager();
	static DBusInterfacePointer xfcePowerManager();
	static DBusInterfacePointer systemdLoginManager();
	static DBusInterfacePointer consoleKitManager();

	static int systemctl( const QStringList& arguments );

	static void restartDisplayManagers();

	static void forEachChildProcess( const std::function<bool(proc_t *)>& visitor,
							 int parentPid, int flags, bool visitParent );

private:
	int m_screenSaverTimeout{0};
	int m_screenSaverPreferBlanking{0};
	bool m_dpmsEnabled{false};
	unsigned short m_dpmsStandbyTimeout{0};
	unsigned short m_dpmsSuspendTimeout{0};
	unsigned short m_dpmsOffTimeout{0};

};
