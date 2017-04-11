/*
 * ServiceControlPlugin.h - declaration of ServiceControlPlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef SERVICE_CONTROL_PLUGIN_H
#define SERVICE_CONTROL_PLUGIN_H

#include "CommandLinePluginInterface.h"
#include "ServiceControl.h"

class ServiceControlPlugin : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon-solutions.Veyon.Plugins.ServiceControl")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	ServiceControlPlugin();
	~ServiceControlPlugin() override;

	Plugin::Uid uid() const override
	{
		return "b47bcae0-24ff-4bf5-869c-484d64af5c4c";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "ServiceControl" );
	}

	QString description() const override
	{
		return tr( "Configure and control Veyon service" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Doerffel" );
	}

	QString commandName() const override
	{
		return QStringLiteral( "service" );
	}

	QString commandHelp() const override
	{
		return tr( "commands for configuring and controlling Veyon Service" );
	}

	QStringList subCommands() const override
	{
		return m_subCommands.keys();
	}

	QString subCommandHelp( const QString& subCommand ) const override
	{
		return m_subCommands.value( subCommand );
	}

	RunResult runCommand( const QStringList& arguments ) override;

public slots:
	CommandLinePluginInterface::RunResult handle_register( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_unregister( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_start( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_stop( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_restart( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_status( const QStringList& arguments );

private:
	QMap<QString, QString> m_subCommands;
	ServiceControl m_serviceControl;

};

#endif // SERVICE_CONTROL_PLUGIN_H
