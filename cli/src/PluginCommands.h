/*
 * PluginsCommands.h - declaration of PluginsCommands class
 *
 * Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
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
#include "CommandLineIO.h"

class PluginCommands : public QObject, CommandLinePluginInterface, PluginInterface, CommandLineIO
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit PluginCommands( QObject* parent = nullptr );
	~PluginCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("cbea24b5-4f6b-446d-8f54-4f98ec796a8c") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "Plugin" );
	}

	QString description() const override
	{
		return tr( "Plugin-related CLI operations" );
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
		return QStringLiteral( "plugin" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for managing plugins" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_show( const QStringList& arguments );

private:
	const QMap<QString, QString> m_commands;

};
