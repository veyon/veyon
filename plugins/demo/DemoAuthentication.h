/*
 * DemoAuthentication.h - declaration of DemoAuthentication class
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

class DemoAuthentication : public AuthenticationPluginInterface
{
public:
	using AccessToken = CryptoCore::PlaintextPassword;

	explicit DemoAuthentication( const Plugin::Uid& pluginUid );
	~DemoAuthentication() override = default;

	QString authenticationTypeName() const override
	{
		return {};
	}

	bool initializeCredentials() override;
	bool hasCredentials() const override;

	bool checkCredentials() const override;

	VncServerClient::AuthState performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const override;

	bool authenticate( QIODevice* socket ) const override;

	bool requiresAccessControl() const override
	{
		return false;
	}

	const AccessToken& accessToken() const
	{
		return m_accessToken;
	}

	void setAccessToken( const AccessToken& token )
	{
		m_accessToken = token;
	}

	const Plugin::Uid& pluginUid() const
	{
		return m_pluginUid;
	}

private:
	AccessToken m_accessToken;
	Plugin::Uid m_pluginUid;

};
