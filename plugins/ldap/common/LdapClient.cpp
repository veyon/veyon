/*
 * LdapClient.cpp - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <ldap.h>

#include "ldapconnection.h"
#include "ldapoperation.h"
#include "ldapserver.h"


static inline KLDAP::LdapUrl::Scope kldapUrlScope( LdapClient::Scope scope )
{
	switch( scope )
	{
	case LdapClient::Scope::Base: return KLDAP::LdapUrl::Base;
	case LdapClient::Scope::One: return KLDAP::LdapUrl::One;
	case LdapClient::Scope::Sub: return KLDAP::LdapUrl::Sub;
	}

	return KLDAP::LdapUrl::Base;
}



LdapClient::LdapClient( const LdapConfiguration& configuration, const QUrl& url, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration ),
	m_server( new KLDAP::LdapServer ),
	m_connection( new KLDAP::LdapConnection ),
	m_operation( new KLDAP::LdapOperation )
{
	connectAndBind( url );
}



LdapClient::~LdapClient()
{
	delete m_connection;
	delete m_operation;
	delete m_server;
}



QString LdapClient::errorString() const
{
	if( m_connection->handle() == nullptr )
	{
		return m_connection->connectionError();
	}

	return m_connection->ldapErrorString();
}



QString LdapClient::errorDescription() const
{
	const auto string = errorString();
	if( string.isEmpty() == false )
	{
		return tr( "LDAP error description: %1" ).arg( string );
	}

	return {};
}



LdapClient::Objects LdapClient::queryObjects( const QString& dn, const QStringList& attributes,
											  const QString& filter, LdapClient::Scope scope )
{
	vDebug() << "called with" << dn << attributes << filter << scope;

	if( m_state != Bound && reconnect() == false )
	{
		vCritical() << "not bound to server!";
		return {};
	}

	if( dn.isEmpty() )
	{
		vCritical() << "DN is empty!";
		return {};
	}

	if( attributes.isEmpty() )
	{
		vCritical() << "attributes empty!";
		return {};
	}

	Objects entries;

	int result = -1;
	auto id = m_operation->search( KLDAP::LdapDN( dn ), kldapUrlScope( scope ), filter, QStringList( attributes ) );

	if( id != -1 )
	{
		auto realAttributeNames = attributes;
		for( auto& attribute : realAttributeNames )
		{
			attribute = attribute.toLower();
		}

		auto isFirstResult = true;

		while( ( result = m_operation->waitForResult( id, LdapQueryTimeout ) ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
		{
			if( isFirstResult )
			{
				isFirstResult = false;

				// match attribute name from result with requested attribute name in order
				// to keep result aggregation below case-insensitive
				const auto attributes = m_operation->object().attributes();
				for( auto it = attributes.constBegin(), end = attributes.constEnd(); it != end; ++it )
				{
					for( auto& attribute : realAttributeNames )
					{
						if( QString::compare( it.key().toLower(), attribute, Qt::CaseInsensitive ) == 0 )
						{
							attribute = it.key();
							break;
						}
					}
				}
			}

			// convert result list from type QList<QByteArray> to QStringList
			const auto dn = m_operation->object().dn().toString();
			for( const auto& attribute : realAttributeNames )
			{
				const auto values = m_operation->object().values( attribute );
				for( const auto& value : values )
				{
					entries[dn][attribute] += QString::fromUtf8( value );
				}
			}
		}

		vDebug() << "results:" << entries;
	}

	if( result == -1 )
	{
		vWarning() << "LDAP search failed with code" << m_connection->ldapErrorCode();

		if( m_state == Bound && m_queryRetry == false )
		{
			// close connection and try again
			m_queryRetry = true;
			m_state = Disconnected;
			entries = queryObjects( dn, attributes, filter, scope );
			m_queryRetry = false;
		}
	}

	return entries;

}



QStringList LdapClient::queryAttributeValues( const QString& dn, const QString& attribute,
											  const QString& filter, Scope scope )
{
	vDebug() << "called with" << dn << attribute << filter << scope;

	if( m_state != Bound && reconnect() == false )
	{
		vCritical() << "not bound to server!";
		return {};
	}

	if( dn.isEmpty() && attribute != m_namingContextAttribute &&
		attribute.contains( QLatin1String("namingcontext"), Qt::CaseInsensitive ) == false )
	{
		vCritical() << "DN is empty!";
		return {};
	}

	if( attribute.isEmpty() )
	{
		vCritical() << "attribute is empty!";
		return {};
	}

	QStringList entries;

	int result = -1;
	int id = m_operation->search( KLDAP::LdapDN( dn ), kldapUrlScope( scope ), filter, QStringList( attribute ) );

	if( id != -1 )
	{
		bool isFirstResult = true;
		QString realAttributeName = attribute.toLower();

		while( ( result = m_operation->waitForResult( id, LdapQueryTimeout ) ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
		{
			if( isFirstResult )
			{
				isFirstResult = false;

				// match attribute name from result with requested attribute name in order
				// to keep result aggregation below case-insensitive
				const auto attributes = m_operation->object().attributes();
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
			const auto values = m_operation->object().values( realAttributeName );
			for( const auto& value : values )
			{
				entries += QString::fromUtf8( value );
			}
		}

		vDebug() << "results:" << entries;
	}

	if( result == -1 )
	{
		vWarning() << "LDAP search failed with code" << m_connection->ldapErrorCode();

		if( m_state == Bound && m_queryRetry == false )
		{
			// close connection and try again
			m_queryRetry = true;
			m_state = Disconnected;
			entries = queryAttributeValues( dn, attribute, filter, scope );
			m_queryRetry = false;
		}
	}

	return entries;
}



QStringList LdapClient::queryDistinguishedNames( const QString& dn, const QString& filter, Scope scope )
{
	vDebug() << dn << filter << scope;

	if( m_state != Bound && reconnect() == false )
	{
		vCritical() << "not bound to server!";
		return {};
	}

	if( dn.isEmpty() )
	{
		vCritical() << "DN is empty!";
		return {};
	}

	QStringList distinguishedNames;

	int result = -1;
	int id = m_operation->search( KLDAP::LdapDN( dn ), kldapUrlScope( scope ), filter, QStringList() );

	if( id != -1 )
	{
		while( ( result = m_operation->waitForResult( id, LdapQueryTimeout ) ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
		{
			distinguishedNames += m_operation->object().dn().toString();
		}
		vDebug() << "results" << distinguishedNames;
	}

	if( result == -1 )
	{
		vWarning() << "LDAP search failed with code" << m_connection->ldapErrorCode();

		if( m_state == Bound && m_queryRetry == false )
		{
			// close connection and try again
			m_queryRetry = true;
			m_state = Disconnected;
			distinguishedNames = queryDistinguishedNames( dn, filter, scope );
			m_queryRetry = false;
		}
	}

	return distinguishedNames;
}



QStringList LdapClient::queryObjectAttributes( const QString& dn )
{
	vDebug() << "called with" << dn;

	if( m_state != Bound && reconnect() == false )
	{
		vCritical() << "not bound to server!";
		return {};
	}

	if( dn.isEmpty() )
	{
		vCritical() << "DN is empty!";
		return {};
	}

	int id = 0;
	if( ldap_search_ext( static_cast<LDAP *>( m_connection->handle() ),
						 dn.toUtf8().data(), LDAP_SCOPE_BASE, "objectClass=*",
						 nullptr, 1, nullptr, nullptr, nullptr,
						 m_connection->sizeLimit(), &id ) != 0 )
	{
		return {};
	}

	if( m_operation->waitForResult( id, LdapQueryTimeout ) == KLDAP::LdapOperation::RES_SEARCH_ENTRY )
	{
		const auto keys = m_operation->object().attributes().keys();
		vDebug() << "results" << keys;
		return keys;
	}

	return {};
}



QStringList LdapClient::queryBaseDn()
{
	return queryDistinguishedNames( baseDn(), QStringLiteral( "(objectclass=*)" ), Scope::Base );
}



QStringList LdapClient::queryNamingContexts( const QString& attribute )
{
	return queryAttributeValues( {}, attribute.isEmpty() ? m_namingContextAttribute : attribute );
}



QString LdapClient::baseDn()
{
	if( m_baseDn.isEmpty() )
	{
		// query base DN via naming context if configured
		if( m_configuration.queryNamingContext() )
		{
			m_baseDn = queryNamingContexts().value( 0 );
		}
		else
		{
			// use the configured base DN
			m_baseDn = m_configuration.baseDn();
		}
	}

	return m_baseDn;
}



QString LdapClient::parentDn( const QString& dn )
{
	auto rdns = toRDNs( dn );
	if( rdns.size() > 1 )
	{
		return rdns.mid( 1 ).join( QLatin1Char( ',') );
	}

	return {};
}



QString LdapClient::stripBaseDn( const QString& dn, const QString& baseDn )
{
	const auto fullDnLower = dn.toLower();
	const auto baseDnLower = baseDn.toLower();

	if( fullDnLower.endsWith( QLatin1Char( ',' ) + baseDnLower ) && dn.length() > baseDn.length()+1 )
	{
		// cut off comma and base DN
		return dn.left( dn.length() - baseDn.length() - 1 );
	}
	else if( fullDnLower == baseDnLower )
	{
		return {};
	}

	return dn;
}



QString LdapClient::addBaseDn( const QString& relativeDn, const QString& baseDn )
{
	if( relativeDn.isEmpty() )
	{
		return baseDn;
	}

	return relativeDn + QLatin1Char( ',' ) + baseDn;
}



QStringList LdapClient::stripBaseDn( const QStringList& dns, const QString& baseDn )
{
	QStringList strippedDns;

	strippedDns.reserve( dns.size() );

	for( const auto& dn : dns )
	{
		strippedDns += stripBaseDn( dn, baseDn );
	}

	return strippedDns;
}



bool LdapClient::connectAndBind( const QUrl& url )
{
	if( url.isValid() )
	{
		m_server->setUrl( KLDAP::LdapUrl( url ) );
	}
	else
	{
		m_server->setHost( m_configuration.serverHost() );
		m_server->setPort( m_configuration.serverPort() );

		if( m_configuration.useBindCredentials() )
		{
			m_server->setBindDn( m_configuration.bindDn() );
			m_server->setPassword( QString::fromUtf8( m_configuration.bindPassword().plainText().toByteArray() ) );
			m_server->setAuth( KLDAP::LdapServer::Simple );
		}
		else
		{
			m_server->setAuth( KLDAP::LdapServer::Anonymous );
		}

		const auto security = static_cast<ConnectionSecurity>( m_configuration.connectionSecurity() );
		switch( security )
		{
		case ConnectionSecurityTLS:
			m_server->setSecurity( KLDAP::LdapServer::TLS );
			break;
		case ConnectionSecuritySSL:
			m_server->setSecurity( KLDAP::LdapServer::SSL );
			break;
		default:
			m_server->setSecurity( KLDAP::LdapServer::None );
			break;
		}
	}

	if( m_configuration.connectionSecurity() != ConnectionSecurityNone )
	{
		initTLS();
	}

	if( reconnect() == false )
	{
		return false;
	}

	m_namingContextAttribute = m_configuration.namingContextAttribute();

	if( m_namingContextAttribute.isEmpty() )
	{
		// fallback to AD default value
		m_namingContextAttribute = QStringLiteral( "defaultNamingContext" );
	}

	return true;
}



void LdapClient::initTLS()
{
	switch( m_configuration.tlsVerifyMode() )
	{
	case TLSVerifyDefault:
		m_server->setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertDefault );
		break;
	case TLSVerifyNever:
		m_server->setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertNever );
		break;
	case TLSVerifyCustomCert:
		m_server->setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertHard );
		m_server->setTLSCACertFile( m_configuration.tlsCACertificateFile() );
		break;
	default:
		vCritical() << "invalid TLS verify mode specified!";
		m_server->setTLSRequireCertificate( KLDAP::LdapServer::TLSReqCertDefault );
		break;
	}
}



bool LdapClient::reconnect()
{
	m_connection->close();
	m_state = Disconnected;

	m_connection->setServer( *m_server );

	if( qEnvironmentVariableIsSet( "VEYON_DEBUG_LDAP_LIBRARY") )
	{
		const auto debugLevel = LdapLibraryDebugAny;
		ldap_set_option( nullptr, LDAP_OPT_DEBUG_LEVEL, &debugLevel );
		ber_set_option( nullptr, LDAP_OPT_DEBUG_LEVEL, &debugLevel );
	}

	if( m_connection->connect() != 0 )
	{
		vWarning() << "LDAP connect failed:" << errorString();
		return false;
	}

	m_state = Connected;

	m_operation->setConnection( *m_connection );
	if( m_operation->bind_s() != 0 )
	{
		vWarning() << "LDAP bind failed:" << errorString();
		return false;
	}

	m_state = Bound;

	return true;
}



QString LdapClient::constructSubDn( const QString& subtree, const QString& baseDn )
{
	if( baseDn.isEmpty() )
	{
		return {};
	}

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



QStringList LdapClient::toRDNs( const QString& dn )
{
	QStringList strParts;
	int index = 0;
	int searchFrom = 0;
	int strPartStartIndex = 0;
	while( ( index = dn.indexOf( QLatin1Char(','), searchFrom ) ) != -1)
	{
		const auto prev = dn[std::max(0, index - 1)];
		if (prev != QLatin1Char('\\')) {
			strParts.append( dn.mid(strPartStartIndex, index - strPartStartIndex) );
			strPartStartIndex = index + 1;
		}

		searchFrom = index + 1;
	}

	strParts.append( dn.mid(strPartStartIndex) );

	return strParts;
}
