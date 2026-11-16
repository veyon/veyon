// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_X11VNC_CONFIG_PROPERTY(OP) \
	OP(X11VncConfiguration, m_configuration, bool, isXDamageDisabled, setXDamageDisabled, "XDamageDisabled", "X11Vnc", false, Configuration::Property::Flag::Advanced) \
	OP(X11VncConfiguration, m_configuration, QString, extraArguments, setExtraArguments, "ExtraArguments", "X11Vnc", QString(), Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(X11VncConfiguration, FOREACH_X11VNC_CONFIG_PROPERTY)
