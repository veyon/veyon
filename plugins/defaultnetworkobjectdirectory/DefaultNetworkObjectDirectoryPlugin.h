/*
 * DefaultNetworkObjectDirectoryPlugin.h - declaration of DefaultNetworkObjectDirectoryPlugin class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef DEFAULT_NETWORK_OBJECT_DIRECTORY_PLUGIN_H
#define DEFAULT_NETWORK_OBJECT_DIRECTORY_PLUGIN_H

#include "ConfigurationPagePluginInterface.h"
#include "DefaultNetworkObjectDirectoryConfiguration.h"
#include "NetworkObjectDirectoryPluginInterface.h"

class DefaultNetworkObjectDirectoryPlugin : public QObject,
		PluginInterface,
		NetworkObjectDirectoryPluginInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.DefaultNetworkObjectDirectory")
	Q_INTERFACES(PluginInterface NetworkObjectDirectoryPluginInterface ConfigurationPagePluginInterface)
public:
	DefaultNetworkObjectDirectoryPlugin( QObject* paren = nullptr );
	virtual ~DefaultNetworkObjectDirectoryPlugin();

	Plugin::Uid uid() const override
	{
		return "14bacaaa-ebe5-449c-b881-5b382f952571";
	}

	QString version() const override
	{
		return QStringLiteral( "1.1" );
	}

	QString name() const override
	{
		return QStringLiteral( "DefaultNetworkObjectDirectory" );
	}

	QString description() const override
	{
		return tr( "Network object directory which stores objects in local configuration" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QString directoryName() const override
	{
		return tr( "Default (store objects in local configuration)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	ConfigurationPage* createConfigurationPage() override;

private:
	DefaultNetworkObjectDirectoryConfiguration m_configuration;

};

#endif // DEFAULT_NETWORK_OBJECT_DIRECTORY_PLUGIN_H
