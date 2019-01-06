/*
 * ShellCommandLinePlugin.h - declaration of ShellCommandLinePlugin class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef SHELL_COMMAND_LINE_PLUGIN_H
#define SHELL_COMMAND_LINE_PLUGIN_H

#include "CommandLinePluginInterface.h"

class ShellCommandLinePlugin : public QObject, CommandLinePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ShellCommandLineInterface")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface)
public:
	ShellCommandLinePlugin( QObject* parent = nullptr );
	~ShellCommandLinePlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("85f6c631-e75a-4c78-8cb2-a7f3f502015a");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "Shell" );
	}

	QString description() const override
	{
		return tr( "Interactive shell and script execution for Veyon Control" );
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
		return QStringLiteral( "shell" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for shell functionalities" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public slots:
	CommandLinePluginInterface::RunResult handle_main();
	CommandLinePluginInterface::RunResult handle_run( const QStringList& arguments );

private:
	void runCommand( const QString& command );

	QMap<QString, QString> m_commands;

};

#endif // SHELL_COMMAND_LINE_PLUGIN_H
