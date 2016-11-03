/*
 * KLdapIntegration.h - definition of logging category for kldap
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <QDebug>

#include "LdapDirectory.h"

#include "ItalcCore.h"
#include "ItalcConfiguration.h"

#include "ldapconnection.h"
#include "ldapoperation.h"
#include "ldapserver.h"
#include "ldapdn.h"


class LdapDirectory::LdapDirectoryPrivate
{
public:
	KLDAP::LdapConnection connection;
	KLDAP::LdapOperation operation;
	QString usersDn;
	QString groupsDn;

	QString userLoginAttribute;

	bool isConnected;
	bool isBound;
};



LdapDirectory::LdapDirectory() :
	d( new LdapDirectoryPrivate )
{
	reconnect();
}



LdapDirectory::~LdapDirectory()
{
}



bool LdapDirectory::isConnected() const
{
	return d->isConnected;
}



bool LdapDirectory::isBound() const
{
	return d->isBound;
}



QString LdapDirectory::ldapErrorDescription() const
{
	QString errorString = d->connection.ldapErrorString();
	if( errorString.isEmpty() == false )
	{
		return tr( "LDAP error description: %1" ).arg( errorString );
	}

	return QString();
}



QStringList LdapDirectory::users(const QString &filter)
{
	return queryEntries( d->usersDn, d->userLoginAttribute, filter );
}



QStringList LdapDirectory::groups(const QString &filter)
{
	return queryEntries( d->groupsDn, "cn", filter );
}




bool LdapDirectory::reconnect()
{
	ItalcConfiguration* c = ItalcCore::config;

	KLDAP::LdapServer server;

	server.setHost( c->ldapServerHost() );
	server.setPort( c->ldapServerPort() );
	server.setBaseDn( KLDAP::LdapDN( c->ldapBaseDn() ) );
	//server.setUser( c->ldapBindDn() );
	server.setBindDn( c->ldapBindDn() );
	server.setPassword( c->ldapBindPassword() );
	server.setSecurity( KLDAP::LdapServer::None );
	server.setAuth( KLDAP::LdapServer::Simple );

	d->connection.close();

	d->isConnected = false;
	d->isBound = false;

	d->connection.setServer( server );
	if( d->connection.connect() != 0 )
	{
		qWarning() << "LDAP connect failed:" << ldapErrorDescription();
		return false;
	}

	d->isConnected = true;

	d->operation.setConnection( d->connection );
	if( d->operation.bind_s() != 0 )
	{
		qWarning() << "LDAP bind failed:" << ldapErrorDescription();
		return false;
	}

	d->isBound = true;

	d->usersDn = c->ldapUserTree() + "," + c->ldapBaseDn();
	d->groupsDn = c->ldapGroupTree() + "," + c->ldapBaseDn();
	d->userLoginAttribute = c->ldapUserLoginAttribute();

	return true;
}



QStringList LdapDirectory::queryEntries(const QString &dn, const QString &attribute, const QString &filter)
{
	QStringList entries;

	int id = d->operation.search( KLDAP::LdapDN( dn ), KLDAP::LdapUrl::Sub, filter, QStringList( attribute ) );

	if( id != -1 )
	{
		while( d->operation.waitForResult(id) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
		{
			entries += d->operation.object().value( attribute );
		}
	}
	else
	{
		qWarning() << "LDAP search failed:" << ldapErrorDescription();
	}

	return entries;
}
