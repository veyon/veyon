/*
 * AuthenticationManager.h - header file for AuthenticationManager
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthenticationPluginInterface.h"

class VEYON_CORE_EXPORT AuthenticationManager : public QObject
{
	Q_OBJECT
public:
	using Plugins = QMap<Plugin::Uid, AuthenticationPluginInterface *>;
	using Types = QMap<Plugin::Uid, QString>;

	explicit AuthenticationManager( QObject* parent = nullptr );

	const Plugins& plugins() const
	{
		return m_plugins;
	}

	Plugin::Uid toUid( AuthenticationPluginInterface* authPlugin ) const;

	Types availableMethods() const;

	void setEnabled( Plugin::Uid uid, bool enabled );
	bool isEnabled( Plugin::Uid uid ) const;

	void setEnabled( AuthenticationPluginInterface* authPlugin, bool enabled );
	bool isEnabled( AuthenticationPluginInterface* authPlugin ) const;

	bool initializeCredentials();

	AuthenticationPluginInterface* initializedPlugin() const
	{
		return m_initializedPlugin;
	}

private:
	Plugins m_plugins{};
	AuthenticationPluginInterface* m_initializedPlugin{nullptr};

};
