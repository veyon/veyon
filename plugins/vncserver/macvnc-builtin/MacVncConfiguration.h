#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_MAC_VNC_CONFIG_PROPERTY(OP) \
	OP( MacVncConfiguration, m_configuration, bool, viewOnly, setViewOnly, "ViewOnly", "MacVncServer", false, Configuration::Property::Flag::Standard ) \
	OP( MacVncConfiguration, m_configuration, bool, preventDimming, setPreventDimming, "PreventDimming", "MacVncServer", false, Configuration::Property::Flag::Advanced ) \
	OP( MacVncConfiguration, m_configuration, bool, preventSleep, setPreventSleep, "PreventSleep", "MacVncServer", true, Configuration::Property::Flag::Advanced ) \
	OP( MacVncConfiguration, m_configuration, int, displayIndex, setDisplayIndex, "DisplayIndex", "MacVncServer", -1, Configuration::Property::Flag::Advanced )

DECLARE_CONFIG_PROXY(MacVncConfiguration, FOREACH_MAC_VNC_CONFIG_PROPERTY)
