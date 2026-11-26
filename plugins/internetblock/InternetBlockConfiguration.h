// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "VeyonConfiguration.h"

#define FOREACH_INTERNET_BLOCK_CONFIG_PROPERTY(OP) \
	OP(InternetBlockConfiguration, m_configuration, bool, allowPrivateNetworks, setAllowPrivateNetworks, "AllowPrivateNetworks", "InternetBlock", true, Configuration::Property::Flag::Standard) \
	OP(InternetBlockConfiguration, m_configuration, QString, allowedNetworks, setAllowedNetworks, "AllowedNetworks", "InternetBlock", QString(), Configuration::Property::Flag::Standard) \
	OP(InternetBlockConfiguration, m_configuration, bool, allowMasterComputer, setAllowMasterComputer, "AllowMasterComputer", "InternetBlock", true, Configuration::Property::Flag::Standard) \
	OP(InternetBlockConfiguration, m_configuration, bool, blockIPv6, setBlockIPv6, "BlockIPv6", "InternetBlock", true, Configuration::Property::Flag::Advanced) \
	OP(InternetBlockConfiguration, m_configuration, bool, unblockOnServiceStart, setUnblockOnServiceStart, "UnblockOnServiceStart", "InternetBlock", true, Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(InternetBlockConfiguration, FOREACH_INTERNET_BLOCK_CONFIG_PROPERTY)
