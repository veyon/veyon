// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "AuthLdapCore.h"
#include "LdapClient.h"
#include "LdapConfiguration.h"
#include "VeyonConfiguration.h"


AuthLdapCore::AuthLdapCore( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



void AuthLdapCore::clear()
{
	m_username.clear();
	m_password.clear();
}



bool AuthLdapCore::hasCredentials() const
{
	return m_username.isEmpty() == false && m_password.isEmpty() == false;
}



bool AuthLdapCore::authenticate()
{
	Configuration::Object configCopy;
	configCopy += VeyonCore::config();

	LdapConfiguration ldapConfig( &configCopy );
	ldapConfig.setUseBindCredentials( true );
	ldapConfig.setBindPassword( Configuration::Password::fromPlainText( m_password ) );

	if( m_configuration.usernameBindDnMapping().isEmpty() )
	{
		ldapConfig.setBindDn( m_username );
	}
	else
	{
		ldapConfig.setBindDn( m_configuration.usernameBindDnMapping().replace( QStringLiteral("%username%"), m_username ) );
	}

	return LdapClient( ldapConfig ).isBound();
}


IMPLEMENT_CONFIG_PROXY(AuthLdapConfiguration)
