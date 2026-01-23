/*
 * FeatureCommands.h - declaration of FeatureCommands class
 *
 * Copyright (c) 2021-2026 Tobias Junghans <tobydox@veyon.io>
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
#include "FeatureProviderInterface.h"

class FeatureCommands : public QObject, CommandLinePluginInterface, PluginInterface, CommandLineIO
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit FeatureCommands( QObject* parent = nullptr );
	~FeatureCommands() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("2376b972-0bf1-4f50-bcfd-79d4d90730c6") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "FeatureCommands" );
	}

	QString description() const override
	{
		return tr( "Feature-related CLI operations" );
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
		return QStringLiteral( "feature" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for controlling features" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_show( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_start( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_stop( const QStringList& arguments );

private:
	static QString listCommand()
	{
		return QStringLiteral("list");
	}

	static QString showCommand()
	{
		return QStringLiteral("show");
	}

	static QString startCommand()
	{
		return QStringLiteral("start");
	}

	static QString stopCommand()
	{
		return QStringLiteral("stop");
	}

	RunResult controlComputer( FeatureProviderInterface::Operation operation, const QStringList& arguments );

	const QMap<QString, QString> m_commands;

};
