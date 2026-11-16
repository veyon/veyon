// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_ULTRAVNC_CONFIG_PROPERTY(OP) \
	OP(UltraVncConfiguration, m_configuration, bool, ultraVncCaptureLayeredWindows, setUltraVncCaptureLayeredWindows, "CaptureLayeredWindows", "UltraVNC", true, Configuration::Property::Flag::Advanced) \
	OP(UltraVncConfiguration, m_configuration, bool, ultraVncMultiMonitorSupportEnabled, setUltraVncMultiMonitorSupportEnabled, "MultiMonitorSupport", "UltraVNC", true, Configuration::Property::Flag::Standard) \
	OP(UltraVncConfiguration, m_configuration, bool, ultraVncPollFullScreen, setUltraVncPollFullScreen, "PollFullScreen", "UltraVNC", true, Configuration::Property::Flag::Advanced) \
	OP(UltraVncConfiguration, m_configuration, bool, ultraVncLowAccuracy, setUltraVncLowAccuracy, "LowAccuracy", "UltraVNC", true, Configuration::Property::Flag::Advanced) \
	OP(UltraVncConfiguration, m_configuration, bool, ultraVncDeskDupEngineEnabled, setUltraVncDeskDupEngineEnabled, "DeskDupEngine", "UltraVNC", true, Configuration::Property::Flag::Advanced) \
	OP(UltraVncConfiguration, m_configuration, int, ultraVncMaxCpu, setUltraVncMaxCpu, "MaxCPU", "UltraVNC", 100, Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(UltraVncConfiguration, FOREACH_ULTRAVNC_CONFIG_PROPERTY)
