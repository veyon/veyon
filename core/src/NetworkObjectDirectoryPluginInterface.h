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

	// directories that read credentials from the SecretStore (e.g. a bind password) return true;
	// they are used through the directory broker in unprivileged components
	// (04-directory-broker.md §2)
	virtual bool requiresSecrets() const
	{
		return false;
	}
};

using NetworkObjectDirectoryPluginInterfaceList = QList<NetworkObjectDirectoryPluginInterface>;

// version 2 (4.12): requiresSecrets(), new virtual functions in NetworkObjectDirectory; plugins built
// against the old interface are refused by qobject_cast instead of being called with a wrong vtable
#define NetworkObjectDirectoryPluginInterface_iid "io.veyon.Veyon.Plugins.NetworkObjectPluginInterface/2"

Q_DECLARE_INTERFACE(NetworkObjectDirectoryPluginInterface, NetworkObjectDirectoryPluginInterface_iid)
