/*
 * LdapCommandLinePlugin.h - declaration of LdapCommandLinePlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include "CommandLinePluginInterface.h"
#include "NetworkObjectDirectoryPluginInterface.h"

class LdapPlugin : public QObject, PluginInterface, CommandLinePluginInterface, NetworkObjectDirectoryPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.italc-solutions.iTALC.Plugins.Ldap")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface NetworkObjectDirectoryPluginInterface)
public:
	LdapPlugin();
	virtual ~LdapPlugin();

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
		return tr( "Configure LDAP/AD integration of iTALC at command line" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "iTALC Community" );
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
		return QStringLiteral( "operations for configuring LDAP/AD integration of iTALC" );
	}

	QStringList subCommands() const override;
	QString subCommandHelp( const QString& subCommand ) const override;
	RunResult runCommand( const QStringList& arguments ) override;

	QString directoryName() const override
	{
		return tr( "LDAP (load objects from LDAP/AD)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent );

public slots:
	CommandLinePluginInterface::RunResult handle_autoconfigurebasedn( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );

private:
	QMap<QString, QString> m_subCommands;

};

#endif // LDAP_PLUGIN_H
