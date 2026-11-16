// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_PIPEWIREVNC_CONFIG_PROPERTY(OP) \
	OP(PipeWireVncConfiguration, m_configuration, bool, persistRestoreToken, setPersistRestoreToken, "PersistRestoreToken", "PipeWireVnc", false, Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(PipeWireVncConfiguration, FOREACH_PIPEWIREVNC_CONFIG_PROPERTY)
