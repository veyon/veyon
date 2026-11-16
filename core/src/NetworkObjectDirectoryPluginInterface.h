// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PluginInterface.h"

class NetworkObjectDirectory;

// clazy:excludeall=copyable-polymorphic

class NetworkObjectDirectoryPluginInterface
{
public:
	virtual QString directoryName() const = 0;
	virtual NetworkObjectDirectory* createNetworkObjectDirectory(QObject* parent) = 0;
};

using NetworkObjectDirectoryPluginInterfaceList = QList<NetworkObjectDirectoryPluginInterface>;

#define NetworkObjectDirectoryPluginInterface_iid "io.veyon.Veyon.Plugins.NetworkObjectPluginInterface"

Q_DECLARE_INTERFACE(NetworkObjectDirectoryPluginInterface, NetworkObjectDirectoryPluginInterface_iid)
