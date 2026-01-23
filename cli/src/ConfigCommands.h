/*
 * ConfigCommands.h - declaration of ConfigCommands class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "CommandLinePluginInterface.h"
#include "VeyonConfiguration.h"

class ConfigCommands : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit ConfigCommands( QObject* parent = nullptr );
	~ConfigCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("1bdb0d1c-f8eb-4d21-a093-d555a10f3975") };
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

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_clear( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_import( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_export( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_get( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_set( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_unset( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_upgrade( const QStringList& arguments );

private:
	enum class ListMode {
		Values,
		Defaults,
		Types
	};

	void listConfiguration( ListMode listMode ) const;

	CommandLinePluginInterface::RunResult applyConfiguration();

	static QString printableConfigurationValue( const QVariant& value );

	CommandLinePluginInterface::RunResult operationError( const QString& message );

	QMap<QString, QString> m_commands;

};
