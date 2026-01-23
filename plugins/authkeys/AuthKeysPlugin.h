/*
 * AuthKeysPlugin.h - declaration of AuthKeysPlugin class
 *
 * Copyright (c) 2018-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthenticationPluginInterface.h"
#include "AuthKeysConfiguration.h"
#include "AuthKeysManager.h"
#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"

class AuthKeysTableModel;

class AuthKeysPlugin : public QObject,
		AuthenticationPluginInterface,
		CommandLinePluginInterface,
		PluginInterface,
		CommandLineIO
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AuthKeys")
	Q_INTERFACES(PluginInterface
				 AuthenticationPluginInterface
				 CommandLinePluginInterface)
public:
	explicit AuthKeysPlugin( QObject* parent = nullptr );
	~AuthKeysPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("0c69b301-81b4-42d6-8fae-128cdd113314") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 2, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "AuthKeys" );
	}

	QString description() const override
	{
		return tr( "Key file authentication" );
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

	QString authenticationMethodName() const override
	{
		return tr("Key file");
	}

	QWidget* createAuthenticationConfigurationWidget() override;

	bool initializeCredentials() override;
	bool hasCredentials() const override;
	bool checkCredentials() const override;

	// server side authentication
	VncServerClient::AuthState performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const override;

	// client side authentication
	bool authenticate( QIODevice* socket ) const override;

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

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_setaccessgroup( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_create( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_delete( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_export( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_import( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_list( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_extract( const QStringList& arguments );

private:
	bool loadPrivateKey( const QString& privateKeyFile );

	void printAuthKeyTable();
	static QString authKeysTableData( const AuthKeysTableModel& tableModel, int row, int column );
	void printAuthKeyList();

	AuthKeysConfiguration m_configuration;
	AuthKeysManager m_manager;

	CryptoCore::PrivateKey m_privateKey{};
	QString m_authKeyName;

	QMap<QString, QString> m_commands;

};
