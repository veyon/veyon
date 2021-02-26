/*
 * UserConfig.h - UserConfig class
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
#include <QJsonObject>

#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include "ComputerMonitoringView.h"
#include "SlideshowPanel.h"
#include "VeyonMaster.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class UserConfig : public Configuration::Object
{
	Q_OBJECT
public:
	explicit UserConfig( Configuration::Store::Backend backend );

#define FOREACH_PERSONAL_CONFIG_PROPERTY(OP)						\
	OP( UserConfig, VeyonMaster::userConfig(), QJsonArray, checkedNetworkObjects, setCheckedNetworkObjects, "CheckedNetworkObjects", "UI", QJsonArray(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QJsonArray, computerPositions, setComputerPositions, "ComputerPositions", "UI", QJsonArray(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, useCustomComputerPositions, setUseCustomComputerPositions, "UseCustomComputerPositions", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, filterPoweredOnComputers, setFilterPoweredOnComputers, "FilterPoweredOnComputers", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), int, monitoringScreenSize, setMonitoringScreenSize, "MonitoringScreenSize", "UI", ComputerMonitoringView::DefaultComputerScreenSize, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, autoAdjustMonitoringIconSize, setAutoAdjustMonitoringIconSize, "AutoAdjustMonitoringIconSize", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), int, slideshowDuration, setSlideshowDuration, "SlideshowDuration", "UI", SlideshowPanel::DefaultDuration, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, spotlightRealtime, setSpotlightRealtime, "SpotlightRealtime", "UI", true, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), int, defaultRole, setDefaultRole, "DefaultRole", "Authentication", 0, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, toolButtonIconOnlyMode, setToolButtonIconOnlyMode, "ToolButtonIconOnlyMode", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, noToolTips, setNoToolTips, "NoToolTips", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QJsonObject, splitterStates, setSplitterStates, "SplitterStates", "UI", {}, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QString, windowState, setWindowState, "WindowState", "UI", QString(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QString, windowGeometry, setWindowGeometry, "WindowGeometry", "UI", QString(), Configuration::Property::Flag::Standard )	\

	FOREACH_PERSONAL_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)

} ;
