/*
 * BuiltinDirectoryPlugin.h - declaration of BuiltinDirectoryPlugin class
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

#ifndef BUILTIN_DIRECTORY_PLUGIN_H
#define BUILTIN_DIRECTORY_PLUGIN_H

#include "ConfigurationPagePluginInterface.h"
#include "BuiltinDirectoryConfiguration.h"
#include "NetworkObjectDirectoryPluginInterface.h"

class BuiltinDirectoryPlugin : public QObject,
		PluginInterface,
		NetworkObjectDirectoryPluginInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.BuiltinDirectory")
	Q_INTERFACES(PluginInterface NetworkObjectDirectoryPluginInterface ConfigurationPagePluginInterface)
public:
	BuiltinDirectoryPlugin( QObject* paren = nullptr );
	virtual ~BuiltinDirectoryPlugin();

	Plugin::Uid uid() const override
	{
		return "14bacaaa-ebe5-449c-b881-5b382f952571";
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "BuiltinDirectory" );
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

	void upgrade( const QVersionNumber& oldVersion ) override;

	QString directoryName() const override
	{
		return tr( "Builtin (computers and rooms in local configuration)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	ConfigurationPage* createConfigurationPage() override;

private:
	BuiltinDirectoryConfiguration m_configuration;

};

#endif // BUILTIN_DIRECTORY_PLUGIN_H
