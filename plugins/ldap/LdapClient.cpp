/*
 * LdapClient.cpp - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2018 Tobias Junghans <tobydox@veyon.io>
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

#include "LdapConfiguration.h"
#include "LdapClient.h"

#include "ldapconnection.h"
#include "ldapoperation.h"
#include "ldapserver.h"
#include "ldapdn.h"


class LdapClient::LdapClientPrivate
{
public:
	LdapClientPrivate() = default;

	QStringList queryAttributes(const QString &dn, const QString &attribute,
								const QString& filter, KLDAP::LdapUrl::Scope scope )
	{
		QStringList entries;

		if( state != Bound && reconnect() == false )
		{
			qCritical( "LdapClient::queryAttributes(): not bound to server!" );
			return entries;
		}

		if( dn.isEmpty() && attribute != namingContextAttribute )
		{
			qCritical( "LdapClient::queryAttributes(): DN is empty!" );
			return entries;
		}

		if( attribute.isEmpty() )
		{
			qCritical( "LdapClient::queryAttributes(): attribute is empty!" );
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
					entries += QString::fromUtf8( value );
				}
			}

			vDebug() << "LdapClient::queryAttributes(): results:" << entries;
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
			qCritical() << "LdapClient::queryDistinguishedNames(): not bound to server!";
			return distinguishedNames;
		}

		if( dn.isEmpty() )
		{
			qCritical() << "LdapClient::queryDistinguishedNames(): DN is empty!";

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
			vDebug() << "LdapClient::queryDistinguishedNames(): results:" << distinguishedNames;
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

	QString errorString() const
	{
		if( connection.handle() == nullptr )
		{
			return connection.connectionError();
		}

		return connection.ldapErrorString();
	}

	QString errorDescription() const
	{
		const auto string = errorString();
		if( string.isEmpty() == false )
		{
			return LdapClient::tr( "LDAP error description: %1" ).arg( string );
		}

		return LdapClient::tr( "No LDAP error description available");
	}

	bool reconnect()
	{
		connection.close();
		state = Disconnected;

		connection.setServer( server );

		if( connection.connect() != 0 )
		{
			qWarning() << "LDAP connect failed:" << errorString();
			return false;
		}

		state = Connected;

		operation.setConnection( connection );
		if( operation.bind_s() != 0 )
		{
			qWarning() << "LDAP bind failed:" << errorString();
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



LdapClient::LdapClient( const LdapConfiguration& configuration, const QUrl& url, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration ),
	d( new LdapClientPrivate )
{
	reconnect( url );
}



LdapClient::~LdapClient()
{
	delete d;
}



KLDAP::LdapConnection& LdapClient::connection() const
{
	return d->connection;
}



bool LdapClient::isConnected() const
{
	return d->state >= LdapClientPrivate::Connected;
}



bool LdapClient::isBound() const
{
	return d->state >= LdapClientPrivate::Bound;
}



QStringList LdapClient::queryAttributes( const QString& dn, const QString& attribute,
										 const QString& filter, KLDAP::LdapUrl::Scope scope )
{
	return d->queryAttributes( dn, attribute, filter, scope );
}



QStringList LdapClient::queryDistinguishedNames( const QString& dn, const QString& filter, KLDAP::LdapUrl::Scope scope )
{
	return d->queryDistinguishedNames( dn, filter, scope );
}



QString LdapClient::errorDescription() const
{
	return d->errorDescription();
}



QStringList LdapClient::queryBaseDn()
{
	return d->queryDistinguishedNames( d->baseDn, QStringLiteral( "(objectclass=*)" ), KLDAP::LdapUrl::Base );
}



QString LdapClient::queryNamingContext()
{
	return queryAttributes( QString(), d->namingContextAttribute ).value( 0 );
}



const QString& LdapClient::baseDn() const
{
	return d->baseDn;
}



QString LdapClient::parentDn( const QString& dn )
{
	const auto separatorPos = dn.indexOf( QLatin1Char(',') );
	if( separatorPos > 0 && separatorPos+1 < dn.size() )
	{
		return dn.mid( separatorPos + 1 );
	}

	return QString();
}



QString LdapClient::toRelativeDn( const QString& fullDn )
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



QString LdapClient::toFullDn( const QString& relativeDn )
{
	return relativeDn + QLatin1Char( ',' ) + d->baseDn;
}



QStringList LdapClient::toRelativeDnList( const QStringList& fullDnList )
{
	QStringList relativeDnList;

	relativeDnList.reserve( fullDnList.size() );

	for( const auto& fullDn : fullDnList )
	{
		relativeDnList += toRelativeDn( fullDn );
	}

	return relativeDnList;
}



bool LdapClient::reconnect( const QUrl &url )
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

		const auto security = static_cast<ConnectionSecurity>( m_configuration.connectionSecurity() );
		switch( security )
		{
		case ConnectionSecurityTLS:
			d->server.setSecurity( KLDAP::LdapServer::TLS );
			break;
		case ConnectionSecuritySSL:
			d->server.setSecurity( KLDAP::LdapServer::SSL );
			break;
		default:
			d->server.setSecurity( KLDAP::LdapServer::None );
			break;
		}
	}

	if( m_configuration.connectionSecurity() != ConnectionSecurityNone )
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

	return true;
}



void LdapClient::initTLS()
{
	switch( m_configuration.tlsVerifyMode() )
	{
	case TLSVerifyDefault:
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertDefault );
		break;
	case TLSVerifyNever:
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertNever );
		break;
	case TLSVerifyCustomCert:
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertHard );
		d->server.setTLSCACertFile( m_configuration.tlsCACertificateFile() );
		break;
	default:
		qCritical( "LdapClient: invalid TLS verify mode specified!" );
		d->server.setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertDefault );
		break;
	}
}



QString LdapClient::constructSubDn( const QString& subtree, const QString& baseDn )
{
	if( subtree.isEmpty() )
	{
		return baseDn;
	}

	return subtree + QLatin1Char(',') + baseDn;
}



QString LdapClient::constructQueryFilter( const QString& filterAttribute,
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



QString LdapClient::escapeFilterValue( const QString& filterValue )
{
	return QString( filterValue )
			.replace( QStringLiteral("\\"), QStringLiteral("\\\\") )
			.replace( QStringLiteral("("), QStringLiteral("\\(") )
			.replace( QStringLiteral(")"), QStringLiteral("\\)") );
}
