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
#include <QHostAddress>
#include <QHostInfo>

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
								const QString& filter = QStringLiteral( "(objectclass=*)" ),
								KLDAP::LdapUrl::Scope scope = KLDAP::LdapUrl::Base )
	{
		QStringList entries;

		if( isConnected == false || isBound == false )
		{
			qCritical() << "LdapDirectory::queryAttributes(): not connected or not bound to server!";
			return entries;
		}

		if( dn.isEmpty() && attribute != namingContextAttribute )
		{
			qCritical() << "LdapDirectory::queryAttributes(): DN is empty!";
			return entries;
		}

		if( attribute.isEmpty() )
		{
			qCritical() << "LdapDirectory::queryAttributes(): attribute is empty!";
			return entries;
		}

		int id = operation.search( KLDAP::LdapDN( dn ), scope, filter, QStringList( attribute ) );

		if( id != -1 )
		{
			bool isFirstResult = true;
			QString realAttributeName = attribute.toLower();

			while( operation.waitForResult( id, LdapQueryTimeout ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
			{
				if( isFirstResult )
				{
					isFirstResult = false;

					// match attribute name from result with requested attribute name in order
					// to keep result aggregation below case-insensitive
					for( auto a : operation.object().attributes().keys() )
					{
						if( a.toLower() == realAttributeName )
						{
							realAttributeName = a;
							break;
						}
					}
				}

				// convert result list from type QList<QByteArray> to QStringList
				for( auto value : operation.object().values( realAttributeName ) )
				{
					entries += value;
				}
			}
			qDebug() << "LdapDirectory::queryAttributes(): results:" << entries;
		}
		else
		{
			qWarning() << "LDAP search failed:" << ldapErrorDescription();
		}

		return entries;
	}

	QStringList queryDistinguishedNames( const QString &dn, const QString &filter, KLDAP::LdapUrl::Scope scope )
	{
		QStringList distinguishedNames;

		if( isConnected == false || isBound == false )
		{
			qCritical() << "LdapDirectory::queryAttributes(): not connected or not bound to server!";
			return distinguishedNames;
		}

		if( dn.isEmpty() )
		{
			qCritical() << "LdapDirectory::queryDistinguishedNames(): DN is empty!";

			return distinguishedNames;
		}

		int id = operation.search( KLDAP::LdapDN( dn ), scope, filter, QStringList() );

		if( id != -1 )
		{
			while( operation.waitForResult( id, LdapQueryTimeout ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
			{
				distinguishedNames += operation.object().dn().toString();
			}
			qDebug() << "LdapDirectory::queryDistinguishedNames(): results:" << distinguishedNames;
		}
		else
		{
			qWarning() << "LDAP search failed:" << ldapErrorDescription();
		}

		return distinguishedNames;
	}

	QStringList queryCommonNames( const QString& dn,
								  const QString& filter = QStringLiteral( "(objectclass=*)" ),
								  KLDAP::LdapUrl::Scope scope = KLDAP::LdapUrl::Base )
	{
		return queryAttributes( dn, "cn", filter, scope );
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

	KLDAP::LdapUrl::Scope defaultSearchScope;

	QString userLoginAttribute;
	QString groupMemberAttribute;
	QString computerHostNameAttribute;
	bool computerHostNameAsFQDN;

	QString usersFilter;
	QString userGroupsFilter;
	QString computerGroupsFilter;

	bool identifyGroupMembersByNameAttribute;
	bool computerPoolMembersByAttribute;
	QString computerPoolAttribute;

	bool isConnected;
	bool isBound;
};



LdapDirectory::LdapDirectory( const QUrl &url ) :
	d( new LdapDirectoryPrivate )
{
	reconnect( url );
}



LdapDirectory::~LdapDirectory()
{
}



bool LdapDirectory::isEnabled() const
{
	return ItalcCore::config->isLdapIntegrationEnabled();
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
	return d->queryDistinguishedNames( d->baseDn, QString( "(objectclass=*)"), KLDAP::LdapUrl::Base );
}



QString LdapDirectory::queryNamingContext()
{
	return d->queryAttributes( QString(), d->namingContextAttribute ).value( 0 );
}



QStringList LdapDirectory::users(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->usersDn,
									   constructQueryFilter( d->userLoginAttribute, filterValue, d->usersFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::groups(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( "cn", filterValue ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::userGroups(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( "cn", filterValue, d->userGroupsFilter ),
									   d->defaultSearchScope );
}


/*!
 * \brief Returns list of computer object names matching the given hostname filter
 * \param filterValue A filter value which is used to query the host name attribute
 * \return List of DNs of all matching computer objects
 */
QStringList LdapDirectory::computers(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->computersDn,
									   constructQueryFilter( d->computerHostNameAttribute, filterValue ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::computerGroups(const QString &filterValue)
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( "cn", filterValue, d->computerGroupsFilter ) ,
									   d->defaultSearchScope );
}



QStringList LdapDirectory::computerPools(const QString &filterValue)
{
	QStringList computerPools = d->queryAttributes( d->computersDn,
													d->computerPoolAttribute,
													constructQueryFilter( d->computerPoolAttribute, filterValue ),
													d->defaultSearchScope );

	computerPools.removeDuplicates();

	qSort( computerPools );

	return computerPools;
}



QStringList LdapDirectory::groupMembers(const QString &groupDn)
{
	return d->queryAttributes( groupDn, d->groupMemberAttribute );
}



QStringList LdapDirectory::groupsOfUser(const QString &userDn)
{
	QString userId = groupMemberUserIdentification( userDn );

	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( d->groupMemberAttribute, userId, d->userGroupsFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::groupsOfComputer(const QString &computerDn)
{
	QString computerId = groupMemberComputerIdentification( computerDn );

	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( d->groupMemberAttribute, computerId, d->computerGroupsFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::computerPoolsOfUser(const QString &userDn)
{
	if( d->computerPoolMembersByAttribute )
	{
		return d->queryAttributes( userDn, d->computerPoolAttribute );
	}

	QString userId = groupMemberUserIdentification( userDn );

	return d->queryCommonNames( d->groupsDn,
								constructQueryFilter( d->groupMemberAttribute, userId, d->computerGroupsFilter ),
								d->defaultSearchScope );
}



QStringList LdapDirectory::computerPoolsOfComputer(const QString &computerDn)
{
	if( d->computerPoolMembersByAttribute )
	{
		return d->queryAttributes( computerDn, d->computerPoolAttribute );
	}

	QString computerId = groupMemberComputerIdentification( computerDn );

	return d->queryCommonNames( d->groupsDn,
								constructQueryFilter( d->groupMemberAttribute, computerId, d->computerGroupsFilter ),
								d->defaultSearchScope );
}


/*!
 * \brief Determines common aggregations (groups, computer pools) of two objects
 * \param objectOne DN of first object
 * \param objectTwo DN of second object
 * \return list of computer pools and groups which both objects have in common
 */
QStringList LdapDirectory::commonAggregations(const QString &objectOne, const QString &objectTwo)
{
	QStringList commonComputerPools;

	if( d->computerPoolMembersByAttribute )
	{
		QStringList computerPoolsOfObjectOne = d->queryAttributes( objectOne, d->computerPoolAttribute );
		QStringList computerPoolsOfObjectTwo = d->queryAttributes( objectTwo, d->computerPoolAttribute );

		// get intersection of computer pool list of both objects
		commonComputerPools = computerPoolsOfObjectOne.toSet().intersect(
					computerPoolsOfObjectTwo.toSet() ).toList();
	}

	return commonComputerPools +
			d->queryDistinguishedNames( d->groupsDn,
										QString( "(&(%1=%2)(%1=%3))" ).arg( d->groupMemberAttribute, objectOne, objectTwo ),
										d->defaultSearchScope );
}



QString LdapDirectory::userLoginName(const QString &userDn)
{
	return d->queryAttributes( userDn, d->userLoginAttribute ).value( 0 );
}



QString LdapDirectory::groupName(const QString &groupDn)
{
	return d->queryCommonNames( groupDn ).value( 0 );
}



QString LdapDirectory::computerHostName(const QString &computerDn)
{
	if( computerDn.isEmpty() )
	{
		return QString();
	}

	return d->queryAttributes( computerDn, d->computerHostNameAttribute ).value( 0 );
}



QString LdapDirectory::groupMemberUserIdentification(const QString &userDn)
{
	if( d->identifyGroupMembersByNameAttribute )
	{
		return userLoginName( userDn );
	}

	return userDn;
}



QString LdapDirectory::groupMemberComputerIdentification(const QString &computerDn)
{
	if( d->identifyGroupMembersByNameAttribute )
	{
		return computerHostName( computerDn );
	}

	return computerDn;
}



QStringList LdapDirectory::computerPoolMembers(const QString &computerPoolName)
{
	if( d->computerPoolMembersByAttribute )
	{
		return d->queryDistinguishedNames( d->baseDn,
										   constructQueryFilter( d->computerPoolAttribute, computerPoolName ),
										   KLDAP::LdapUrl::Sub );
	}

	return groupMembers( computerGroups( computerPoolName ).value( 0 ) );
}



QStringList LdapDirectory::userGroupsNames()
{
	return d->queryCommonNames( d->groupsDn,
								d->userGroupsFilter,
								d->defaultSearchScope );
}



QStringList LdapDirectory::computerGroupsNames()
{
	return d->queryCommonNames( d->groupsDn,
								d->computerGroupsFilter,
								d->defaultSearchScope );
}



bool LdapDirectory::reconnect( const QUrl &url )
{
	ItalcConfiguration* c = ItalcCore::config;

	if( c->isLdapIntegrationEnabled() == false )
	{
		d->isConnected = false;
		d->isBound = false;

		return false;
	}

	KLDAP::LdapServer server;

	if( url.isValid() )
	{
		server.setUrl( KLDAP::LdapUrl( url ) );
	}
	else
	{
		server.setHost( c->ldapServerHost() );
		server.setPort( c->ldapServerPort() );

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
	}

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

	d->usersDn = c->ldapUserTree() + "," + d->baseDn;
	d->groupsDn = c->ldapGroupTree() + "," + d->baseDn;
	d->computersDn = c->ldapComputerTree() + "," + d->baseDn;

	if( c->ldapRecursiveSearchOperations() )
	{
		d->defaultSearchScope = KLDAP::LdapUrl::Sub;
	}
	else
	{
		d->defaultSearchScope = KLDAP::LdapUrl::One;
	}

	d->userLoginAttribute = c->ldapUserLoginAttribute();
	d->groupMemberAttribute = c->ldapGroupMemberAttribute();
	d->computerHostNameAttribute = c->ldapComputerHostNameAttribute();
	d->computerHostNameAsFQDN = c->ldapComputerHostNameAsFQDN();

	d->usersFilter = c->ldapUsersFilter();
	d->userGroupsFilter = c->ldapUserGroupsFilter();
	d->computerGroupsFilter = c->ldapComputerGroupsFilter();

	d->identifyGroupMembersByNameAttribute = c->ldapIdentifyGroupMembersByNameAttribute();

	d->computerPoolMembersByAttribute = c->ldapComputerPoolMembersByAttribute();
	d->computerPoolAttribute = c->ldapComputerPoolAttribute();

	return true;
}



QString LdapDirectory::constructQueryFilter( const QString& filterAttribute,
											 const QString& filterValue,
											 const QString& extraFilter )
{
	QString queryFilter;

	if( filterAttribute.isEmpty() == false )
	{
		if( filterValue.isEmpty() )
		{
			queryFilter = QString( "(%1=*)" ).arg( filterAttribute );
		}
		else
		{
			queryFilter = QString( "(%1=%2)" ).arg( filterAttribute ).arg( filterValue );
		}
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



QString LdapDirectory::hostNameToLdapFormat(const QString &hostName)
{
	QHostAddress hostAddress( hostName );

	// no valid IP address given?
	if( hostAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol )
	{
		// then try to resolve ist first
		QHostInfo hostInfo = QHostInfo::fromName( hostName );
		if( hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().isEmpty() )
		{
			qWarning() << "LdapDirectory::hostNameToLdapFormat(): could not lookup IP address host host"
					   << hostName << "error:" << hostInfo.errorString();
			return hostName;
		}

		hostAddress = hostInfo.addresses().first();
	}

	// now do a name lookup to get the full host name information
	QHostInfo hostInfo = QHostInfo::fromName( hostAddress.toString() );
	if( hostInfo.error() != QHostInfo::NoError )
	{
		qWarning() << "LdapDirectory::hostNameToLdapFormat(): could not lookup host name for IP"
				   << hostAddress.toString() << "error:" << hostInfo.errorString();
		return hostName;
	}

	// are we working with fully qualified domain name?
	if( d->computerHostNameAsFQDN )
	{
		return hostInfo.hostName();
	}

	// return first part of host name which should be the actual machine name
	return hostInfo.hostName().split( '.' ).value( 0 );
}
