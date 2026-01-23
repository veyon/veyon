// Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"

#define FOREACH_AUTH_LDAP_CONFIG_PROPERTY(OP) \
	OP( AuthLdapConfiguration, m_configuration, QString, usernameBindDnMapping, setUsernameBindMapping, "UsernameBindDnMapping", "LDAP", QString(), Configuration::Property::Flag::Standard )	\

// clazy:excludeall=missing-qobject-macro

DECLARE_CONFIG_PROXY(AuthLdapConfiguration, FOREACH_AUTH_LDAP_CONFIG_PROPERTY)
