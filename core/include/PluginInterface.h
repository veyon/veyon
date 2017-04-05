/*
 * PluginInterface.h - interface class for plugins
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QObject>

#include "ItalcCore.h"
#include "Plugin.h"

class ITALC_CORE_EXPORT PluginInterface
{
public:
	virtual ~PluginInterface() {}

	virtual Plugin::Uid uid() const = 0;
	virtual QString version() const = 0;
	virtual QString name() const = 0;
	virtual QString description() const = 0;
	virtual QString vendor() const = 0;
	virtual QString copyright() const = 0;
	virtual Plugin::Flags flags() const
	{
		return Plugin::NoFlags;
	}

};

typedef QList<PluginInterface *> PluginInterfaceList;

#define PluginInterface_iid "org.italc-solutions.iTALC.Plugins.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGIN_INTERFACE_H
