/*
 * LdapPlugin.h - declaration of LdapPlugin class
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

#ifndef LDAP_PLUGIN_H
#define LDAP_PLUGIN_H

#include "AccessControlDataBackendInterface.h"
#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "LdapConfiguration.h"
#include "NetworkObjectDirectoryPluginInterface.h"

class LdapDirectory;

class LdapPlugin : public QObject, PluginInterface,
		CommandLinePluginInterface,
		NetworkObjectDirectoryPluginInterface,
		AccessControlDataBackendInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.Ldap")
	Q_INTERFACES(PluginInterface
				 CommandLinePluginInterface
				 NetworkObjectDirectoryPluginInterface
				 AccessControlDataBackendInterface
				 ConfigurationPagePluginInterface)
public:
	LdapPlugin();
	~LdapPlugin() override;

	Plugin::Uid uid() const override
	{
		return "6f0a491e-c1c6-4338-8244-f823b0bf8670";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "LDAP" );
	}

	QString description() const override
	{
		return tr( "Configure LDAP/AD integration of Veyon at command line" );
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
		return QStringLiteral( "ldap" );
	}

	QString commandHelp() const override
	{
		return QStringLiteral( "operations for configuring LDAP/AD integration of Veyon" );
	}

	QStringList subCommands() const override;
	QString subCommandHelp( const QString& subCommand ) const override;
	RunResult runCommand( const QStringList& arguments ) override;

	QString directoryName() const override
	{
		return tr( "LDAP (load objects from LDAP/AD)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	QString accessControlDataBackendName() const override
	{
		return tr( "LDAP (load users/groups and computers/rooms from LDAP/AD)" );
	}

	void reloadConfiguration() override;

	QStringList users() override;
	QStringList userGroups() override;
	QStringList groupsOfUser( QString userName ) override;
	QStringList allRooms() override;
	QStringList roomsOfComputer( QString computerName ) override;

	ConfigurationPage* createConfigurationPage() override;


public slots:
	CommandLinePluginInterface::RunResult handle_autoconfigurebasedn( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_query( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );

private:
	LdapDirectory& ldapDirectory();

	LdapConfiguration m_configuration;
	LdapDirectory* m_ldapDirectory;
	QMap<QString, QString> m_subCommands;

};

#endif // LDAP_PLUGIN_H
