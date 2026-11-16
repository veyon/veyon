// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonArray>

#include "Configuration/Proxy.h"

#define FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY(OP) \
	OP(BuiltinDirectoryConfiguration, m_configuration, QJsonArray, networkObjects, setNetworkObjects, "NetworkObjects", "BuiltinDirectory", QJsonArray(), Configuration::Property::Flag::Standard) \
	/* legacy properties required for upgrade */ \
	OP(BuiltinDirectoryConfiguration, m_configuration, QJsonArray, legacyLocalDataNetworkObjects, setLegacyLocalDataNetworkObjects, "NetworkObjects", "LocalData", QJsonArray(), Configuration::Property::Flag::Legacy)

DECLARE_CONFIG_PROXY(BuiltinDirectoryConfiguration, FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY)
