/*
 * AuthKeysPlugin.h - declaration of AuthKeysPlugin class
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

#ifndef AUTH_KEYS_PLUGIN_H
#define AUTH_KEYS_PLUGIN_H

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"

class AuthKeysTableModel;

class AuthKeysPlugin : public QObject,
		CommandLinePluginInterface,
		PluginInterface,
		CommandLineIO,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AuthKeys")
	Q_INTERFACES(PluginInterface
				 CommandLinePluginInterface
				 ConfigurationPagePluginInterface)
public:
	AuthKeysPlugin( QObject* parent = nullptr );
	~AuthKeysPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("4790bad8-4c56-40d5-8361-099a68f0c24b");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "AuthKeys" );
	}

	QString description() const override
	{
		return tr( "Command line support for managing authentication keys" );
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
		return QStringLiteral( "authkeys" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for managing authentication keys" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

	ConfigurationPage* createConfigurationPage() override;

public slots:
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_setaccessgroup( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_create( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_delete( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_export( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_import( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_extract( const QStringList& arguments );

private:
	static void printAuthKeyTable();
	static QString authKeysTableData( const AuthKeysTableModel& tableModel, int row, int column );
	static void printAuthKeyList();

	QMap<QString, QString> m_commands;

};

#endif // AUTH_KEYS_COMMAND_LINE_PLUGIN_H
