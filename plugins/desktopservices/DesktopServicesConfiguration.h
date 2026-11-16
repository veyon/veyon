// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "VeyonConfiguration.h"

#define FOREACH_DESKTOP_SERVICES_CONFIG_PROPERTY(OP) \
	OP(DesktopServicesConfiguration, m_configuration, QJsonArray, legacyPredefinedPrograms, setLegacyPredefinedPrograms, "PredefinedPrograms", "DesktopServices", QJsonArray(), Configuration::Property::Flag::Legacy) \
	OP(DesktopServicesConfiguration, m_configuration, QJsonArray, predefinedApplications, setPredefinedApplications, "PredefinedApplications", "DesktopServices", QJsonArray(), Configuration::Property::Flag::Standard) \
	OP(DesktopServicesConfiguration, m_configuration, QJsonArray, predefinedWebsites, setPredefinedWebsites, "PredefinedWebsites", "DesktopServices", QJsonArray(), Configuration::Property::Flag::Standard)

DECLARE_CONFIG_PROXY(DesktopServicesConfiguration, FOREACH_DESKTOP_SERVICES_CONFIG_PROPERTY)
