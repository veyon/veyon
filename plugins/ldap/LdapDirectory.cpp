/*
 * LdapDirectory.cpp - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QHostAddress>
#include <QHostInfo>

#include "LdapConfiguration.h"
#include "LdapDirectory.h"

#include "ldapconnection.h"
#include "ldapoperation.h"
#include "ldapserver.h"
#include "ldapdn.h"


class LdapDirectory::LdapDirectoryPrivate
{
public:
	LdapDirectoryPrivate() = default;

	QStringList queryAttributes(const QString &dn, const QString &attribute,
								const QString& filter = QStringLiteral( "(objectclass=*)" ),
								KLDAP::LdapUrl::Scope scope = KLDAP::LdapUrl::Base )
	{
		QStringList entries;

		if( state != Bound && reconnect() == false )
		{
			qCritical( "LdapDirectory::queryAttributes(): not bound to server!" );
			return entries;
		}

		if( dn.isEmpty() && attribute != namingContextAttribute )
		{
			qCritical( "LdapDirectory::queryAttributes(): DN is empty!" );
			return entries;
		}

		if( attribute.isEmpty() )
		{
			qCritical( "LdapDirectory::queryAttributes(): attribute is empty!" );
			return entries;
		}

		int result = -1;
		int id = operation.search( KLDAP::LdapDN( dn ), scope, filter, QStringList( attribute ) );

		if( id != -1 )
		{
			bool isFirstResult = true;
			QString realAttributeName = attribute.toLower();

			while( ( result = operation.waitForResult( id, LdapQueryTimeout ) ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
			{
				if( isFirstResult )
				{
					isFirstResult = false;

					// match attribute name from result with requested attribute name in order
					// to keep result aggregation below case-insensitive
					const auto attributes = operation.object().attributes();
					for( auto it = attributes.constBegin(), end = attributes.constEnd(); it != end; ++it )
					{
						if( it.key().toLower() == realAttributeName )
						{
							realAttributeName = it.key();
							break;
						}
					}
				}

				// convert result list from type QList<QByteArray> to QStringList
				const auto values = operation.object().values( realAttributeName );
				for( const auto& value : values )
				{
					entries += value;
				}
			}

			qDebug() << "LdapDirectory::queryAttributes(): results:" << entries;
		}

		if( result == -1 )
		{
			qWarning() << "LDAP search failed with code" << connection.ldapErrorCode();

			if( state == Bound && queryRetry == false )
			{
				// close connection and try again
				queryRetry = true;
				state = Disconnected;
				entries = queryAttributes( dn, attribute, filter, scope );
				queryRetry = false;
			}
		}

		return entries;
	}

	QStringList queryDistinguishedNames( const QString& dn, const QString& filter, KLDAP::LdapUrl::Scope scope )
	{
		QStringList distinguishedNames;

		if( state != Bound && reconnect() == false )
		{
			qCritical() << "LdapDirectory::queryDistinguishedNames(): not bound to server!";
			return distinguishedNames;
		}

		if( dn.isEmpty() )
		{
			qCritical() << "LdapDirectory::queryDistinguishedNames(): DN is empty!";

			return distinguishedNames;
		}

		int result = -1;
		int id = operation.search( KLDAP::LdapDN( dn ), scope, filter, QStringList() );

		if( id != -1 )
		{
			while( ( result = operation.waitForResult( id, LdapQueryTimeout ) ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
			{
				distinguishedNames += operation.object().dn().toString();
			}
			qDebug() << "LdapDirectory::queryDistinguishedNames(): results:" << distinguishedNames;
		}

		if( result == -1 )
		{
			qWarning() << "LDAP search failed with code" << connection.ldapErrorCode();

			if( state == Bound && queryRetry == false )
			{
				// close connection and try again
				queryRetry = true;
				state = Disconnected;
				distinguishedNames = queryDistinguishedNames( dn, filter, scope );
				queryRetry = false;
			}
		}

		return distinguishedNames;
	}

	QString ldapErrorString() const
	{
		if( connection.handle() == nullptr )
		{
			return connection.connectionError();
		}

		return connection.ldapErrorString();
	}

	QString ldapErrorDescription() const
	{
		const auto errorString = ldapErrorString();
		if( errorString.isEmpty() == false )
		{
			return LdapDirectory::tr( "LDAP error description: %1" ).arg( errorString );
		}

		return LdapDirectory::tr( "No LDAP error description available");
	}

	bool reconnect()
	{
		connection.close();
		state = Disconnected;

		connection.setServer( server );

		if( connection.connect() != 0 )
		{
			qWarning() << "LDAP connect failed:" << ldapErrorString();
			return false;
		}

		state = Connected;

		operation.setConnection( connection );
		if( operation.bind_s() != 0 )
		{
			qWarning() << "LDAP bind failed:" << ldapErrorString();
			return false;
		}

		state = Bound;

		return true;
	}

	enum {
		LdapQueryTimeout = 3000,
		LdapConnectionTimeout = 60*1000
	};

	KLDAP::LdapServer server;
	KLDAP::LdapConnection connection;
	KLDAP::LdapOperation operation;

	QString baseDn;
	QString namingContextAttribute;
	QString usersDn;
	QString groupsDn;
	QString computersDn;
	QString computerGroupsDn;

	QString userLoginAttribute;
	QString groupMemberAttribute;
	QString computerHostNameAttribute;
	QString computerMacAddressAttribute;
	QString computerRoomNameAttribute;

	QString usersFilter;
	QString userGroupsFilter;
	QString computersFilter;
	QString computerGroupsFilter;
	QString computerParentsFilter;

	QString computerRoomAttribute;

	KLDAP::LdapUrl::Scope defaultSearchScope = KLDAP::LdapUrl::Base;

	bool identifyGroupMembersByNameAttribute = false;
	bool computerRoomMembersByContainer = false;
	bool computerRoomMembersByAttribute = false;
	bool computerHostNameAsFQDN = false;

	using State = enum States
	{
		Disconnected,
		Connected,
		Bound,
		StateCount
	} ;

	State state = Disconnected;

	bool queryRetry = false;

};



LdapDirectory::LdapDirectory( const LdapConfiguration& configuration, const QUrl& url, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration ),
	d( new LdapDirectoryPrivate )
{
	reconnect( url );
}



LdapDirectory::~LdapDirectory()
{
}



bool LdapDirectory::isConnected() const
{
	return d->state >= LdapDirectoryPrivate::Connected;
}



bool LdapDirectory::isBound() const
{
	return d->state >= LdapDirectoryPrivate::Bound;
}


/*!
 * \brief Disables any configured attributes which is required for some test scenarious
 */
void LdapDirectory::disableAttributes()
{
	d->userLoginAttribute.clear();
	d->computerHostNameAttribute.clear();
	d->computerMacAddressAttribute.clear();
}



/*!
 * \brief Disables any configured filters which is required for some test scenarious
 */
void LdapDirectory::disableFilters()
{
	d->usersFilter.clear();
	d->userGroupsFilter.clear();
	d->computersFilter.clear();
	d->computerGroupsFilter.clear();
	d->computerParentsFilter.clear();
}



QString LdapDirectory::ldapErrorDescription() const
{
	return d->ldapErrorDescription();
}



QStringList LdapDirectory::queryBaseDn()
{
	return d->queryDistinguishedNames( d->baseDn, QStringLiteral( "(objectclass=*)" ), KLDAP::LdapUrl::Base );
}



QString LdapDirectory::queryNamingContext()
{
	return d->queryAttributes( QString(), d->namingContextAttribute ).value( 0 );
}



QString LdapDirectory::parentDn( const QString& dn )
{
	const auto separatorPos = dn.indexOf( ',' );
	if( separatorPos > 0 && separatorPos+1 < dn.size() )
	{
		return dn.mid( separatorPos + 1 );
	}

	return QString();
}



QString LdapDirectory::toRelativeDn( const QString& fullDn )
{
	const auto fullDnLower = fullDn.toLower();
	const auto baseDnLower = d->baseDn.toLower();

	if( fullDnLower.endsWith( QLatin1Char( ',' ) + baseDnLower ) &&
			fullDn.length() > d->baseDn.length()+1 )
	{
		// cut off comma and base DN
		return fullDn.left( fullDn.length() - d->baseDn.length() - 1 );
	}
	return fullDn;
}



QStringList LdapDirectory::toRelativeDnList( const QStringList& fullDnList )
{
	QStringList relativeDnList;

	relativeDnList.reserve( fullDnList.size() );

	for( const auto& fullDn : fullDnList )
	{
		relativeDnList += toRelativeDn( fullDn );
	}

	return relativeDnList;
}



QStringList LdapDirectory::users( const QString& filterValue )
{
	return d->queryDistinguishedNames( d->usersDn,
									   constructQueryFilter( d->userLoginAttribute, filterValue, d->usersFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::groups( const QString& filterValue )
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( QStringLiteral( "cn" ), filterValue ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::userGroups( const QString& filterValue )
{
	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( QStringLiteral( "cn" ), filterValue, d->userGroupsFilter ),
									   d->defaultSearchScope );
}


/*!
 * \brief Returns list of computer object names matching the given hostname filter
 * \param filterValue A filter value which is used to query the host name attribute
 * \return List of DNs of all matching computer objects
 */
QStringList LdapDirectory::computers( const QString& filterValue )
{
	return d->queryDistinguishedNames( d->computersDn,
									   constructQueryFilter( d->computerHostNameAttribute, filterValue, d->computersFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::computerGroups( const QString& filterValue )
{
	return d->queryDistinguishedNames( d->computerGroupsDn.isEmpty() ? d->groupsDn : d->computerGroupsDn,
									   constructQueryFilter( QStringLiteral( "cn" ), filterValue, d->computerGroupsFilter ) ,
									   d->defaultSearchScope );
}



QStringList LdapDirectory::computerRooms( const QString& filterValue )
{
	QStringList computerRooms;

	if( d->computerRoomMembersByAttribute )
	{
		computerRooms = d->queryAttributes( d->computersDn,
											d->computerRoomAttribute,
											constructQueryFilter( d->computerRoomAttribute, filterValue, d->computersFilter ),
											d->defaultSearchScope );
	}
	else if( d->computerRoomMembersByContainer )
	{
		computerRooms = d->queryAttributes( d->computersDn,
											d->computerRoomNameAttribute,
											constructQueryFilter( d->computerRoomNameAttribute, filterValue, d->computerParentsFilter ) ,
											d->defaultSearchScope );
	}
	else
	{
		computerRooms = d->queryAttributes( d->computerGroupsDn.isEmpty() ? d->groupsDn : d->computerGroupsDn,
											d->computerRoomNameAttribute,
											constructQueryFilter( d->computerRoomNameAttribute, filterValue, d->computerGroupsFilter ) ,
											d->defaultSearchScope );
	}

	computerRooms.removeDuplicates();

	std::sort( computerRooms.begin(), computerRooms.end() );

	return computerRooms;
}



QStringList LdapDirectory::groupMembers( const QString& groupDn )
{
	return d->queryAttributes( groupDn, d->groupMemberAttribute );
}



QStringList LdapDirectory::groupsOfUser( const QString& userDn )
{
	const auto userId = groupMemberUserIdentification( userDn );
	if( d->groupMemberAttribute.isEmpty() || userId.isEmpty() )
	{
		return {};
	}

	return d->queryDistinguishedNames( d->groupsDn,
									   constructQueryFilter( d->groupMemberAttribute, userId, d->userGroupsFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::groupsOfComputer( const QString& computerDn )
{
	const auto computerId = groupMemberComputerIdentification( computerDn );
	if( d->groupMemberAttribute.isEmpty() || computerId.isEmpty() )
	{
		return {};
	}

	return d->queryDistinguishedNames( d->computerGroupsDn.isEmpty() ? d->groupsDn : d->computerGroupsDn,
									   constructQueryFilter( d->groupMemberAttribute, computerId, d->computerGroupsFilter ),
									   d->defaultSearchScope );
}



QStringList LdapDirectory::computerRoomsOfComputer( const QString& computerDn )
{
	if( d->computerRoomMembersByAttribute )
	{
		return d->queryAttributes( computerDn, d->computerRoomAttribute );
	}
	else if( d->computerRoomMembersByContainer )
	{
		return d->queryAttributes( parentDn( computerDn ), d->computerRoomNameAttribute );
	}

	const auto computerId = groupMemberComputerIdentification( computerDn );
	if( d->groupMemberAttribute.isEmpty() || computerId.isEmpty() )
	{
		return {};
	}

	return d->queryAttributes( d->computerGroupsDn.isEmpty() ? d->groupsDn : d->computerGroupsDn,
							   d->computerRoomNameAttribute,
							   constructQueryFilter( d->groupMemberAttribute, computerId, d->computerGroupsFilter ),
							   d->defaultSearchScope );
}



QString LdapDirectory::userLoginName( const QString& userDn )
{
	return d->queryAttributes( userDn, d->userLoginAttribute ).value( 0 );
}



QString LdapDirectory::groupName( const QString& groupDn )
{
	return d->queryAttributes( groupDn,
							   QStringLiteral( "cn" ),
							   QStringLiteral( "(objectclass=*)" ),
							   KLDAP::LdapUrl::Base ).
			value( 0 );
}



QString LdapDirectory::computerHostName( const QString& computerDn )
{
	if( computerDn.isEmpty() )
	{
		return QString();
	}

	return d->queryAttributes( computerDn, d->computerHostNameAttribute ).value( 0 );
}



QString LdapDirectory::computerMacAddress( const QString& computerDn )
{
	if( computerDn.isEmpty() )
	{
		return QString();
	}

	return d->queryAttributes( computerDn, d->computerMacAddressAttribute ).value( 0 );
}



QString LdapDirectory::groupMemberUserIdentification( const QString& userDn )
{
	if( d->identifyGroupMembersByNameAttribute )
	{
		return userLoginName( userDn );
	}

	return userDn;
}



QString LdapDirectory::groupMemberComputerIdentification( const QString& computerDn )
{
	if( d->identifyGroupMembersByNameAttribute )
	{
		return computerHostName( computerDn );
	}

	return computerDn;
}



QStringList LdapDirectory::computerRoomMembers( const QString& computerRoomName )
{
	if( d->computerRoomMembersByAttribute )
	{
		return d->queryDistinguishedNames( d->computersDn,
										   constructQueryFilter( d->computerRoomAttribute, computerRoomName, d->computersFilter ),
										   d->defaultSearchScope );
	}
	else if( d->computerRoomMembersByContainer )
	{
		const auto roomDnFilter = constructQueryFilter( d->computerRoomNameAttribute, computerRoomName, d->computerParentsFilter );
		const auto roomDns = d->queryDistinguishedNames( d->computersDn, roomDnFilter, d->defaultSearchScope );

		return d->queryDistinguishedNames( roomDns.value( 0 ),
										   constructQueryFilter( QString(), QString(), d->computersFilter ),
										   d->defaultSearchScope );
	}

	auto memberComputers = groupMembers( computerGroups( computerRoomName ).value( 0 ) );

	// computer filter configured?
	if( d->computersFilter.isEmpty() == false )
	{
		auto memberComputersSet = memberComputers.toSet();

		// then return intersection of filtered computer list and group members
		return memberComputersSet.intersect( computers().toSet() ).toList();
	}

	return memberComputers;
}



bool LdapDirectory::reconnect( const QUrl &url )
{
	if( url.isValid() )
	{
		d->server.setUrl( KLDAP::LdapUrl( url ) );
	}
	else
	{
		d->server.setHost( m_configuration.serverHost() );
		d->server.setPort( m_configuration.serverPort() );

		if( m_configuration.useBindCredentials() )
		{
			d->server.setBindDn( m_configuration.bindDn() );
			d->server.setPassword( m_configuration.bindPassword() );
			d->server.setAuth( KLDAP::LdapServer::Simple );
		}
		else
		{
			d->server.setAuth( KLDAP::LdapServer::Anonymous );
		}

		const auto security = static_cast<LdapConfiguration::ConnectionSecurity>( m_configuration.connectionSecurity() );
		switch( security )
		{
		case LdapConfiguration::ConnectionSecurityTLS:
			d->server.setSecurity( KLDAP::LdapServer::TLS );
			break;
		case LdapConfiguration::ConnectionSecuritySSL:
			d->server.setSecurity( KLDAP::LdapServer::SSL );
			break;
		default:
			d->server.setSecurity( KLDAP::LdapServer::None );
			break;
		}
	}

	if( m_configuration.connectionSecurity() != LdapConfiguration::ConnectionSecurityNone )
	{
		initTLS();
	}

	if( d->reconnect() == false )
	{
		return false;
	}

	d->namingContextAttribute = m_configuration.namingContextAttribute();

	if( d->namingContextAttribute.isEmpty() )
	{
		// fallback to AD default value
		d->namingContextAttribute = QStringLiteral( "defaultNamingContext" );
	}

	// query base DN via naming context if configured
	if( m_configuration.queryNamingContext() )
	{
		d->baseDn = queryNamingContext();
	}
	else
	{
		// use the configured base DN
		d->baseDn = m_configuration.baseDn();
	}

	d->usersDn = constructSubDn( m_configuration.userTree(), d->baseDn );
	d->groupsDn = constructSubDn( m_configuration.groupTree(), d->baseDn );
	d->computersDn = constructSubDn( m_configuration.computerTree(), d->baseDn );

	if( m_configuration.computerGroupTree().isEmpty() == false )
	{
		d->computerGroupsDn = constructSubDn( m_configuration.computerGroupTree(), d->baseDn );
	}
	else
	{
		d->computerGroupsDn.clear();
	}

	if( m_configuration.recursiveSearchOperations() )
	{
		d->defaultSearchScope = KLDAP::LdapUrl::Sub;
	}
	else
	{
		d->defaultSearchScope = KLDAP::LdapUrl::One;
	}

	d->userLoginAttribute = m_configuration.userLoginAttribute();
	d->groupMemberAttribute = m_configuration.groupMemberAttribute();
	d->computerHostNameAttribute = m_configuration.computerHostNameAttribute();
	d->computerHostNameAsFQDN = m_configuration.computerHostNameAsFQDN();
	d->computerMacAddressAttribute = m_configuration.computerMacAddressAttribute();
	d->computerRoomNameAttribute = m_configuration.computerRoomNameAttribute();
	if( d->computerRoomNameAttribute.isEmpty() )
	{
		d->computerRoomNameAttribute = QStringLiteral("cn");
	}

	d->usersFilter = m_configuration.usersFilter();
	d->userGroupsFilter = m_configuration.userGroupsFilter();
	d->computersFilter = m_configuration.computersFilter();
	d->computerGroupsFilter = m_configuration.computerGroupsFilter();
	d->computerParentsFilter = m_configuration.computerContainersFilter();

	d->identifyGroupMembersByNameAttribute = m_configuration.identifyGroupMembersByNameAttribute();

	d->computerRoomMembersByContainer = m_configuration.computerRoomMembersByContainer();
	d->computerRoomMembersByAttribute = m_configuration.computerRoomMembersByAttribute();
	d->computerRoomAttribute = m_configuration.computerRoomAttribute();

	return true;
}



void LdapDirectory::initTLS()
{
	switch( m_configuration.tlsVerifyMode() )
	{
	case LdapConfiguration::TLSVerifyDefault:
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertDefault );
		break;
	case LdapConfiguration::TLSVerifyNever:
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertNever );
		break;
	case LdapConfiguration::TLSVerifyCustomCert:
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertHard );
		d->server.setTLSCACertFile( m_configuration.tlsCACertificateFile() );
		break;
	default:
		qCritical( "LdapDirectory: invalid TLS verify mode specified!" );
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertDefault );
		break;
	}
}



QString LdapDirectory::constructSubDn( const QString& subtree, const QString& baseDn )
{
	if( subtree.isEmpty() )
	{
		return baseDn;
	}

	return subtree + "," + baseDn;
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
			queryFilter = QStringLiteral( "(%1=*)" ).arg( filterAttribute );
		}
		else
		{
			queryFilter = QStringLiteral( "(%1=%2)" ).arg( filterAttribute,
														   escapeFilterValue( filterValue ) );
		}
	}

	if( extraFilter.isEmpty() == false )
	{
		if( queryFilter.isEmpty() )
		{
			queryFilter = extraFilter;
		}
		else
		{
			queryFilter = QStringLiteral( "(&%1%2)" ).arg( extraFilter, queryFilter );
		}
	}

	return queryFilter;
}



QString LdapDirectory::escapeFilterValue( const QString& filterValue )
{
	return QString( filterValue )
			.replace( QStringLiteral("\\"), QStringLiteral("\\\\") )
			.replace( QStringLiteral("("), QStringLiteral("\\(") )
			.replace( QStringLiteral(")"), QStringLiteral("\\)") );
}



QString LdapDirectory::hostToLdapFormat( const QString& host )
{
	QHostAddress hostAddress( host );

	// no valid IP address given?
	if( hostAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol )
	{
		// then try to resolve ist first
		QHostInfo hostInfo = QHostInfo::fromName( host );
		if( hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().isEmpty() )
		{
			qWarning() << "LdapDirectory::hostToLdapFormat(): could not lookup IP address of host"
					   << host << "error:" << hostInfo.errorString();
			return QString();
		}

#if QT_VERSION < 0x050600
		hostAddress = hostInfo.addresses().value( 0 );
#else
		hostAddress = hostInfo.addresses().constFirst();
#endif
		qDebug() << "LdapDirectory::hostToLdapFormat(): no valid IP address given, resolved IP address of host"
				 << host << "to" << hostAddress.toString();
	}

	// now do a name lookup to get the full host name information
	QHostInfo hostInfo = QHostInfo::fromName( hostAddress.toString() );
	if( hostInfo.error() != QHostInfo::NoError )
	{
		qWarning() << "LdapDirectory::hostToLdapFormat(): could not lookup host name for IP"
				   << hostAddress.toString() << "error:" << hostInfo.errorString();
		return QString();
	}

	// are we working with fully qualified domain name?
	if( d->computerHostNameAsFQDN )
	{
		qDebug() << "LdapDirectory::hostToLdapFormat(): Resolved FQDN" << hostInfo.hostName();
		return hostInfo.hostName();
	}

	// return first part of host name which should be the actual machine name
	const QString hostName = hostInfo.hostName().split( '.' ).value( 0 );

	qDebug() << "LdapDirectory::hostToLdapFormat(): resolved host name" << hostName;
	return hostName;
}



QString LdapDirectory::computerObjectFromHost( const QString& host )
{
	QString hostName = hostToLdapFormat( host );
	if( hostName.isEmpty() )
	{
		qWarning( "LdapDirectory::computerObjectFromHost(): could not resolve hostname, returning empty computer object" );
		return QString();
	}

	QStringList computerObjects = computers( hostName );
	if( computerObjects.count() == 1 )
	{
		return computerObjects.first();
	}

	// return empty result if not exactly one object was found
	qWarning( "LdapDirectory::computerObjectFromHost(): more than one computer object found, returning empty computer object!" );
	return QString();
}
