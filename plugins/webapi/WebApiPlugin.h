/*
 * WebApiPlugin.h - declaration of WebApiPlugin class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QThread>

#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "WebApiConfiguration.h"

class WebApiHttpServer;

class WebApiPlugin : public QObject, PluginInterface, CommandLinePluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.WebApi")
	Q_INTERFACES(PluginInterface
					  CommandLinePluginInterface
						  ConfigurationPagePluginInterface)
public:
	explicit WebApiPlugin( QObject* parent = nullptr );
	~WebApiPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("e11bee03-b99c-465c-bf90-7e5339b83f6b");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("WebAPI");
	}

	QString description() const override
	{
		return tr( "Provide access to a computer via HTTP" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "webapi" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for running the WebAPI server" );
	}

	QStringList commands() const override
	{
		return m_commands.keys();
	}

	QString commandHelp( const QString& command ) const override
	{
		return m_commands.value( command );
	}

	ConfigurationPage* createConfigurationPage() override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_runserver( const QStringList& arguments );

private:
	void startHttpServerThread();

	static constexpr auto ServerThreadTerminationTimeout = 1000;

	WebApiConfiguration m_configuration;

	QThread m_httpServerThread;
	WebApiHttpServer* m_httpServer{nullptr};

	const QMap<QString, QString> m_commands;

};
