#pragma once

#include "VeyonConfiguration.h"
#include "Configuration/Proxy.h"

#define FOREACH_ACCESS_CONTROL_CONFIG_PROPERTY(OP) \
	OP(AccessControlConfiguration, m_configuration, QString, blockedApplications, setBlockedApplications, "BlockedApplications", "AccessControl", QStringLiteral(""), Configuration::Property::Flag::Standard) \
	OP(AccessControlConfiguration, m_configuration, bool, blockInternetEntirely, setBlockInternetEntirely, "BlockInternetEntirely", "AccessControl", false, Configuration::Property::Flag::Standard)

// clazy:excludeall=missing-qobject-macro

DECLARE_CONFIG_PROXY(AccessControlConfiguration, FOREACH_ACCESS_CONTROL_CONFIG_PROPERTY)
