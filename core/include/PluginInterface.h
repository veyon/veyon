/*
 * PluginInterface.h - interface class for plugins
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

#include "VeyonCore.h"
#include "Plugin.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT PluginInterface
{
public:
	virtual Plugin::Uid uid() const = 0;
	virtual QVersionNumber version() const = 0;
	virtual QString name() const = 0;
	virtual QString description() const = 0;
	virtual QString vendor() const = 0;
	virtual QString copyright() const = 0;
	virtual Plugin::Flags flags() const
	{
		return Plugin::NoFlags;
	}
	virtual void upgrade( const QVersionNumber& oldVersion )
	{
		Q_UNUSED(oldVersion)
	}

};

typedef QList<PluginInterface *> PluginInterfaceList;

#define PluginInterface_iid "io.veyon.Veyon.Plugins.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
