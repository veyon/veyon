// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_LINUX_PLATFORM_CONFIG_PROPERTY(OP) \
	OP(LinuxPlatformConfiguration, m_configuration, QString, pamServiceName, setPamServiceName, "PamServiceName", "Linux", QString(), Configuration::Property::Flag::Advanced) \
	OP(LinuxPlatformConfiguration, m_configuration, int, minimumUserSessionLifetime, setMinimumUserSessionLifetime, "MinimumUserSessionLifetime", "Linux", 3, Configuration::Property::Flag::Advanced) \
	OP(LinuxPlatformConfiguration, m_configuration, QString, userLoginKeySequence, setUserLoginKeySequence, "UserLoginKeySequence", "Linux", QStringLiteral("%username%<Tab>%password%<Return>"), Configuration::Property::Flag::Advanced) \
	OP(LinuxPlatformConfiguration, m_configuration, int, userLoginInputStartDelay, setLoginInputStartDelay, "UserLoginInputStartDelay", "Linux", 1000, Configuration::Property::Flag::Advanced) \
	OP(LinuxPlatformConfiguration, m_configuration, int, userLoginTextInputKeyPressInterval, setUserLoginTextInputKeyPressInterval, "UserLoginTextInputKeyPressInterval", "Linux", 5, Configuration::Property::Flag::Advanced) \
	OP(LinuxPlatformConfiguration, m_configuration, int, userLoginControlKeyPressInterval, setUserLoginControlKeyPressInterval, "UserLoginControlKeyPressInterval", "Linux", 100, Configuration::Property::Flag::Advanced)


DECLARE_CONFIG_PROXY(LinuxPlatformConfiguration, FOREACH_LINUX_PLATFORM_CONFIG_PROPERTY)
