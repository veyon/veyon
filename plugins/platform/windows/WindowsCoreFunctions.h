// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include "PlatformCoreFunctions.h"
#include "WindowsSmartObjects.h"

// clazy:excludeall=copyable-polymorphic

class CXEventLog;

class WindowsCoreFunctions : public PlatformCoreFunctions
{
public:
	struct alignas(SID) SecurityIdentifierBuffer : public std::array<std::byte, SECURITY_MAX_SID_SIZE>
	{
	};

	WindowsCoreFunctions() = default;
	~WindowsCoreFunctions() override;

	bool applyConfiguration() override;

	bool prepareSessionBusAccess() override { return false; }

	void initNativeLoggingSystem(const QString& appName) override;
	void writeToNativeLoggingSystem(const QString& message, Logger::LogLevel loglevel) override;


	void reboot() override;
	void powerDown(bool installUpdates) override;

	void raiseWindow(QWidget* widget, bool stayOnTop) override;

	void disableScreenSaver() override;
	void restoreScreenSaverSettings() override;

	void setSystemUiState(bool enabled) override;

	QString activeDesktopName() override;

	bool isRunningAsAdmin() const override;
	bool runProgramAsAdmin(const QString& program, const QStringList& parameters) override;

	bool runProgramAsUser(const QString& program,
						  const QStringList& parameters,
						  const QString& username,
						  const QString& desktop,
						  const QByteArray& stdInData) override;

	QString genericUrlHandler() const override;

	QString queryDisplayDeviceName(const QScreen& screen) const override;

	QString getApplicationName(ProcessId processId) const override;

	static bool enablePrivilege(LPCWSTR privilegeName, bool enable);

	static SmartWCharPtr toWCharArray(const QString& qstring);
	static const wchar_t* toConstWCharArray(const QString& qstring);

	static QString securityIdentifierToString(const SecurityIdentifierBuffer& sidBuffer);
	static bool stringToSecurityIdentifier(const QString& sidString, WindowsCoreFunctions::SecurityIdentifierBuffer& sidBuffer);

	static SmartHandle runProgramInSession(const QString& program,
										   const QStringList& parameters,
										   const QStringList& extraEnvironment,
										   DWORD baseProcessId,
										   const QString& desktop,
										   const QByteArray& stdInData);

	static QStringList queryProcessEnvironmentVariables(DWORD processId);

	static bool terminateProcess(ProcessId processId, DWORD timeout = DefaultProcessTerminationTimeout);

private:
	static constexpr int ConsoleOutputBufferSize = 256;
	static constexpr DWORD DefaultProcessTerminationTimeout = 5000;
	static constexpr size_t ScreenSaverSettingsCount = 3;

	static constexpr auto MaximumEnvironmentBlockSize = 1024 * 1024;

	static constexpr auto ShutdownFlags = SHUTDOWN_FORCE_OTHERS | SHUTDOWN_FORCE_SELF;
	static constexpr auto ShutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_FLAG_PLANNED;

	const std::array<UINT, ScreenSaverSettingsCount> ScreenSaverSettingsGetList = {
		SPI_GETLOWPOWERTIMEOUT,
		SPI_GETPOWEROFFTIMEOUT,
		SPI_GETSCREENSAVETIMEOUT};

	const std::array<UINT, ScreenSaverSettingsCount> ScreenSaverSettingsSetList = {
		SPI_SETLOWPOWERTIMEOUT,
		SPI_SETPOWEROFFTIMEOUT,
		SPI_SETSCREENSAVETIMEOUT};

	static wchar_t* appendToEnvironmentBlock(const wchar_t* env, const QStringList& strings);
	static void setTaskbarState(bool enabled);
	static void setStartMenuState(bool enabled);
	static void setDesktopState(bool enabled);

	CXEventLog* m_eventLog{nullptr};
	std::array<UINT, ScreenSaverSettingsCount> m_screenSaverSettings{};
};
