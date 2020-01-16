/*
 * AuthLogonPlugin.h - declaration of AuthLogonPlugin class
 *
 * Copyright (c) 2018-2020 Tobias Junghans <tobydox@veyon.io>
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

class AuthLogonPlugin : public QObject,
		AuthenticationPluginInterface,
		PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AuthLogon")
	Q_INTERFACES(PluginInterface
				 AuthenticationPluginInterface)
public:
	explicit AuthLogonPlugin( QObject* parent = nullptr );
	~AuthLogonPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("63611f7c-b457-42c7-832e-67d0f9281085");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "AuthLogon" );
	}

	QString description() const override
	{
		return tr( "Logon authentication" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	QString authenticationTypeName() const override
	{
		return description();
	}

	bool initializeCredentials() override;
	bool hasCredentials() const override;

	bool checkCredentials() const override;

	void configureCredentials() override;

	VncServerClient::AuthState performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const override;

	bool authenticate( QIODevice* socket ) const override;

	QString accessControlUser() const override
	{
		return m_username;
	}

private:
	QString m_username;
	CryptoCore::PlaintextPassword m_password;

};
