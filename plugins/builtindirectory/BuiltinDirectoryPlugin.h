/*
 * BuiltinDirectoryPlugin.h - declaration of BuiltinDirectoryPlugin class
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

#ifndef BUILTIN_DIRECTORY_PLUGIN_H
#define BUILTIN_DIRECTORY_PLUGIN_H

#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "BuiltinDirectoryConfiguration.h"
#include "NetworkObject.h"
#include "NetworkObjectDirectoryPluginInterface.h"

class QFile;

class BuiltinDirectoryPlugin : public QObject,
		PluginInterface,
		NetworkObjectDirectoryPluginInterface,
		ConfigurationPagePluginInterface,
		CommandLinePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.BuiltinDirectory")
	Q_INTERFACES(PluginInterface
				 NetworkObjectDirectoryPluginInterface
				 ConfigurationPagePluginInterface
				 CommandLinePluginInterface)
public:
	BuiltinDirectoryPlugin( QObject* paren = nullptr );
	~BuiltinDirectoryPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("14bacaaa-ebe5-449c-b881-5b382f952571");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "BuiltinDirectory" );
	}

	QString description() const override
	{
		return tr( "Network object directory which stores objects in local configuration" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	void upgrade( const QVersionNumber& oldVersion ) override;

	QString directoryName() const override
	{
		return tr( "Builtin (computers and rooms in local configuration)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	ConfigurationPage* createConfigurationPage() override;

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "networkobjects" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for managing the builtin network object directory" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

public slots:
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_add( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_clear( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_dump( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_remove( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_import( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_export( const QStringList& arguments );

private:
	void listObjects( const QJsonArray& objects, const NetworkObject& parent );
	static QStringList dumpNetworkObject( const NetworkObject& object );
	static QString listNetworkObject( const NetworkObject& object );
	static QString networkObjectTypeName( const NetworkObject& object );

	CommandLinePluginInterface::RunResult saveConfiguration();

	bool importFile( QFile& inputFile, const QString& regExWithVariables, const QString& room );
	bool exportFile( QFile& outputFile, const QString& formatString, const QString& room );

	NetworkObject findNetworkObject( const QString& uidOrName ) const;

	static NetworkObject toNetworkObject( const QString& line, const QString& regExWithVariables, QString& room );
	static QString toFormattedString( const NetworkObject& networkObject, const QString& formatString, const QString& room );

	static QStringList fileImportVariables();

	BuiltinDirectoryConfiguration m_configuration;
	QMap<QString, QString> m_commands;

};

#endif // BUILTIN_DIRECTORY_PLUGIN_H
