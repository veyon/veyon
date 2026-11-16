// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_EXTERNAL_VNC_SERVER_CONFIG_PROPERTY(OP) \
	OP(ExternalVncServerConfiguration, m_configuration, int, serverPort, setServerPort, "ServerPort", "ExternalVncServer", 5900, Configuration::Property::Flag::Standard) \
	OP(ExternalVncServerConfiguration, m_configuration, Configuration::Password, password, setPassword, "Password", "ExternalVncServer", QString(), Configuration::Property::Flag::Standard)

DECLARE_CONFIG_PROXY(ExternalVncServerConfiguration, FOREACH_EXTERNAL_VNC_SERVER_CONFIG_PROPERTY)
