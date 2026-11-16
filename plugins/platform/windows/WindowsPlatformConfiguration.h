// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "WindowsSessionFunctions.h"

#define FOREACH_WINDOWS_PLATFORM_CONFIG_PROPERTY(OP) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, isSoftwareSASEnabled, setSoftwareSASEnabled, "SoftwareSASEnabled", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, WindowsSessionFunctions::InterferingWindowHandling, interferingWindowsHandling, setInterferingWindowsHandling, "InterferingWindowsHandling", "Windows", QVariant::fromValue(WindowsSessionFunctions::InterferingWindowHandling::None), Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, disableSSPIBasedUserAuthentication, setDisableSSPIBasedUserAuthentication, "DisableSSPIBasedUserAuthentication", "Windows", false, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, hideDesktopForScreenLock, setHideDesktopForScreenLock, "HideDesktopForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, hideTaskbarForScreenLock, setHideTaskbarForScreenLock, "HideTaskbarForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, hideStartMenuForScreenLock, setHideStartMenuForScreenLock, "HideStartMenuForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, useCustomPowerSchemeForScreenLock, setUseCustomPowerSchemeForScreenLock, "UseCustomPowerSchemeForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, useInterceptionDriver, setUseInterceptionDriver, "UseInterceptionDriver", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, disableTouchDevicesForScreenLock, setDisableTouchDevicesForScreenLock, "DisableTouchDevicesForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, disableKeyboardDevicesForScreenLock, setDisableKeyboardDevicesForScreenLock, "DisableKeyboardDevicesForScreenLock", "Windows", false, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, disableMouseDevicesForScreenLock, setDisableMouseDevicesForScreenLock, "DisableMouseDevicesForScreenLock", "Windows", false, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, int, logonInputStartDelay, setLogonInputStartDelay, "LogonInputStartDelay", "Windows", 1000, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, int, logonKeyPressInterval, setLogonKeyPressInterval, "LogonKeyPressInterval", "Windows", 10, Configuration::Property::Flag::Advanced) \
	OP(WindowsPlatformConfiguration, m_configuration, bool, logonConfirmLegalNotice, setLogonConfirmLegalNotice, "LogonConfirmLegalNotice", "Windows", false, Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(WindowsPlatformConfiguration, FOREACH_WINDOWS_PLATFORM_CONFIG_PROPERTY)
