/*
 * PlatformPluginInterface.h - interface class for platform plugins
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "PluginInterface.h"

class PlatformCoreFunctions;
class PlatformFilesystemFunctions;
class PlatformInputDeviceFunctions;
class PlatformNetworkFunctions;
class PlatformServiceFunctions;
class PlatformUserFunctions;
class PlatformSmartcardFunctions;

// clazy:excludeall=copyable-polymorphic

class PlatformPluginInterface
{
public:
	virtual PlatformCoreFunctions& coreFunctions() = 0;
	virtual PlatformFilesystemFunctions& filesystemFunctions() = 0;
	virtual PlatformInputDeviceFunctions& inputDeviceFunctions() = 0;
	virtual PlatformNetworkFunctions& networkFunctions() = 0;
	virtual PlatformServiceFunctions& serviceFunctions() = 0;
	virtual PlatformUserFunctions& userFunctions() = 0;
	virtual PlatformSmartcardFunctions& smartcardFunctions() = 0;

};

using PlatformPluginInterfaceList = QList<PlatformPluginInterface *>;

#define PlatformPluginInterface_iid "io.veyon.Veyon.Plugins.PlatformPluginInterface"

Q_DECLARE_INTERFACE(PlatformPluginInterface, PlatformPluginInterface_iid)
