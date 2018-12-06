/*
 * UserConfig.h - UserConfig class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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

#include "Configuration/Object.h"
#include "ComputerMonitoringView.h"
#include "VeyonMaster.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class UserConfig : public Configuration::Object
{
	Q_OBJECT
public:
	UserConfig( Configuration::Store::Backend backend );

#define FOREACH_PERSONAL_CONFIG_PROPERTY(OP)						\
	OP( UserConfig, VeyonMaster::userConfig(), JSONARRAY, checkedNetworkObjects, setCheckedNetworkObjects, "CheckedNetworkObjects", "UI", QJsonArray() )	\
	OP( UserConfig, VeyonMaster::userConfig(), JSONARRAY, computerPositions, setComputerPositions, "ComputerPositions", "UI", QJsonArray() )	\
	OP( UserConfig, VeyonMaster::userConfig(), BOOL, useCustomComputerPositions, setUseCustomComputerPositions, "UseCustomComputerPositions", "UI", false )	\
	OP( UserConfig, VeyonMaster::userConfig(), BOOL, filterPoweredOnComputers, setFilterPoweredOnComputers, "FilterPoweredOnComputers", "UI", false )	\
	OP( UserConfig, VeyonMaster::userConfig(), INT, monitoringScreenSize, setMonitoringScreenSize, "MonitoringScreenSize", "UI", ComputerMonitoringView::DefaultComputerScreenSize )	\
	OP( UserConfig, VeyonMaster::userConfig(), INT, defaultRole, setDefaultRole, "DefaultRole", "Authentication", 0 )	\
	OP( UserConfig, VeyonMaster::userConfig(), BOOL, toolButtonIconOnlyMode, setToolButtonIconOnlyMode, "ToolButtonIconOnlyMode", "UI", false )	\
	OP( UserConfig, VeyonMaster::userConfig(), BOOL, noToolTips, setNoToolTips, "NoToolTips", "UI", false )	\
	OP( UserConfig, VeyonMaster::userConfig(), STRING, windowState, setWindowState, "WindowState", "UI", QString() )	\
	OP( UserConfig, VeyonMaster::userConfig(), STRING, windowGeometry, setWindowGeometry, "WindowGeometry", "UI", QString() )	\

	FOREACH_PERSONAL_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

} ;
