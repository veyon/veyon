/*
 * TestingCommandLinePlugin.h - declaration of TestingCommandLinePlugin class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

class TestingCommandLinePlugin : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.TestingCommandLineInterface")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	explicit TestingCommandLinePlugin( QObject* parent = nullptr );
	~TestingCommandLinePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("a8a84654-40ca-4731-811e-7e05997ed081");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "Testing" );
	}

	QString description() const override
	{
		return tr( "Test internal Veyon components and functions" );
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
		return QStringLiteral( "testing" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for testing internal components and functions of Veyon" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_checkaccess( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_authorizedgroups( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_accesscontrolrules( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_isaccessdeniedbylocalstate( const QStringList& arguments );

private:
	QMap<QString, QString> m_commands;

};
