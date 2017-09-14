/*
 * NetworkObjectDirectoryPluginInterface.h - plugin interface for network
 *                                           object directory implementations
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

#ifndef NETWORK_OBJECT_DIRECTORY_PLUGIN_INTERFACE_H
#define NETWORK_OBJECT_DIRECTORY_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

class NetworkObjectDirectory;

// clazy:excludeall=copyable-polymorphic

class NetworkObjectDirectoryPluginInterface
{
public:
	virtual QString directoryName() const = 0;
	virtual NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) = 0;

};

typedef QList<NetworkObjectDirectoryPluginInterface> NetworkObjectDirectoryPluginInterfaceList;

#define NetworkObjectDirectoryPluginInterface_iid "org.veyon.Veyon.Plugins.NetworkObjectPluginInterface"

Q_DECLARE_INTERFACE(NetworkObjectDirectoryPluginInterface, NetworkObjectDirectoryPluginInterface_iid)

#endif // NETWORK_OBJECT_DIRECTORY_PLUGIN_INTERFACE_H
