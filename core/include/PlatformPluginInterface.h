/*
 * PlatformPluginInterface.h - interface class for platform plugins
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef PLATFORM_PLUGIN_INTERFACE_H
#define PLATFORM_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

class PlatformNetworkFunctions;
class PlatformUserSessionFunctions;

class VEYON_CORE_EXPORT PlatformPluginInterface
{
public:
	virtual ~PlatformPluginInterface() {}

	virtual PlatformNetworkFunctions* networkFunctions() = 0;
	virtual PlatformUserSessionFunctions* userSessionFunctions() = 0;

};

typedef QList<PlatformPluginInterface *> PlatformPluginInterfaceList;

#define PlatformPluginInterface_iid "org.veyon.Veyon.Plugins.PlatformPluginInterface"

Q_DECLARE_INTERFACE(PlatformPluginInterface, PlatformPluginInterface_iid)

#endif // PLATFORM_PLUGIN_INTERFACE_H
