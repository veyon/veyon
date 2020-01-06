/*
 * AuthSmartcardPlugin.h - declaration of AuthSmartcardPlugin class
 *
 * Copyright (c) 2019 State of New Jersey
 * 
 * Maintained by Austin McGuire <austin.mcguire@jjc.nj.gov> 
 * 
 * This file is part of a fork of Veyon - https://veyon.io
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

#include "AuthSmartcardConfiguration.h"
#include "AuthSmartcardDialog.h"


class AuthSmartcardPlugin : public QObject,
		AuthenticationPluginInterface,
		PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AuthSmartcard")
	Q_INTERFACES(PluginInterface
				 AuthenticationPluginInterface)
public:
	explicit AuthSmartcardPlugin( QObject* parent = nullptr );
	~AuthSmartcardPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("17a1df39-8c05-46e7-b935-83fdf6afef1c");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "AuthSmartcard" );
	}

	QString description() const override
	{
		return tr( "Smartcard authentication" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "State Of New Jersey" );
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
		return _username;
	}

	bool validateCertificate( QString certificatePem ) const;
	bool validateCertificate( QCA::Certificate certificate ) const;
	QString certificateUpn(QString certificatePem) const;

private:
	AuthSmartcardConfiguration _configuration;
	AuthSmartcardDialog* _smartcardDialog;
	QString _serializedEntry;
	QString _pin;
	QString _username;
	QString _certificatePem;
	mutable QDateTime _lastTokenGeneration;
	mutable QCA::SecureArray _token;
	mutable QMutex _mutex;
	
};

