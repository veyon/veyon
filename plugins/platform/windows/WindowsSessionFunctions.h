// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "PlatformSessionFunctions.h"

// clazy:excludeall=copyable-polymorphic

class WindowsSessionFunctions : public PlatformSessionFunctions
{
	Q_GADGET
public:
	enum class InterferingWindowHandling {
		None,
		FixWindowAttributes,
		TerminateProcess,
		CloseSession
	};
	Q_ENUM(InterferingWindowHandling)

	WindowsSessionFunctions();

	SessionId currentSessionId() override;

	SessionUptime currentSessionUptime() const override;
	QString currentSessionClientAddress() const override;
	QString currentSessionClientName() const override;
	QString currentSessionHostName() const override;

	QString currentSessionType() const override;
	bool currentSessionHasUser() const override;

	EnvironmentVariables currentSessionEnvironmentVariables() const override;
	QVariant querySettingsValueInCurrentSession(const QString& key) const override;

private:
	void initInterferingWindowHandling();
	void inspectDesktopWindows();
	WINBOOL inspectDesktopWindow(HWND window);

	static constexpr auto DesktopWindowsInspectionInterval = 1000;

	InterferingWindowHandling m_interferingWindowsHandling = InterferingWindowHandling::None;
	QTimer m_desktopWindowsInspectionTimer;
};
