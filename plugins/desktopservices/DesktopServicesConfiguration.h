/*
 * DesktopServicesConfiguration.h - configuration values for DesktopServices
 *
 * Copyright (c) 2018-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonConfiguration.h"
#include "Configuration/Proxy.h"

#define FOREACH_DESKTOP_SERVICES_CONFIG_PROPERTY(OP) \
	OP( DesktopServicesConfiguration, m_configuration, QJsonArray, legacyPredefinedPrograms, setLegacyPredefinedPrograms, "PredefinedPrograms", "DesktopServices", QJsonArray(), Configuration::Property::Flag::Legacy)	\
	OP( DesktopServicesConfiguration, m_configuration, QJsonArray, predefinedApplications, setPredefinedApplications, "PredefinedApplications", "DesktopServices", QJsonArray(), Configuration::Property::Flag::Standard )	\
	OP( DesktopServicesConfiguration, m_configuration, QJsonArray, predefinedWebsites, setPredefinedWebsites, "PredefinedWebsites", "DesktopServices", QJsonArray(), Configuration::Property::Flag::Standard )	\

// clazy:excludeall=missing-qobject-macro

DECLARE_CONFIG_PROXY(DesktopServicesConfiguration, FOREACH_DESKTOP_SERVICES_CONFIG_PROPERTY)
