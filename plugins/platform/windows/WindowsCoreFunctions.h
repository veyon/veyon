/*
 * WindowsCoreFunctions.h - declaration of WindowsCoreFunctions class
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

#include <windows.h>

#include "PlatformCoreFunctions.h"

// clazy:excludeall=copyable-polymorphic

class CXEventLog;

class WindowsCoreFunctions : public PlatformCoreFunctions
{
public:
	using ProcessId = DWORD;

	WindowsCoreFunctions() = default;
	~WindowsCoreFunctions() override;

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

	bool runProgramAsUser( const QString& program,
						   const QStringList& parameters,
						   const QString& username,
						   const QString& desktop ) override;

	QString genericUrlHandler() const override;

	static bool enablePrivilege( LPCWSTR privilegeName, bool enable );

	static QSharedPointer<wchar_t> toWCharArray( const QString& qstring );
	static const wchar_t* toConstWCharArray( const QString& qstring );

	static HANDLE runProgramInSession( const QString& program,
									   const QStringList& parameters,
									   const QStringList& extraEnvironment,
									   DWORD baseProcessId,
									   const QString& desktop );

	static bool terminateProcess( ProcessId processId, DWORD timeout = DefaultProcessTerminationTimeout );

private:
	static constexpr int ConsoleOutputBufferSize = 256;
	static constexpr DWORD DefaultProcessTerminationTimeout = 5000;
	static constexpr size_t ScreenSaverSettingsCount = 3;

	static constexpr auto ShutdownFlags = SHUTDOWN_FORCE_OTHERS | SHUTDOWN_FORCE_SELF;
	static constexpr auto ShutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_FLAG_PLANNED;

	const std::array<UINT, ScreenSaverSettingsCount> ScreenSaverSettingsGetList = {
		SPI_GETLOWPOWERTIMEOUT,
		SPI_GETPOWEROFFTIMEOUT,
		SPI_GETSCREENSAVETIMEOUT
	};

	const std::array<UINT, ScreenSaverSettingsCount> ScreenSaverSettingsSetList = {
		SPI_SETLOWPOWERTIMEOUT,
		SPI_SETPOWEROFFTIMEOUT,
		SPI_SETSCREENSAVETIMEOUT
	};

	static wchar_t* appendToEnvironmentBlock( const wchar_t* env, const QStringList& strings );
	static void setTaskbarState( bool enabled );
	static void setStartMenuState( bool enabled );
	static void setDesktopState( bool enabled );

	CXEventLog* m_eventLog{nullptr};
	std::array<UINT, ScreenSaverSettingsCount> m_screenSaverSettings{};

};
