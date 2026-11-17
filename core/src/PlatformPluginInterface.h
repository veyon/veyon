// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PluginInterface.h"

class PlatformCoreFunctions;
class PlatformFilesystemFunctions;
class PlatformInputDeviceFunctions;
class PlatformNetworkFunctions;
class PlatformSecretFunctions;
class PlatformServiceFunctions;
class PlatformSessionFunctions;
class PlatformUserFunctions;

// clazy:excludeall=copyable-polymorphic

class PlatformPluginInterface
{
public:
	virtual PlatformCoreFunctions& coreFunctions() = 0;
	virtual PlatformFilesystemFunctions& filesystemFunctions() = 0;
	virtual PlatformInputDeviceFunctions& inputDeviceFunctions() = 0;
	virtual PlatformNetworkFunctions& networkFunctions() = 0;
	virtual PlatformSecretFunctions& secretFunctions() = 0;
	virtual PlatformServiceFunctions& serviceFunctions() = 0;
	virtual PlatformSessionFunctions& sessionFunctions() = 0;
	virtual PlatformUserFunctions& userFunctions() = 0;
};

using PlatformPluginInterfaceList = QList<PlatformPluginInterface*>;

#define PlatformPluginInterface_iid "io.veyon.Veyon.Plugins.PlatformPluginInterface"

Q_DECLARE_INTERFACE(PlatformPluginInterface, PlatformPluginInterface_iid)
