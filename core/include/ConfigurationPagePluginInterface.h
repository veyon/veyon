/*
 * ConfigurationPagePluginInterface.h - interface class for configuration pages
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef CONFIGURATION_PAGE_PLUGIN_INTERFACE_H
#define CONFIGURATION_PAGE_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

class ConfigurationPage;

// clazy:excludeall=copyable-polymorphic

class ConfigurationPagePluginInterface
{
public:
	virtual ConfigurationPage* createConfigurationPage() = 0;

};

typedef QList<ConfigurationPagePluginInterface> ConfigurationPagePluginInterfaceList;

#define ConfigurationPagePluginInterface_iid "org.veyon.Veyon.Plugins.ConfigurationPagePluginInterface"

Q_DECLARE_INTERFACE(ConfigurationPagePluginInterface, ConfigurationPagePluginInterface_iid)

#endif // CONFIGURATION_PAGE_PLUGIN_INTERFACE_H
