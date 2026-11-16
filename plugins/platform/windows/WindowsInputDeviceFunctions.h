// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include <interception.h>

#include "PlatformInputDeviceFunctions.h"

// clazy:excludeall=copyable-polymorphic

class WindowsInputDeviceFunctions : public PlatformInputDeviceFunctions
{
public:
	WindowsInputDeviceFunctions() = default;
	virtual ~WindowsInputDeviceFunctions();

	void enableInputDevices() override;
	void disableInputDevices() override;

	KeyboardShortcutTrapper* createKeyboardShortcutTrapper(QObject* parent) override;

	static void checkInterceptionInstallation();

	static void stopOnScreenKeyboard();

private:
	const QUuid CustomPowerSchemeId{QStringLiteral("2699bf3e-30c6-4c74-914a-47d4504203f6")};
	const QString CustomPowerSchemeIdString = CustomPowerSchemeId.toString(QUuid::WithoutBraces);

	static constexpr auto OnScreenKeyboardTerminateTimeout = 5000;

	void enableInterception();
	void disableInterception();
	void initHIDServiceStatus();
	void stopHIDService();
	void restoreHIDService();
	void setCustomPowerScheme();
	void restorePowerScheme();

	static bool installInterception();
	static bool uninstallInterception();
	static int interceptionInstaller(const QString& argument);

	bool m_inputDevicesDisabled{false};
	InterceptionContext m_interceptionContext{nullptr};
	HHOOK m_keyboardHook{nullptr};
	QString m_hidServiceName{QStringLiteral("hidserv")};
	bool m_hidServiceStatusInitialized{false};
	bool m_hidServiceActivated{false};
	QString m_originalPowerSchemeId{};
};
