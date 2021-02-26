/*
 * WindowsPlatformConfiguration.h - configuration values for WindowsPlatform plugin
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

#include "Configuration/Proxy.h"

#define FOREACH_WINDOWS_PLATFORM_CONFIG_PROPERTY(OP) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, isSoftwareSASEnabled, setSoftwareSASEnabled, "SoftwareSASEnabled", "Windows", true, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, disableSSPIBasedUserAuthentication, setDisableSSPIBasedUserAuthentication, "DisableSSPIBasedUserAuthentication", "Windows", false, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, hideDesktopForScreenLock, setHideDesktopForScreenLock, "HideDesktopForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, hideTaskbarForScreenLock, setHideTaskbarForScreenLock, "HideTaskbarForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, hideStartMenuForScreenLock, setHideStartMenuForScreenLock, "HideStartMenuForScreenLock", "Windows", true, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, useInterceptionDriver, setUseInterceptionDriver, "UseInterceptionDriver", "Windows", true, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, int, logonInputStartDelay, setLogonInputStartDelay, "LogonInputStartDelay", "Windows", 1000, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, int, logonKeyPressInterval, setLogonKeyPressInterval, "LogonKeyPressInterval", "Windows", 10, Configuration::Property::Flag::Advanced ) \
	OP( WindowsPlatformConfiguration, m_configuration, bool, logonConfirmLegalNotice, setLogonConfirmLegalNotice, "LogonConfirmLegalNotice", "Windows", false, Configuration::Property::Flag::Advanced ) \

DECLARE_CONFIG_PROXY(WindowsPlatformConfiguration, FOREACH_WINDOWS_PLATFORM_CONFIG_PROPERTY)
