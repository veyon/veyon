/*
 * NetworkObjectDirectoryManager.h - header file for NetworkObjectDirectoryManager
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"
#include "Plugin.h"

class NetworkObjectDirectory;
class NetworkObjectDirectoryPluginInterface;
class PluginInterface;

class VEYON_CORE_EXPORT NetworkObjectDirectoryManager : public QObject
{
	Q_OBJECT
public:
	using Plugins = QMap<Plugin::Uid, NetworkObjectDirectoryPluginInterface *>;

	explicit NetworkObjectDirectoryManager( QObject* parent = nullptr );

	const Plugins& plugins() const
	{
		return m_plugins;
	}

	NetworkObjectDirectory* configuredDirectory();

	NetworkObjectDirectory* createDirectory( Plugin::Uid uid, QObject* parent );

	void setEnabled( Plugin::Uid uid, bool enabled );
	bool isEnabled( Plugin::Uid uid ) const;

	void setEnabled( NetworkObjectDirectoryPluginInterface* plugin, bool enabled );
	bool isEnabled( NetworkObjectDirectoryPluginInterface* plugin ) const;

private:
	Plugins m_plugins{};
	NetworkObjectDirectory* m_configuredDirectory{nullptr};

};
