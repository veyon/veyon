/*
 * ConfigCommandLinePlugin.h - declaration of ConfigCommandLinePlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef CONFIG_COMMAND_LINE_PLUGIN_H
#define CONFIG_COMMAND_LINE_PLUGIN_H

#include "CommandLinePluginInterface.h"
#include "VeyonConfiguration.h"

class ConfigCommandLinePlugin : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ConfigCommandLineInterface")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	ConfigCommandLinePlugin( QObject* parent = nullptr );
	~ConfigCommandLinePlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("1bdb0d1c-f8eb-4d21-a093-d555a10f3975");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "Config" );
	}

	QString description() const override
	{
		return tr( "Configure Veyon at command line" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "config" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for managing the configuration of Veyon" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public slots:
	CommandLinePluginInterface::RunResult handle_clear( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_import( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_export( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_get( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_set( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_unset( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_upgrade( const QStringList& arguments );

private:
	void listConfiguration( const VeyonConfiguration::DataMap &map,
							const QString &parentKey );
	CommandLinePluginInterface::RunResult applyConfiguration();

	static QString printableConfigurationValue( const QVariant& value );

	CommandLinePluginInterface::RunResult operationError( const QString& message );

	QMap<QString, QString> m_commands;

};

#endif // CONFIG_COMMAND_LINE_PLUGIN_H
