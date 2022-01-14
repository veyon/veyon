/*
 * AuthLdapCore.cpp - implementation of AuthLdapCore class
 *
 * Copyright (c) 2020-2022 Tobias Junghans <tobydox@veyon.io>
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
