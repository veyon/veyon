// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "DemoServer.h"
#include "VeyonConfiguration.h"

#define FOREACH_DEMO_CONFIG_PROPERTY(OP) \
	OP(DemoConfiguration, m_configuration, bool, slowDownThumbnailUpdates, setSlowDownThumbnailUpdates, "SlowDownThumbnailUpdates", "Demo", true, Configuration::Property::Flag::Advanced) \
	OP(DemoConfiguration, m_configuration, int, bandwidthLimit, setBandwidthLimit, "BandwidthLimit", "Demo", DemoServer::DefaultBandwidthLimit, Configuration::Property::Flag::Advanced) \
	OP(DemoConfiguration, m_configuration, int, framebufferUpdateInterval, setFramebufferUpdateInterval, "FramebufferUpdateInterval", "Demo", 50, Configuration::Property::Flag::Advanced) \
	OP(DemoConfiguration, m_configuration, int, keyFrameInterval, setKeyFrameInterval, "KeyFrameInterval", "Demo", 10, Configuration::Property::Flag::Advanced) \
	OP(DemoConfiguration, m_configuration, int, memoryLimit, setMemoryLimit, "MemoryLimit", "Demo", 128, Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(DemoConfiguration, FOREACH_DEMO_CONFIG_PROPERTY)
