// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "LdapConfiguration.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "UserGroupsBackendInterface.h"

class LdapDirectory;

class LdapPlugin : public QObject, PluginInterface,
		CommandLinePluginInterface,
		NetworkObjectDirectoryPluginInterface,
		UserGroupsBackendInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Ldap")
	Q_INTERFACES(PluginInterface
				 CommandLinePluginInterface
				 NetworkObjectDirectoryPluginInterface
				 UserGroupsBackendInterface
				 ConfigurationPagePluginInterface)
public:
	explicit LdapPlugin( QObject* parent = nullptr );
	~LdapPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("6f0a491e-c1c6-4338-8244-f823b0bf8670") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 2 );
	}

	QString name() const override
	{
		return QStringLiteral( "LDAP Basic" );
	}

	QString description() const override
	{
		return tr( "Basic LDAP/AD support for Veyon" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	void upgrade( const QVersionNumber& oldVersion ) override;

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "ldap" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for configuring and testing LDAP/AD integration" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

	QString directoryName() const override
	{
		return tr( "%1 (load computers and locations from LDAP/AD)" ).arg( name() );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	QString userGroupsBackendName() const override
	{
		return tr( "%1 (load users and groups from LDAP/AD)" ).arg( name() );
	}

	void reloadConfiguration() override;

	QStringList userGroups( bool queryDomainGroups ) override;
	QStringList groupsOfUser( const QString& username, bool queryDomainGroups ) override;

	ConfigurationPage* createConfigurationPage() override;


public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_autoconfigurebasedn( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_query( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );

private:
	enum {
		MaximumPlaintextPasswordLength = 64
	};

	LdapClient& ldapClient();
	LdapDirectory& ldapDirectory();

	LdapConfiguration m_configuration;
	LdapClient* m_ldapClient;
	LdapDirectory* m_ldapDirectory;
	QMap<QString, QString> m_commands;

};
