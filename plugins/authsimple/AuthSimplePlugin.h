/*
 * AuthSimplePlugin.h - declaration of AuthSimplePlugin class
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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

class AuthSimplePlugin : public QObject,
		AuthenticationPluginInterface,
		PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AuthSimple")
	Q_INTERFACES(PluginInterface
				 AuthenticationPluginInterface)
public:
	explicit AuthSimplePlugin( QObject* parent = nullptr );
	~AuthSimplePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("3940fdb4-bcc5-4cba-a227-1a2b22b5971d");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "AuthSimple" );
	}

	QString description() const override
	{
		return tr( "Simple password authentication" );
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

	VncServerClient::AuthState performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const override;

	bool authenticate( QIODevice* socket ) const override;

private:
	CryptoCore::SecureArray m_password;

};
