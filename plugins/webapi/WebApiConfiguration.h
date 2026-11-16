// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "VeyonConfiguration.h"

#define FOREACH_HTTP_API_CONFIG_PROPERTY(OP) \
	OP(WebApiConfiguration, m_configuration, bool, httpServerEnabled, setHttpServerEnabled, "HttpServerEnabled", "WebAPI", false, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, int, httpServerPort, setHttpServerPort, "HttpServerPort", "WebAPI", 11080, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, int, connectionLifetime, setConnectionLifetime, "ConnectionLifetime", "WebAPI", 3, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, int, connectionIdleTimeout, setConnectionIdleTimeout, "ConnectionIdleTimeout", "WebAPI", 60, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, int, connectionAuthenticationTimeout, setConnectionAuthenticationTimeout, "ConnectionAuthenticationTimeout", "WebAPI", 15, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, int, connectionLimit, setConnectionLimit, "ConnectionLimit", "WebAPI", 32, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, bool, httpsEnabled, setHttpsEnabled, "HttpsEnabled", "WebAPI", false, Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, QString, tlsCertificateFile, setTlsCertificateFile, "TlsCertificateFile", "WebAPI", QString(), Configuration::Property::Flag::Advanced) \
	OP(WebApiConfiguration, m_configuration, QString, tlsPrivateKeyFile, setTlsPrivateKeyFile, "TlsPrivateKeyFile", "WebAPI", QString(), Configuration::Property::Flag::Advanced)

DECLARE_CONFIG_PROXY(WebApiConfiguration, FOREACH_HTTP_API_CONFIG_PROPERTY)
