/*
 * UltraVncConfiguration.h - UltraVNC-specific configuration values
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#define FOREACH_ULTRAVNC_CONFIG_PROPERTY(OP) \
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncCaptureLayeredWindows, setUltraVncCaptureLayeredWindows, "CaptureLayeredWindows", "UltraVNC", true, Configuration::Object::AdvancedProperty )	\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncMultiMonitorSupportEnabled, setUltraVncMultiMonitorSupportEnabled, "MultiMonitorSupport", "UltraVNC", true, Configuration::Object::StandardProperty )	\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncPollFullScreen, setUltraVncPollFullScreen, "PollFullScreen", "UltraVNC", true, Configuration::Object::AdvancedProperty )			\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncLowAccuracy, setUltraVncLowAccuracy, "LowAccuracy", "UltraVNC", true, Configuration::Object::AdvancedProperty )					\
	OP( UltraVncConfiguration, m_configuration, BOOL, ultraVncDeskDupEngineEnabled, setUltraVncDeskDupEngineEnabled, "DeskDupEngine", "UltraVNC", true, Configuration::Object::AdvancedProperty )	\

DECLARE_CONFIG_PROXY(UltraVncConfiguration, FOREACH_ULTRAVNC_CONFIG_PROPERTY)

