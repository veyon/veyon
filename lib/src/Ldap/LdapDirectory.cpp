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

	QStringList queryAttributes(const QString &dn, const QString &attribute,
								const QString &filter, KLDAP::LdapUrl::Scope scope = KLDAP::LdapUrl::One )
	{
		QStringList entries;

		int id = operation.search( KLDAP::LdapDN( dn ), scope, filter, QStringList( attribute ) );

		if( id != -1 )
		{
			while( operation.waitForResult( id, LdapQueryTimeout ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
			{
				for( auto value : operation.object().values( attribute ) )
				{
					entries += value;
				}
			}
		}
		else
		{
			qWarning() << "LDAP search failed:" << ldapErrorDescription();
		}

		return entries;
	}

	QStringList queryDistinguishedNames( const QString &dn, const QString &filter, KLDAP::LdapUrl::Scope scope = KLDAP::LdapUrl::One )
	{
		QStringList distinguishedNames;

		int id = operation.search( KLDAP::LdapDN( dn ), scope, filter, QStringList() );

		if( id != -1 )
		{
			while( operation.waitForResult( id, LdapQueryTimeout ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
			{
				distinguishedNames += operation.object().dn().toString();
			}
		}
		else
		{
			qWarning() << "LDAP search failed:" << ldapErrorDescription();
		}

		return distinguishedNames;
	}

	QString ldapErrorDescription() const
	{
		QString errorString = connection.ldapErrorString();
		if( errorString.isEmpty() == false )
		{
			return tr( "LDAP error description: %1" ).arg( errorString );
		}

		return QString();
	}

	enum {
		LdapQueryTimeout = 3000
	};

	KLDAP::LdapConnection connection;
	KLDAP::LdapOperation operation;

	QString baseDn;
	QString namingContextAttribute;
	QString usersDn;
	QString groupsDn;
	QString computersDn;

	QString userLoginAttribute;
	QString groupMemberAttribute;
	QString computerHostNameAttribute;

	QString usersFilter;
	QString userGroupsFilter;
	QString computerGroupsFilter;

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



/*!
 * \brief Disables any configured filters which is required for some test scenarious
 */
void LdapDirectory::disableFilters()
{
	d->usersFilter.clear();
	d->userGroupsFilter.clear();
	d->computerGroupsFilter.clear();
}



QString LdapDirectory::ldapErrorDescription() const
{
	return d->ldapErrorDescription();
}



QStringList LdapDirectory::queryBaseDn()
{
	return d->queryDistinguishedNames( d->baseDn, QString(), KLDAP::LdapUrl::Base );
}



QString LdapDirectory::queryNamingContext()
{
	QStringList namingContextEntries = d->queryAttributes( QString(), d->namingContextAttribute, QString(), KLDAP::LdapUrl::Base );

	if( namingContextEntries.isEmpty() )
	{
		return QString();
	}

	return namingContextEntries.first();
}



QStringList LdapDirectory::users(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->usersDn,
									   constructQueryFilter( d->userLoginAttribute, filterValue, d->usersFilter ) );
}



QStringList LdapDirectory::groups(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( "cn", filterValue, QString() ) );
}



QStringList LdapDirectory::userGroups(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( "cn", filterValue, d->userGroupsFilter ) );
}


/*!
 * \brief Returns list of computer object names matching the given hostname filter
 * \param filterValue A filter value which is used to query the host name attribute
 * \return List of DNs of all matching computer objects
 */
QStringList LdapDirectory::computers(const QString &filterValue)
{
	QString queryFilter = constructQueryFilter( d->computerHostNameAttribute, filterValue, QString() );

	return d->queryDistinguishedNames( d->computersDn, queryFilter );
}



QStringList LdapDirectory::computerGroups(const QString &filterValue)
{
	QString queryFilter = constructQueryFilter( "cn", filterValue, d->computerGroupsFilter );

	return d->queryAttributes( d->groupsDn, "cn", queryFilter );
}



QStringList LdapDirectory::groupMembers(const QString &groupName)
{
	if( groupName.isEmpty() )
	{
		return QStringList();
	}

	return d->queryAttributes( d->groupsDn, d->groupMemberAttribute,
							   QString( "(cn=%1)" ).arg( groupName ) );
}



QStringList LdapDirectory::groupsOfUser(const QString &userName)
{
	QStringList userObjects = users( userName );
	if( userObjects.isEmpty() )
	{
		return QStringList();
	}

	QString queryFilter = constructQueryFilter( d->groupMemberAttribute, userObjects.first(),
												d->userGroupsFilter );

	return d->queryDistinguishedNames( d->groupsDn, queryFilter );
}



QString LdapDirectory::computerHostName(const QString &computerObjectName)
{
	if( computerObjectName.isEmpty() )
	{
		return QString();
	}

	QStringList hostNames = d->queryAttributes( d->computersDn, d->computerHostNameAttribute,
											 QString( "cn=%1" ).arg( computerObjectName ) );
	if( hostNames.isEmpty() )
	{
		return QString();
	}

	return hostNames.first();
}



bool LdapDirectory::reconnect()
{
	ItalcConfiguration* c = ItalcCore::config;

	KLDAP::LdapServer server;

	server.setHost( c->ldapServerHost() );
	server.setPort( c->ldapServerPort() );
	server.setBaseDn( KLDAP::LdapDN( c->ldapBaseDn() ) );

	if( c->ldapUseBindCredentials() )
	{
		server.setBindDn( c->ldapBindDn() );
		server.setPassword( c->ldapBindPassword() );
		server.setAuth( KLDAP::LdapServer::Simple );
	}
	else
	{
		server.setAuth( KLDAP::LdapServer::Anonymous );
	}
	server.setSecurity( KLDAP::LdapServer::None );

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

	d->namingContextAttribute = c->ldapNamingContextAttribute();

	if( d->namingContextAttribute.isEmpty() )
	{
		// fallback to AD default value
		d->namingContextAttribute = "defaultNamingContext";
	}

	// query base DN via naming context if configured
	if( c->ldapQueryNamingContext() )
	{
		d->baseDn = queryNamingContext();
	}
	else
	{
		// use the configured base DN
		d->baseDn = c->ldapBaseDn();
	}

	d->usersDn = c->ldapUserTree() + "," + c->ldapBaseDn();
	d->groupsDn = c->ldapGroupTree() + "," + c->ldapBaseDn();
	d->computersDn = c->ldapComputerTree() + "," + c->ldapBaseDn();

	d->userLoginAttribute = c->ldapUserLoginAttribute();
	d->groupMemberAttribute = c->ldapGroupMemberAttribute();
	d->computerHostNameAttribute = c->ldapComputerHostNameAttribute();

	d->usersFilter = c->ldapUsersFilter();
	d->userGroupsFilter = c->ldapUserGroupsFilter();
	d->computerGroupsFilter = c->ldapComputerGroupsFilter();

	return true;
}



QString LdapDirectory::constructQueryFilter( const QString& filterAttribute,
											 const QString& filterValue,
											 const QString& extraFilter )
{
	QString queryFilter;

	if( filterAttribute.isEmpty() == false && filterValue.isEmpty() == false )
	{
		queryFilter = QString( "(%1=%2)" ).arg( filterAttribute ).arg( filterValue );
	}

	if( extraFilter.isEmpty() == false )
	{
		if( queryFilter.isEmpty() )
		{
			queryFilter = QString( "(%1)" ).arg( extraFilter );
		}
		else
		{
			queryFilter = QString( "(&(%1)%2)" ).arg( extraFilter ).arg( queryFilter );
		}
	}

	return queryFilter;
}
