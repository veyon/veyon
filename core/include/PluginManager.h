/*
 * PluginManager.h - header for the PluginManager class
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

#include <QObject>

#include "Plugin.h"
#include "PluginInterface.h"

class VEYON_CORE_EXPORT PluginManager : public QObject
{
	Q_OBJECT
public:
	PluginManager( QObject* parent = nullptr );

	void loadPlatformPlugins();
	void loadPlugins();
	void upgradePlugins();

	const PluginInterfaceList& pluginInterfaces() const
	{
		return m_pluginInterfaces;
	}

	PluginInterfaceList& pluginInterfaces()
	{
		return m_pluginInterfaces;
	}

	QObjectList& pluginObjects()
	{
		return m_pluginObjects;
	}

	void registerExtraPluginInterface( QObject* pluginObject );

	PluginUidList pluginUids() const;

	PluginInterface* pluginInterface( Plugin::Uid pluginUid );

	QString pluginName( Plugin::Uid pluginUid ) const;

private:
	void initPluginSearchPath();
	void loadPlugins( const QString& nameFilter );

	PluginInterfaceList m_pluginInterfaces;
	QObjectList m_pluginObjects;
	bool m_noDebugMessages;

signals:
	void pluginsLoaded();

};
