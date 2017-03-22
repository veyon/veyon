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
#include "ItalcConfiguration.h"

class ConfigCommandLinePlugin : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.italc-solutions.iTALC.Plugins.ConfigCommandLineInterface")
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
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "Config" );
	}

	QString description() const override
	{
		return tr( "Configure iTALC at command line" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "iTALC Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Doerffel" );
	}

	QString commandName() const override
	{
		return QStringLiteral( "config" );
	}

	QString commandHelp() const override
	{
		return QStringLiteral( "operations for changing the configuration of iTALC" );
	}

	QStringList subCommands() const override;
	QString subCommandHelp( const QString& subCommand ) const override;
	RunResult runCommand( const QStringList& arguments ) override;

public slots:
	CommandLinePluginInterface::RunResult handle_clear( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_import( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_export( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_get( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_set( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_unset( const QStringList& arguments );

private:
	void listConfiguration( const ItalcConfiguration::DataMap &map,
							const QString &parentKey );
	CommandLinePluginInterface::RunResult applyConfiguration();

	CommandLinePluginInterface::RunResult operationError( const QString& message );

	QMap<QString, QString> m_subCommands;

};

#endif // CONFIG_COMMAND_LINE_PLUGIN_H
