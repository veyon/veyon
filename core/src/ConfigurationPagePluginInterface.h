// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PluginInterface.h"

class ConfigurationPage;

// clazy:excludeall=copyable-polymorphic

class ConfigurationPagePluginInterface
{
public:
	virtual ConfigurationPage* createConfigurationPage() = 0;
};

using ConfigurationPagePluginInterfaceList = QList<ConfigurationPagePluginInterface>;

#define ConfigurationPagePluginInterface_iid "io.veyon.Veyon.Plugins.ConfigurationPagePluginInterface"

Q_DECLARE_INTERFACE(ConfigurationPagePluginInterface, ConfigurationPagePluginInterface_iid)
