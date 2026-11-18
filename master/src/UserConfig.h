// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>

#include "Configuration/Object.h"
#include "Configuration/Property.h"


#include "VeyonMaster.h"

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class UserConfig : public Configuration::Object
{
	Q_OBJECT
public:
	UserConfig(const QString& storeName);
	explicit UserConfig(Configuration::Store* store);

#define FOREACH_PERSONAL_CONFIG_PROPERTY(OP) \
	OP( UserConfig, VeyonMaster::userConfig(), QJsonArray, checkedNetworkObjects, setCheckedNetworkObjects, "CheckedNetworkObjects", "UI", QJsonArray(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QJsonArray, computerPositions, setComputerPositions, "ComputerPositions", "UI", QJsonArray(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QString, computerPositionsFormat, setComputerPositionsFormat, "ComputerPositionsFormat", "UI", QString(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, useCustomComputerPositions, setUseCustomComputerPositions, "UseCustomComputerPositions", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, filterPoweredOnComputers, setFilterPoweredOnComputers, "FilterPoweredOnComputers", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, filterComputersWithLoggedOnUsers, setFilterComputersWithLoggedOnUsers, "FilterComputersWithLoggedOnUsers", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), int, monitoringScreenSize, setMonitoringScreenSize, "MonitoringScreenSize", "UI", 190, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, autoAdjustMonitoringIconSize, setAutoAdjustMonitoringIconSize, "AutoAdjustMonitoringIconSize", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), int, slideshowDuration, setSlideshowDuration, "SlideshowDuration", "UI", 3000, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, spotlightRealtime, setSpotlightRealtime, "SpotlightRealtime", "UI", true, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), int, defaultRole, setDefaultRole, "DefaultRole", "Authentication", 0, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, toolButtonIconOnlyMode, setToolButtonIconOnlyMode, "ToolButtonIconOnlyMode", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), bool, noToolTips, setNoToolTips, "NoToolTips", "UI", false, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QJsonObject, splitterStates, setSplitterStates, "SplitterStates", "UI", {}, Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QString, windowState, setWindowState, "WindowState", "UI", QString(), Configuration::Property::Flag::Standard )	\
	OP( UserConfig, VeyonMaster::userConfig(), QString, windowGeometry, setWindowGeometry, "WindowGeometry", "UI", QString(), Configuration::Property::Flag::Standard )

	FOREACH_PERSONAL_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)
};
