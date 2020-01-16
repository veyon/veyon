/*
 * AuthenticationPluginInterface.h - interface class for authentication plugins
 *
 * Copyright (c) 2019-2020 Tobias Junghans <tobydox@veyon.io>
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

#include "PluginInterface.h"
#include "VncServerClient.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT AuthenticationPluginInterface
{
	Q_GADGET
public:
	virtual ~AuthenticationPluginInterface() = default;

	virtual QString authenticationTypeName() const = 0;

	virtual bool initializeCredentials() = 0;
	virtual bool hasCredentials() const = 0;
	virtual bool checkCredentials() const = 0;
	virtual void configureCredentials() = 0;

	// server side authentication
	virtual VncServerClient::AuthState performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const = 0;

	// client side authentication
	virtual bool authenticate( QIODevice* socket ) const = 0;

	virtual bool requiresAccessControl() const
	{
		return true;
	}

	virtual QString accessControlUser() const;

	static QString authenticationTestTitle()
	{
		return VeyonCore::tr( "Authentication test");
	}

};

#define AuthenticationPluginInterface_iid "io.veyon.Veyon.Plugins.AuthenticationPluginInterface"

Q_DECLARE_INTERFACE(AuthenticationPluginInterface, AuthenticationPluginInterface_iid)
