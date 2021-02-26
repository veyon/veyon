/*
 * WebApiConfiguration.h - configuration values for the HTTP API plugin
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonConfiguration.h"
#include "Configuration/Proxy.h"

#define FOREACH_HTTP_API_CONFIG_PROPERTY(OP) \
    OP( WebApiConfiguration, m_configuration, bool, httpServerEnabled, setHttpServerEnabled, "HttpServerEnabled", "WebAPI", false, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, int, httpServerPort, setHttpServerPort, "HttpServerPort", "WebAPI", 11080, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, int, connectionLifetime, setConnectionLifetime, "ConnectionLifetime", "WebAPI", 3, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, int, connectionIdleTimeout, setConnectionIdleTimeout, "ConnectionIdleTimeout", "WebAPI", 60, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, int, connectionAuthenticationTimeout, setConnectionAuthenticationTimeout, "ConnectionAuthenticationTimeout", "WebAPI", 15, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, int, connectionLimit, setConnectionLimit, "ConnectionLimit", "WebAPI", 32, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, bool, httpsEnabled, setHttpsEnabled, "HttpsEnabled", "WebAPI", false, Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, QString, tlsCertificateFile, setTlsCertificateFile, "TlsCertificateFile", "WebAPI", QString(), Configuration::Property::Flag::Advanced )	\
    OP( WebApiConfiguration, m_configuration, QString, tlsPrivateKeyFile, setTlsPrivateKeyFile, "TlsPrivateKeyFile", "WebAPI", QString(), Configuration::Property::Flag::Advanced )	\

DECLARE_CONFIG_PROXY(WebApiConfiguration, FOREACH_HTTP_API_CONFIG_PROPERTY)
