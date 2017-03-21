/*
 * ConfigCommandLinePlugin.h - declaration of ConfigCommandLinePlugin class
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

#ifndef CONFIG_COMMAND_LINE_PLUGIN_H
#define CONFIG_COMMAND_LINE_PLUGIN_H

#include "CommandLinePluginInterface.h"

class ConfigCommandLinePlugin : public QObject, CommandLinePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.italc-solutions.iTALC.Plugins.PluginCommandLineInterface")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	ConfigCommandLinePlugin();
	virtual ~ConfigCommandLinePlugin();

	Plugin::Uid uid() const override
	{
		return "1bdb0d1c-f8eb-4d21-a093-d555a10f3975";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "Config";
	}

	QString description() const override
	{
		return tr( "Configure iTALC at command line" );
	}

	QString vendor() const override
	{
		return "iTALC Community";
	}

	QString copyright() const override
	{
		return "Tobias Doerffel";
	}

	const char* commandName() const override
	{
		return "config";
	}

	const char* commandHelp() const override
	{
		return "operations for changing the configuration of iTALC";
	}

	bool runCommand( const QStringList& arguments );

};

#endif // CONFIG_COMMAND_LINE_PLUGIN_H
