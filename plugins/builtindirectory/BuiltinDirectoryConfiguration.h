/*
 * BuiltinDirectoryConfiguration.h - configuration values for BuiltinDirectory plugin
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonArray>

#include "Configuration/Proxy.h"

#define FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY(OP) \
	OP( BuiltinDirectoryConfiguration, m_configuration, QJsonArray, networkObjects, setNetworkObjects, "NetworkObjects", "BuiltinDirectory", QJsonArray(), Configuration::Property::Flag::Standard )	\
	/* legacy properties required for upgrade */ \
	OP( BuiltinDirectoryConfiguration, m_configuration, QJsonArray, legacyLocalDataNetworkObjects, setLegacyLocalDataNetworkObjects, "NetworkObjects", "LocalData", QJsonArray(), Configuration::Property::Flag::Legacy )	\

DECLARE_CONFIG_PROXY(BuiltinDirectoryConfiguration, FOREACH_BUILTIN_DIRECTORY_CONFIG_PROPERTY)
